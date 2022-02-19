/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#include "Request.h"

namespace http {

	/* Initialization of common HTTP Verbs */
	const std::string Request::GET_VERB = "GET";
	const std::string Request::POST_VERB = "POST";
	const std::string Request::PUT_VERB = "PUT";
	const std::string Request::DELETE_VERB = "DELETE";
	const std::string Request::HEAD_VERB = "HEAD";
	const std::string Request::OPTIONS_VERB = "OPTIONS";
	const std::string Request::TRACE_VERB = "TRACE";


	Request::Request(const std::string& verb, const std::string& url, const Cookies& cookies, int version) :
		_http_version(version),
		_verb(verb),
		_url(url),
		_cookies(cookies),
		_headers(),
		_body()
	{
	}


	Request::~Request()
	{
		clear();
	}


	void Request::clear()
	{
		_headers.clear();
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
		tools::ByteBuffer buffer(1024);
		int rc;

		if (_body.size() > 0) {
			// add a content-length header if there is a body.
			_headers.set("Content-Length", _body.size());
		}

		buffer
			.append(_verb).append(" ")
			.append(_url).append(" ")
			.append(_http_version == 0 ? "HTTP/1.0" : "HTTP/1.1")
			.append("\r\n");

		// Append all headers
		_headers.write(buffer);

		// add cookies, cookies are still obfuscated at this stage
		tools::obfstring cookie_header = _cookies.to_header();
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
		if (rc < 0)
			throw new mbed_error(rc);

		// Erase sensitive data
		buffer.clear();

		if (_body.size() > 0) {
			// send the body to the web server
			rc = _socket.write(_body.cbegin(), _body.size());
			if (rc < 0)
				throw new mbed_error(rc);
		}

		// flush the output buffer
		_socket.flush();

		return;
	}

}