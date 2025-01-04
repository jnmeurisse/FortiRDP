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


	Request& Request::set_body(const byte* data, size_t size)
	{
		_body.clear();
		_body.append(data, size);

		return *this;
	}


	void Request::send(Socket& socket, Timer& timer)
	{
		DEBUG_ENTER(_logger, "Request", "send");

		ByteBuffer buffer(1024);

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
		obfstring cookie_header{ _cookies.to_header() };
		if (cookie_header.size() > 0) {
			// cookies are appended decrypted in the buffer
			buffer
				.append("Cookie: ")
				.append(cookie_header)
				.append("\r\n");
		}
		buffer.append("\r\n");

		// Send headers to the web server
		if (_logger->is_trace_enabled())
			_logger->trace(
				"... %x       Request::send : write headers",
				std::addressof(this)
			);
		write_buffer(socket, buffer.cbegin(), buffer.size(), timer);

		// Erase sensitive data
		buffer.clear();

		if (_body.size() > 0) {
			// send the body to the web server
			if (_logger->is_trace_enabled())
				_logger->trace(
					"... %x       Request::send : write body",
					std::addressof(this)
				);
			write_buffer(socket, _body.cbegin(), _body.size(), timer);
		}

		// flush the output buffer
		if (_logger->is_trace_enabled())
			_logger->trace(
				"... %x       Request::send : flush",
				std::addressof(this)
			);

		//TODO : is flush needed ?
		//mbed_err rc = socket.flush();
		//if (!rc)
		//	throw tools::mbed_error(rc);

		return;
	}


	void Request::write_buffer(Socket& socket, const byte* buffer, size_t len, Timer& timer)
	{
		const netctx_snd_status snd_status = socket.write(buffer, len, timer);

		if (snd_status.status_code == NETCTX_SND_ERROR) {
			// send failed or timed out
			throw mbed_error(snd_status.errnum);
		} 
		
		//TODO: Do we have to do something here ?
	}

}
