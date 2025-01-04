/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Answer.h"

#include <string>
#include <zlib.h>
#include "http/Cookie.h"
#include "http/CookieError.h"
#include "tools/StrUtil.h"
#include "tools/ErrUtil.h"


namespace http {

	const int default_code = 400;
	const std::string default_reason = "Bad Request";

	Answer::Answer() :
		_logger(Logger::get_logger()),
		_version(),
		_status_code(default_code),
		_reason_phrase(default_reason),
		_headers(),
		_cookies(),
		_body(4096)
	{
	}


	void Answer::clear()
	{
		_version.clear();
		_status_code = default_code;
		_reason_phrase = default_reason;
		_body.clear();
		_headers.serase();
		_cookies.clear();
	}


	int Answer::read(net::Socket& socket, unsigned char* buf, const size_t len)
	{
		const int rc = socket.read(buf, len);
		if (_logger->is_trace_enabled())
			_logger->trace("... %x       Answer::read : socket=%d len=%d rc = %d",
				(uintptr_t)this,
				socket.get_fd(),
				len,
				rc);

		if (rc < 0)
			throw mbed_error(rc);

		return rc;
	}


	int Answer::read_char(net::Socket& socket, char& c)
	{
		return read(socket, (unsigned char *)&c, sizeof(char));
	}


	int Answer::read_line(net::Socket& socket, int max_len, tools::obfstring& line)
	{
		DEBUG_ENTER(_logger, "Answer", "read_line");

		int state = 0;
		int bytes_received = 0;
		char c;

		line.clear();

		while (state != 2) {
			if (read_char(socket, c) != sizeof(c))
				break;

			switch (state) {
			case 0:
				if (c == '\r')
					state = 1;
				else if (bytes_received < max_len)
					line.push_back(c);
				break;

			case 1:
				if (c == '\n')
					state = 2;
				else if (bytes_received < max_len) {
					line.push_back('\r');
					line.push_back(c);
				}
				break;

			default:
				break;
			}

			bytes_received++;
		}

		return bytes_received;
	}


	int Answer::read_status(net::Socket& socket)
	{
		DEBUG_ENTER(_logger, "Answer", "read_status");
		tools::obfstring line;

		// We assume that the server did not understand our request
		_status_code = default_code;
		_reason_phrase = default_reason;

		// Read the status line and return an error code if the server has closed 
		// the connection before sending a valid response.
		if (read_line(socket, MAX_HEADER_SIZE, line) <= 0)
			return ERR_INVALID_STATUS_LINE;

		// The answer should have the following syntax :  HTTP-Version SP Status-Code SP Reason-Phrase
		// See https://www.rfc-editor.org/rfc/rfc9110.html 
		std::vector<std::string> parts;
		int count = tools::split(line.uncrypt(), ' ', parts);
		if (count >= 1) {
			_version = parts[0];
			if (_version.compare("HTTP/1.1") != 0) {
				return ERR_INVALID_VERSION;
			}
		}

		if (count >= 2) {
			if (!tools::str2i(parts[1], _status_code) || (_status_code < 100) || (_status_code >= 600)) {
				// Invalid HTTP code
				return ERR_INVALID_STATUS_CODE;
			}
		}

		if (count >= 3) {
			// Merge all others parts and rebuild the reason phrase if specified
			_reason_phrase = parts[2];
			for (int idx = 3; idx < count; idx++) {
				_reason_phrase.append(" ");
				_reason_phrase.append(parts[idx]);
			}
		}

		return count >= 2 ? ERR_NONE : ERR_INVALID_STATUS_LINE;
	}


	bool Answer::read_gzip_body(net::Socket& socket, int size, int max_size)
	{
		DEBUG_ENTER(_logger, "Answer", "read_gzip_body");

		// configure the decompresser
		::z_stream strm{ 0 };
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		if (inflateInit2(&strm, MAX_WBITS + 32))
			return false;

		// read by chunk of 1024 bytes
		unsigned char buffer[1024];
		unsigned char out[1024];

		do {
			const int len = read(socket, buffer, min(size, sizeof(buffer)));
			if (len <= 0) {
				inflateEnd(&strm);
				return false;
			}

			strm.avail_in = len;
			strm.next_in = buffer;

			do {
				strm.avail_out = sizeof(out);
				strm.next_out = out;

				switch (inflate(&strm, Z_NO_FLUSH)) {
					case Z_NEED_DICT:
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
						(void)inflateEnd(&strm);
						return false;
				}
				const int have = sizeof(out) - strm.avail_out;

				const size_t available_space = max_size - _body.size();
				if (available_space > 0) {
					_body.append(out, min(have, available_space));
				}
			} while (strm.avail_out == 0);

			size = size - len;
		} while (size > 0);

		return (inflateEnd(&strm) == Z_OK) && size == 0;
	}


	bool Answer::read_body(net::Socket& socket, int size, int max_size)
	{
		DEBUG_ENTER(_logger, "Answer", "read_body");

		// read by chunk of 1024 bytes
		unsigned char buffer[1024];

		do {
			// read a chunk of bytes
			const int len = read(socket, buffer, min(size, sizeof(buffer)));
			if (len <= 0)
				break;

			// append what we can in the buffer
			const size_t available_space = max_size - _body.size();
			if (available_space > 0) {
				_body.append(buffer, min(len, available_space));
			}

			size = size - len;
		} while (size > 0);

		// return True when all bytes have been read
		return size == 0;
	}


	int Answer::recv(net::Socket& socket)
	{
		int rc;
		tools::obfstring line;

		// The received message has the following syntax
		//  answer         = status-line
		//                   *(message-header CRLF)
		//                   CRLF
		//                   [ message-body ]
		//  message-header = field-name ":" [ field-value ]
		//
		//  Note : leading or trailing LWS MAY be removed without changing 
		//         the semantics of the field value

		// Read status-line.
		if ((rc = read_status(socket)) != ERR_NONE)
			return rc;

		// Read message-headers. 
		do {
			if ((rc = read_line(socket, MAX_HEADER_SIZE, line)) <= 0)
				return ERR_INVALID_HEADER;

			const std::string::size_type pos{ line.find(':') };
			if (pos > 0 && pos != std::string::npos) {
				const std::string field_name{ line.substr(0, pos).uncrypt() };
				const tools::obfstring field_value{ tools::trim(line.substr(pos + 1, std::string::npos)) };

				if (tools::iequal(field_name, "Set-Cookie")) {
					// A cookie definition
					try {
						_cookies.add(Cookie::parse(field_value));
					}
					catch (const CookieError& e) {
						_logger->trace("ERROR: %s", e.what());
					}
				}
				else {
					// it is a header
					_headers.set(field_name, field_value.uncrypt());
				}
			}
		} while (line.size() > 0);

		// Get the encoding scheme
		std::string transfer_encoding;
		if (_headers.get("Transfer-Encoding", transfer_encoding)) {
			transfer_encoding = tools::lower(tools::trim(transfer_encoding));
		}

		bool gzip = false;
		std::string content_encoding;
		if (_headers.get("Content-Encoding", content_encoding)) {
			content_encoding = tools::lower(tools::trim(content_encoding));
			if (content_encoding.compare("") == 0) {
			}
			else if (content_encoding.compare("gzip") == 0) {
				gzip = true;
			}
			else {
				return ERR_CONTENT_ENCODING;
			}
		}

		// Read the body.
		// Are we using the chunked-style of transfer encoding?
		if (transfer_encoding.compare("chunked") == 0) {
			// chucked message
			long chunck_size = 0;

			do {
				// read chunk size
				if ((rc = read_line(socket, MAX_LINE_SIZE, line)) <= 0)
					return ERR_CHUNK_SIZE;

				// decode chunk size
				if (!tools::str2num(line.uncrypt(), 16, 0, MAX_CHUNCK_SIZE, chunck_size)) {
					return ERR_CHUNK_SIZE;
				}

				if (chunck_size > 0) {
					if (!read_body(socket, chunck_size, MAX_BODY_SIZE))
						return ERR_BODY;
				}

				// skip eol
				if ((rc = read_line(socket, MAX_LINE_SIZE, line)) <= 0)
					return ERR_BODY;
			} while (chunck_size > 0);
		}
		else if (transfer_encoding.compare("") == 0) {
			// read content length 
			std::string length;

			if (_headers.get("Content-Length", length)) {
				long size = 0;

				if (!tools::str2num(length, 10, 0, MAX_BODY_SIZE, size)) {
					return ERR_BODY_SIZE;
				}

				if (size > 0) {
					// define the capacity of the buffer
					_body.reserve(size);

					// read the whole body
					bool body_available;
					if (gzip)
						body_available = read_gzip_body(socket, size, MAX_BODY_SIZE);
					else
						body_available = read_body(socket, size, MAX_BODY_SIZE);

					if (!body_available)
						return ERR_BODY;
				}
			}
		}
		else {
			// unsupported transfer encoding
			return ERR_TRANSFER_ENCODING;
		}

		return ERR_NONE;
	}

}
