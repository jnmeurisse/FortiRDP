/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <cctype>
#include <iostream>
#include <sstream>
#include <codecvt>

#include "http/HttpsClient.h"
#include "tools/StringMap.h"
#include "tools/StrUtil.h"


namespace http {

	using namespace tools;

	// HTTP status code
	const int HttpsClient::STATUS_OK = 200;
	const int HttpsClient::STATUS_MOVED_PERMANENTLY = 301;
	const int HttpsClient::STATUS_FOUND = 302;
	const int HttpsClient::STATUS_SEE_OTHER = 303;
	const int HttpsClient::STATUS_TEMPORARY_REDIRECT = 307;
	const int HttpsClient::STATUS_UNAUTHORIZED = 401;
	const int HttpsClient::STATUS_FORBIDDEN = 403;


	HttpsClient::HttpsClient(const net::Endpoint& ep) :
		TlsSocket(),
		_host_ep(ep),
		_keepalive_timeout(10),
		_keepalive_timer(_keepalive_timeout * 1000),
		_max_requests(100),
		_request_count(0)
	{
		DEBUG_CTOR(_logger, "HttpsClient");
	}


	HttpsClient::~HttpsClient()
	{
		DEBUG_DTOR(_logger, "HttpsClient");
	}


	bool HttpsClient::must_reconnect() const
	{
		return (!connected())
			|| _keepalive_timer.elapsed()
			|| (_request_count >= _max_requests);
	}


	void HttpsClient::connect()
	{
		DEBUG_ENTER(_logger, "HttpsClient", "connect");

		_keepalive_timer.start(10000);
		_request_count = 0;

		const mbed_err rc = TlsSocket::connect(_host_ep);
		_logger->debug("... %x       HttpsClient::connect : rc = %d", (uintptr_t)this, rc);
		if (rc < 0) {
			throw mbed_error(rc);
		}
	}

	void HttpsClient::disconnect()
	{
		DEBUG_ENTER(_logger, "HttpsClient", "disconnect");

		Socket::close();
	}


	void  HttpsClient::send_request(Request& request)
	{
		DEBUG_ENTER(_logger, "HttpsClient", "send_request");

		if (_logger->is_trace_enabled())
			_logger->trace("... %x enter HttpsClient::send_request url=%s count=%d max=%d timeout=%d",
				(uintptr_t)this,
				request.url().to_string(false).c_str(),
				_request_count,
				_max_requests,
				_keepalive_timeout);

		request.send(*this);

		// update the number of requests and restart the keep alive timer
		_request_count++;
		_keepalive_timer.start(_keepalive_timeout * 1000);

		if (_logger->is_trace_enabled())
			_logger->trace("... %x leave HttpsClient::send_request count=%d max=%d timeout=%d",
				(uintptr_t)this,
				_request_count,
				_max_requests,
				_keepalive_timeout);

		return;
	}


	void HttpsClient::recv_answer(Answer& answer)
	{
		DEBUG_ENTER(_logger, "HttpsClient", "recv_answer");

		// Make sure the answer holder is empty
		answer.clear();

		const int rc = answer.recv(*this);
		_logger->debug("... %x       HttpsClient::recv_answer : rc = %d", (uintptr_t)this, rc);
		
		if (rc != Answer::ERR_NONE) {
			std::string message;
			switch (rc) {
			case Answer::ERR_INVALID_STATUS_LINE:;
				message = "Invalid HTTP Status line"; break;
			case Answer::ERR_INVALID_VERSION:
				message = "Invalid HTTP version"; break;
			case Answer::ERR_INVALID_STATUS_CODE:
				message = "Invalid HTTP status code"; break;
			case Answer::ERR_INVALID_HEADER:
				message = "Invalid HTTP header"; break;
			case Answer::ERR_CHUNK_SIZE:
				message = "Invalid HTTP chunk size"; break;
			case Answer::ERR_BODY_SIZE:
				message = "Invalid HTTP body size"; break;
			case Answer::ERR_CONTENT_ENCODING:;
				message = "Unsupported HTTP content encoding"; break;
			case Answer::ERR_TRANSFER_ENCODING:
				message = "Unsupported HTTP transfer encoding"; break;
			case Answer::ERR_BODY:
				message = "Invalid HTTP body"; break;
			default:
				message = "Unknown error in HttpsClient"; break;
			}

			throw httpcli_error(message);
		}

		int timeout = 60;
		int max_requests = 100;
		std::string keep_alive;

		if (answer.headers().get("Keep-Alive", keep_alive)) {
			StringMap keep_alive_params{ keep_alive, ',' };

			keep_alive_params.get_int("timeout", timeout);
			keep_alive_params.get_int("max", max_requests);
		}

		_max_requests = max(0, max_requests);
		_keepalive_timeout = max(0, timeout);

		if (_logger->is_trace_enabled())
			_logger->trace("... %x leave HttpsClient::recv_answer rc=%d max=%d timeout=%d",
				(uintptr_t)this,
				rc,
				_max_requests,
				_keepalive_timeout);

		return;
	}


	std::string HttpsClient::url_encode(const std::wstring& str)
	{
		static const char hexstr[] = "0123456789abcdef";

		// convert input string to utf-8
		std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
		const std::string utf8_str{ utf8_conv.to_bytes(str) };

		// allocate output buffer
		std::stringstream escaped;

		for (unsigned int i = 0; i < utf8_str.length(); i++) {
			const char c = utf8_str.at(i);
			if ((48 <= c && c <= 57) ||   // 0-9
				(65 <= c && c <= 90) ||   // ABC...XYZ
				(97 <= c && c <= 122) ||  // abc...xyz
				(c == '~' || c == '-' || c == '_' || c == '.')) {

				escaped << (char) c;

			} else if (c <= 0xFF) {
				escaped << "%";
				escaped << hexstr[(c & 0xF0) >> 4];
				escaped << hexstr[(c & 0x0F)];
			}
		}

		return escaped.str();
	}

	std::string HttpsClient::url_decode(const std::string& str)
	{
		std::string unescaped;
		unsigned int i = 0;

		while (i < str.length() - 2) {
			const char c = str.at(i);

			if (c == '%') {
				const int c1 = (int)str.at(i + 1);
				const int c2 = (int)str.at(i + 2);

				if (isxdigit(c1) && isxdigit(c2)) {
					const char dc = ((c1 - '0') << 4) + (c2 - '0');
					unescaped.append(&dc, 1);
				}

				i += 3;

			}
			else {
				unescaped.append(&c, 1);
				i++;
			}
		}

		while (i < str.length()) {
			unescaped.append(&str.at(i), 1);
			i++;
		}

		return unescaped;
	}


	http::Url HttpsClient::make_url(const std::string& path) const
	{
		return http::Url(
			"https",
			_host_ep.to_string(),
			path,
			"");
	}

	http::Url HttpsClient::make_url(const std::string& path, const std::string& query) const
	{
		return http::Url(
			"https",
			_host_ep.to_string(),
			path,
			query);
	}
}
