/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Request.h"

#include "util/ErrUtil.h"
#include "util/Logger.h"


namespace http {
	using namespace utl;
	using namespace net;


	/* Initialization of common HTTP Verbs */
	const std::string Request::GET_VERB = "GET";
	const std::string Request::POST_VERB = "POST";
	const std::string Request::PUT_VERB = "PUT";
	const std::string Request::DELETE_VERB = "DELETE";
	const std::string Request::HEAD_VERB = "HEAD";
	const std::string Request::OPTIONS_VERB = "OPTIONS";
	const std::string Request::TRACE_VERB = "TRACE";


	Request::Request(const std::string& verb, const http::Url& url, const http::Cookies& cookie_jar) :
		_logger(Logger::get_logger()),
		_cookies(cookie_jar),
		_verb(verb),
		_url(url),
		_headers(),
		_body(2048)
	{
		DEBUG_CTOR(_logger);
	}


	Request::~Request()
	{
		DEBUG_DTOR(_logger);
	}


	void Request::clear()
	{
		DEBUG_ENTER(_logger);

		_headers.serase();
		_body.clear();
	}


	Request& Request::set_body(const unsigned char* data, size_t size)
	{
		DEBUG_ENTER_FMT(_logger, "buffer=0x%012Ix size=%zu",
			PTR_VAL(data),
			size
		);

		_body.clear();
		_body.append(data, size);

		return *this;
	}


	void Request::send(net::TcpSocket& socket, const utl::Timer& timer)
	{
		DEBUG_ENTER_FMT(_logger, "timeout=%lu", timer.remaining_time());

		if (!_body.empty()) {
			_headers.set("Content-Length", _body.size());
		}

		ByteBuffer buffer(1024);
		buffer
			.append(_verb).append(' ')
			.append(_url.to_string(true)).append(' ')
			.append("HTTP/1.1")
			.append("\r\n");

		// Append all headers.
		_headers.write(buffer);

		// Add cookies, cookies are still obfuscated at this stage
		utl::obfstring cookie_header{ _cookies.to_header(_url) };
		if (cookie_header.size() > 0) {
			// Cookies are appended decrypted in the buffer.
			buffer
				.append("Cookie: ")
				.append(cookie_header)
				.append("\r\n");
		}
		buffer.append("\r\n");

		// Send headers to the web server.
		LOG_DEBUG(_logger, "write headers");
		write_buffer(socket, buffer.cbegin(), buffer.size(), timer);

		// Erase sensitive data.
		buffer.clear();

		if (!_body.empty()) {
			// Send the body to the web server.
			LOG_DEBUG(_logger, "write body");
			write_buffer(socket, _body.cbegin(), _body.size(), timer);
		}

		return;
	}


	void Request::write_buffer(net::TcpSocket& socket, const unsigned char* buffer, size_t len, const utl::Timer& timer)
	{
		TRACE_ENTER_FMT(_logger, "buffer=0x%012Ix size=%zu timeout=%lu",
			PTR_VAL(buffer),
			len,
			timer.remaining_time()
		);

		const net::snd_status status{ socket.write(buffer, len, timer) };

		if (status.code == snd_status_code::NETCTX_SND_ERROR || status.code == snd_status_code::NETCTX_SND_RETRY) {
			// Send failed or timed out.
			throw mbed_error(status.rc);
		}
	}

	const char* Request::__class__ = "Request";
}
