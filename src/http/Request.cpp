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
	using namespace net;

	/* Initialization of common HTTP Verbs */
	const std::string Request::GET_VERB = "GET";
	const std::string Request::POST_VERB = "POST";
	const std::string Request::PUT_VERB = "PUT";
	const std::string Request::DELETE_VERB = "DELETE";
	const std::string Request::HEAD_VERB = "HEAD";
	const std::string Request::OPTIONS_VERB = "OPTIONS";
	const std::string Request::TRACE_VERB = "TRACE";


	Request::Request(const std::string& verb, const Url& url, const Cookies& cookie_jar) :
		_logger(Logger::get_logger()),
		_verb(verb),
		_url(url),
		_cookies(cookie_jar),
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


	void Request::send(net::TcpSocket& socket, const tools::Timer& timer)
	{
		DEBUG_ENTER(_logger);

		if (_logger->is_trace_enabled())
			_logger->trace("... %x enter %s::%s timeout=%lu",
				(uintptr_t)this,
				__class__,
				__func__,
				timer.remaining_time()
			);

		tools::ByteBuffer buffer(1024);

		if (_body.size() > 0) {
			_headers.set("Content-Length", _body.size());
		}

		buffer
			.append(_verb).append(' ')
			.append(_url.to_string(true)).append(' ')
			.append("HTTP/1.1")
			.append("\r\n");

		// Append all headers.
		_headers.write(buffer);

		// Add cookies, cookies are still obfuscated at this stage
		tools::obfstring cookie_header{ _cookies.to_header(_url) };
		if (cookie_header.size() > 0) {
			// Cookies are appended decrypted in the buffer.
			buffer
				.append("Cookie: ")
				.append(cookie_header)
				.append("\r\n");
		}
		buffer.append("\r\n");

		// Send headers to the web server.
		if (_logger->is_trace_enabled())
			_logger->trace(
				"... %x       %s::%s : write headers",
				(uintptr_t)this,
				__class__,
				__func__
			);
		write_buffer(socket, buffer.cbegin(), buffer.size(), timer);

		// Erase sensitive data.
		buffer.clear();

		if (_body.size() > 0) {
			// Send the body to the web server.
			if (_logger->is_trace_enabled())
				_logger->trace(
					"... %x       %s::%s : write body",
					(uintptr_t)this,
					__class__,
					__func__
				);
			write_buffer(socket, _body.cbegin(), _body.size(), timer);
		}

		return;
	}


	void Request::write_buffer(net::TcpSocket& socket, const byte* buffer, size_t len, const Timer& timer)
	{
		const snd_status status{ socket.write(buffer, len, timer) };

		if (_logger->is_trace_enabled())
			_logger->trace(
				"... %x       %s::%s : write buffer rc = %d",
				(uintptr_t)this,
				__class__,
				__func__,
				status.rc
			);

		if (status.code == snd_status_code::NETCTX_SND_ERROR || status.code == snd_status_code::NETCTX_SND_RETRY) {
			// Send failed or timed out.
			throw mbed_error(status.rc);
		}
	}

	const char* Request::__class__ = "Request";

}
