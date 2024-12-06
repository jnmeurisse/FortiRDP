/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Request.h"

#include "tools/ErrUtil.h"


namespace http {

	using namespace tools;

	/* Initialization of common HTTP Verbs */
	const std::string Request::GET_VERB = "GET";
	const std::string Request::POST_VERB = "POST";
	const std::string Request::PUT_VERB = "PUT";
	const std::string Request::DELETE_VERB = "DELETE";
	const std::string Request::HEAD_VERB = "HEAD";
	const std::string Request::OPTIONS_VERB = "OPTIONS";
	const std::string Request::TRACE_VERB = "TRACE";


	Request::Request(const std::string& verb, const Url& url, const Cookies& cookies) :
		_logger(Logger::get_logger()),
		_verb(verb),
		_url(url),
		_cookies(cookies),
		_headers(),
		_body(2048)
	{
	}


	void Request::clear()
	{
		_headers.serase();
		_body.clear();
	}


	Request& Request::set_body(const unsigned char* data, size_t size)
	{
		_body.clear();
		_body.append(data, size);

		return *this;
	}


	void Request::send(net::Socket& _socket)
	{
		DEBUG_ENTER(_logger, "Request", "send");

		tools::ByteBuffer buffer(1024);
		int rc;

		if (_body.size() > 0) {
			// add a content-length header if there is a body.
			_headers.set("Content-Length", _body.size());
		}

		buffer
			.append(_verb).append(" ")
			.append(_url.to_string(true)).append(" ")
			.append("HTTP/1.1")
			.append("\r\n");

		// Append all headers
		_headers.write(buffer);

		// add cookies, cookies are still obfuscated at this stage
		tools::obfstring cookie_header{ _cookies.to_header() };
		if (cookie_header.size() > 0) {
			// cookies are appended decrypted in the buffer
			buffer
				.append("Cookie: ")
				.append(cookie_header)
				.append("\r\n");
		}
		buffer.append("\r\n");

		// Send headers to the web server
		rc = _socket.write(buffer.cbegin(), buffer.size());
		if (_logger->is_trace_enabled())
			_logger->trace("... %x       Request::send : write headers rc = %d", (uintptr_t)this, rc);

		if (rc < 0)
			throw mbed_error(rc);

		// Erase sensitive data
		buffer.clear();

		if (_body.size() > 0) {
			// send the body to the web server
			rc = _socket.write(_body.cbegin(), _body.size());
			if (_logger->is_trace_enabled())
				_logger->trace("... %x       Request::send : write body rc = %d", (uintptr_t)this, rc);

			if (rc < 0)
				throw mbed_error(rc);
		}

		// flush the output buffer
		rc = _socket.flush();
		if (_logger->is_trace_enabled())
			_logger->trace("... %x       Request::send : flush rc = %d", (uintptr_t)this, rc);

		if (rc < 0)
			throw mbed_error(rc);

		return;
	}

}
