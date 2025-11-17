/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Answer.h"

#include <algorithm>
#include <string>
#include <zlib.h>
#include "http/Cookie.h"
#include "http/CookieError.h"
#include "tools/StrUtil.h"
#include "tools/ErrUtil.h"


namespace http {
	using namespace net;

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


	int Answer::read_buffer(net::TcpSocket& socket, unsigned char* buf, const size_t len, const tools::Timer& timer)
	{
		const rcv_status status{ socket.read(buf, len, timer) };

		if (status.code == rcv_status_code::NETCTX_RCV_ERROR || status.code == rcv_status_code::NETCTX_RCV_RETRY) {
			// read failed or timed out
			throw mbed_error(status.rc);
		}

		// Returns false in case of EOF
		return status.code == rcv_status_code::NETCTX_RCV_OK;
	}


	int Answer::read_char(net::TcpSocket& socket, char& c, const tools::Timer& timer)
	{
		return read_buffer(socket, reinterpret_cast<unsigned char*>(&c), sizeof(char), timer);
	}


	int Answer::read_line(net::TcpSocket& socket, int max_len, tools::obfstring& line, const tools::Timer& timer)
	{
		DEBUG_ENTER(_logger);

		int state = 0;
		int bytes_received = 0;
		char c;

		line.clear();

		while (state != 2) {
			if (read_char(socket, c, timer) != sizeof(c))
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


	int Answer::read_http_status(net::TcpSocket& socket, const tools::Timer& timer)
	{
		DEBUG_ENTER(_logger);
		tools::obfstring line;

		// We assume that the server did not understand our request
		_status_code = default_code;
		_reason_phrase = default_reason;

		// Read the status line and return an error code if the server has closed 
		// the connection before sending a valid response.
		if (read_line(socket, MAX_HEADER_SIZE, line, timer) <= 0)
			return ERR_INVALID_STATUS_LINE;

		// The answer should have the following syntax :  HTTP-Version SP Status-Code SP Reason-Phrase
		// See https://www.rfc-editor.org/rfc/rfc9110.html 
		std::vector<std::string> parts;
		const size_t count = tools::split(line.uncrypt(), ' ', parts);
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
			for (size_t idx = 3; idx < count; idx++) {
				_reason_phrase.append(" ");
				_reason_phrase.append(parts[idx]);
			}
		}

		return count >= 2 ? ERR_NONE : ERR_INVALID_STATUS_LINE;
	}


	bool Answer::read_gzip_body(net::TcpSocket& socket, size_t size, size_t max_size, const tools::Timer& timer)
	{
		DEBUG_ENTER(_logger);

		// configure the de-compressor
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
			const size_t len = std::min(size, sizeof(buffer));
			if (!read_buffer(socket, buffer, std::min(size, sizeof(buffer)), timer)) {
				inflateEnd(&strm);
				return false;
			}

			strm.avail_in = static_cast<uInt>(len);
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
				const size_t have = sizeof(out) - strm.avail_out;

				const size_t available_space = max_size - _body.size();
				if (available_space > 0) {
					_body.append(out, std::min(have, available_space));
				}
			} while (strm.avail_out == 0);

			size = size - len;
		} while (size > 0);

		return (inflateEnd(&strm) == Z_OK) && size == 0;
	}


	bool Answer::read_body(net::TcpSocket& socket, size_t size, size_t max_size, const tools::Timer& timer)
	{
		DEBUG_ENTER(_logger);

		// read by chunk of 4096 bytes
		unsigned char buffer[4096];

		do {
			// read a chunk of bytes
			const size_t len = std::min(size, sizeof(buffer));
			if (!read_buffer(socket, buffer, len, timer))
				break;

			// append what we can in the buffer
			const size_t available_space = max_size - _body.size();
			if (available_space > 0) {
				_body.append(buffer, std::min(len, available_space));
			}

			size = size - len;
		} while (size > 0);

		// return True when all bytes have been read
		return size == 0;
	}


	int Answer::recv(net::TcpSocket& socket, const tools::Timer& timer)
	{
		if (_logger->is_trace_enabled())
			_logger->trace("... %x enter %s::%s timeout=%lu",
				(uintptr_t)this,
				__class__,
				__func__,
				timer.remaining_time()
			);

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
		if ((rc = read_http_status(socket, timer)) != ERR_NONE)
			return rc;

		// Read message-headers. 
		do {
			if ((rc = read_line(socket, MAX_HEADER_SIZE, line, timer)) <= 0)
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
			if (gzip)
				return ERR_CONTENT_ENCODING;

			// chunked message
			long chunk_size = 0;

			do {
				// read chunk size
				if ((rc = read_line(socket, MAX_LINE_SIZE, line, timer)) <= 0)
					return ERR_CHUNK_SIZE;

				// decode chunk size
				if (!tools::str2num(line.uncrypt(), 16, 0, MAX_CHUNK_SIZE, chunk_size)) {
					return ERR_CHUNK_SIZE;
				}

				if (chunk_size > 0) {
					if (!read_body(socket, chunk_size, MAX_BODY_SIZE, timer))
						return ERR_BODY;
				}

				// skip eol
				if ((rc = read_line(socket, MAX_LINE_SIZE, line, timer)) <= 0)
					return ERR_BODY;
			} while (chunk_size > 0);

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
						body_available = read_gzip_body(socket, size, MAX_BODY_SIZE, timer);
					else
						body_available = read_body(socket, size, MAX_BODY_SIZE, timer);

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

	const char* Answer::__class__ = "Answer";

}
