/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "HttpsClient.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include "util/StringMap.h"
#include "net/TlsContext.h"


namespace http {
	using namespace utl;


	// HTTP status code
	const int HttpsClient::STATUS_OK = 200;
	const int HttpsClient::STATUS_MOVED_PERMANENTLY = 301;
	const int HttpsClient::STATUS_FOUND = 302;
	const int HttpsClient::STATUS_SEE_OTHER = 303;
	const int HttpsClient::STATUS_TEMPORARY_REDIRECT = 307;
	const int HttpsClient::STATUS_UNAUTHORIZED = 401;
	const int HttpsClient::STATUS_FORBIDDEN = 403;

	const int DEFAULT_KEEP_ALIVE_TIMEOUT = 60;
	const int DEFAULT_CONNECT_TIMEOUT = 10;
	const int DEFAULT_SND_TIMEOUT = 10;
	const int DEFAULT_RCV_TIMEOUT = 10;

	HttpsClient::HttpsClient(const net::Endpoint& ep, const net::TlsConfig& config) :
		TlsSocket(config),
		_host_ep(ep),
		_keepalive_timer(DEFAULT_KEEP_ALIVE_TIMEOUT * 1000),
		_max_requests(100),
		_keepalive_timeout(DEFAULT_KEEP_ALIVE_TIMEOUT),
		_connect_timeout(DEFAULT_CONNECT_TIMEOUT * 1000),
		_send_timeout(DEFAULT_SND_TIMEOUT * 1000),
		_receive_timeout(DEFAULT_RCV_TIMEOUT * 1000),
		_request_count(0)
	{
		DEBUG_CTOR(_logger);
	}


	HttpsClient::~HttpsClient()
	{
		DEBUG_DTOR(_logger);
	}


	bool HttpsClient::set_timeouts(uint32_t connect_timeout, uint32_t send_timeout, uint32_t receive_timeout)
	{
		DEBUG_DTOR(_logger);
		bool rc = false;

		if (!TlsSocket::is_connected()) {
			_connect_timeout = connect_timeout;
			_send_timeout = send_timeout;
			_receive_timeout = receive_timeout;

			rc = true;
		}

		return rc;
	}


	bool HttpsClient::is_reconnection_required() const
	{
		return (!TlsSocket::is_connected())
			|| _keepalive_timer.is_elapsed()
			|| (_request_count >= _max_requests);
	}


	void HttpsClient::connect()
	{
		DEBUG_ENTER(_logger);

		_keepalive_timer.start(_keepalive_timeout * 1000);
		_request_count = 0;

		utl::Timer connect_timer{ _connect_timeout };

		const mbed_err rc = TlsSocket::connect(_host_ep, connect_timer);
		if (rc < 0)
			throw mbed_error(rc);

		const net::tls_handshake_status status = handshake(connect_timer);
		if (status.status_code != net::hdk_status_code::SSLCTX_HDK_OK)
			throw mbed_error(status.rc);
	}


	void HttpsClient::disconnect()
	{
		DEBUG_ENTER(_logger);

		TlsSocket::shutdown();
	}


	void HttpsClient::send_request(Request& request)
	{
		DEBUG_ENTER_FMT(_logger, "url=%s count=%d max=%d timeout=%d",
			request.url().to_string(false).c_str(),
			_request_count,
			_max_requests,
			_keepalive_timeout
		);

		request.send(*this, utl::Timer{ _send_timeout });

		// Update the number of requests and restart the keep alive timer
		_request_count++;
		_keepalive_timer.start(_keepalive_timeout * 1000);

		LOG_TRACE(_logger, "request count=%d keepalive max=%d timeout=%d",
			_request_count,
			_max_requests,
			_keepalive_timeout
		);

		return;
	}


	void HttpsClient::recv_answer(Answer& answer)
	{
		DEBUG_ENTER(_logger);

		// Make sure the answer buffer is empty.
		answer.clear();

		// Receive the answer from the server.
		answer.recv(*this, utl::Timer{ _receive_timeout });
		LOG_DEBUG(_logger, "status=%3.3d body size=%zu",
			answer.get_status_code(),
			answer.body().size()
		);

		// Update the keep alive timeout and max requests.
		int timeout = DEFAULT_KEEP_ALIVE_TIMEOUT;
		int max_requests = 100;
		std::string keep_alive;

		if (answer.headers().get("Keep-Alive", keep_alive)) {
			StringMap keep_alive_params{ keep_alive, ',' };

			keep_alive_params.get_int("timeout", timeout);
			keep_alive_params.get_int("max", max_requests);
		}

		_max_requests = std::max(0, max_requests);
		_keepalive_timeout = std::max(0, timeout);

		LOG_TRACE(_logger, "keep-alive max=%d timeout=%d", _max_requests, _keepalive_timeout);

		return;
	}


	std::string HttpsClient::encode_url(const std::string& str)
	{
		static const char hexstr[] = "0123456789abcdef";

		// Allocate output buffer
		std::stringstream escaped;

		for (unsigned char c : str) {
			// RFC 3986 unreserved characters
			if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
				escaped << static_cast<char>(c);
			}
			else {
				escaped << "%";
				escaped << hexstr[(c & 0xF0) >> 4];
				escaped << hexstr[c & 0x0F];
			}
		}

		return escaped.str();
	}


	std::string HttpsClient::decode_url(const std::string& str)
	{
		std::string unescaped;
		unescaped.reserve(str.size());
		size_t i = 0;

		while (i < str.length() - 2) {
			const char c = str.at(i);

			if (c == '%') {
				const char c1 = str.at(i + 1);
				const char c2 = str.at(i + 2);

				if (std::isxdigit(c1) && std::isxdigit(c2)) {
					const char dc = ((c1 - '0') << 4) + (c2 - '0') & 0x7F;
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

	const char* HttpsClient::__class__ = "HttpsClient";
}
