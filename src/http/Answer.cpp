/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "http/Answer.h"
#include "tools/FortiError.h"
#include "tools/StrUtil.h"


namespace http {

	const std::string default_code = "400";
	const std::string default_reason = "Bad Request";

	Answer::Answer() :
		_headers(),
		_cookies(),
		_body(1024)
	{
		clear();
	}


	Answer::~Answer()
	{
		clear();
	}


	void Answer::clear()
	{
		_version.clear();
		_status_code = default_code;
		_reason_phrase = default_reason;
		_body.clear();
		_headers.clear();
		_cookies.clear();
	}


	bool Answer::read_char(net::Socket& socket, char& c)
	{
		int rc = socket.read((unsigned char *)&c, sizeof(char));
		if (rc < 0)
			throw new mbed_error(rc);

		return rc > 0;
	}


	bool Answer::read_line(net::Socket& socket, int maxlen, tools::obfstring& line)
	{
		int  state = 0;
		int  cnt = 0;
		char c;

		line.clear();

		while ((state != 2) && (cnt < maxlen)) {
			if (!read_char(socket, c))
				return false;

			cnt++;

			switch (state) {
			case 0:
				if (c == '\r')
					state = 1;
				else
					line.push_back(c);
				break;

			case 1:
				if (c == '\n')
					state = 2;
				else {
					line.push_back('\r');
					line.push_back(c);
				}
				break;

			default:
				break;
			}
		}

		return cnt > 0;
	}


	bool Answer::read_status(net::Socket& socket)
	{
		tools::obfstring line;

		// We assume that the server did not understood our request
		_status_code = default_code;
		_reason_phrase = default_reason;

		// Read the status line and return an error code if the server has closed 
		// the connection before sending a valid response.
		if (!read_line(socket, MAX_HEADER_SIZE, line))
			return false;

		// The answer should have the following syntax :  HTTP-Version SP Status-Code SP Reason-Phrase
		// See https://www.w3.org/Protocols/rfc2616/rfc2616-sec6.html 
		std::vector<std::string> parts;
		int count = tools::split(line.uncrypt(), ' ', parts);
		if (count >= 1)
			_version = parts[0];
		if (count >= 2)
			_status_code = parts[1];
		if (count >= 3) {
			// Merge all others parts to rebuild the reason phrase
			_reason_phrase = parts[2];
			for (int idx = 3; idx < count; idx++) {
				_reason_phrase.append(" ");
				_reason_phrase.append(parts[idx]);
			}
		}

		return true;
	}


	bool Answer::read_body(net::Socket& socket, size_t size)
	{
		int rc = 0;

		// read by block of 1024 bytes
		unsigned char buffer[1024];
		size_t cnt = size;

		do {
			rc = socket.read(buffer, min(cnt, sizeof(buffer)));
			if (rc < 0)
				throw new mbed_error(rc);
			if (rc == 0) 
				return false;

			_body.append(buffer, rc);
			cnt = cnt - rc;

		} while (cnt > 0);

		return true;
	}


	bool Answer::recv(net::Socket& socket)
	{
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
		if (!read_status(socket))
			return false;

		// Read message-header. We limit explicitly each header to 8K Bytes. 
		do {
			if (!read_line(socket, MAX_HEADER_SIZE, line))
				return false;

			std::string::size_type pos = line.find(':');
			if (pos > 0 && pos != std::string::npos) {
				const std::string field_name = line.substr(0, pos).uncrypt();
				const tools::obfstring field_value = tools::trim(line.substr(pos + 1, std::string::npos));

				if (tools::iequal(field_name, "Set-Cookie")) {
					// it should be a cookie definition
					try {
						const Cookie cookie(field_value);
						_cookies.set(cookie.get_name(), cookie);
					}
					catch (CookieError e) {
					}
				}
				else {
					// it is a header
					_headers.set(field_name, field_value.uncrypt());
				}
			}
		} while (line.size() > 0);

		// Get the encoding scheme
		std::string encoding;
		if (_headers.get("Transfer-Encoding", encoding))
			encoding = tools::lower(tools::trim(encoding));

		// Read the body.
		// Are we using the chunked-style of transfer encoding?
		if (encoding.compare("chunked") == 0) {
			// chucked message
			long chunck_size = 0;

			do {
				// read chunk size
				if (!read_line(socket, 8 * 1024, line))
					return false;

				// decode chunk size
				if (!tools::str2num(line.uncrypt(), 16, 0, MAX_CHUNCK_SIZE, chunck_size)) {
					// Chunk size error
					throw new http_error(http_error::CHUNK_SIZE);
				}

				if (chunck_size > 0) {
					if (!read_body(socket, chunck_size))
						return false;
				}

				// skip eol
				if (!read_line(socket, 8 * 1024, line))
					return false;

			} while (chunck_size > 0 && _body.size() < MAX_BODY_SIZE);
		}
		else {
			// read content length 
			std::string length;

			if (_headers.get("Content-Length", length)) {
				long size = 0;

				if (!tools::str2num(length, 10, 0, MAX_BODY_SIZE, size)) {
					// Invalid body size, too large
					throw new http_error(http_error::BODY_SIZE);
				}

				if (size > 0) {
					// define the capacity of the buffer
					_body.resize(size);

					// read the whole body
					if (!read_body(socket, size))
						return false;
				}
			}
		}

		return true;
	}


	const std::string& Answer::get_version() const
	{
		return _version;
	}


	const int Answer::get_status_code() const
	{
		int code;
		return tools::str2i(_status_code, code) ? code : 400;
	}


	const std::string& Answer::get_reason_phrase() const
	{
		return _reason_phrase;
	}
}