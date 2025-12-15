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
#include <iosfwd>
#include <iostream>
#include <sstream>
#include <codecvt>
#include "tools/StringMap.h"
#include <algorithm>


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

	const int DEFAULT_KEEP_ALIVE_TIMEOUT = 60;
	const int DEFAULT_CONNECT_TIMEOUT = 10;
	const int DEFAULT_SND_TIMEOUT = 10;
	const int DEFAULT_RCV_TIMEOUT = 10;

	HttpsClient::HttpsClient(const net::Endpoint& ep, const net::TlsConfig& config) :
		TlsSocket(config),
		_host_ep(ep),
		_keepalive_timeout(DEFAULT_KEEP_ALIVE_TIMEOUT),
		_keepalive_timer(),
		_max_requests(100),
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

		_keepalive_timer.start(10000);
		_request_count = 0;

		tools::Timer connect_timer{ _connect_timeout };

		const mbed_err rc = TlsSocket::connect(_host_ep, connect_timer);
		_logger->debug(
			"... %x       %s::%s - TlsSocket::call returns rc=%d",
			(uintptr_t)this,
			__class__,
			__func__,
			rc);

		if (rc < 0)
			throw mbed_error(rc);

		const tls_handshake_status status = handshake(connect_timer);
		if (status.status_code != hdk_status_code::SSLCTX_HDK_OK)
			throw mbed_error(status.rc);
	}


	void HttpsClient::disconnect()
	{
		DEBUG_ENTER(_logger);

		TlsSocket::shutdown();
	}


	void HttpsClient::send_request(Request& request)
	{
		DEBUG_ENTER(_logger);

		if (_logger->is_trace_enabled())
			_logger->trace("... %x       %s::%s send url=%s count=%d max=%d timeout=%d",
				(uintptr_t)this,
				__class__,
				__func__,
				request.url().to_string(false).c_str(),
				_request_count,
				_max_requests,
				_keepalive_timeout);

		request.send(*this, tools::Timer{ _send_timeout });

		// Update the number of requests and restart the keep alive timer
		_request_count++;
		_keepalive_timer.start(_keepalive_timeout * 1000);

		if (_logger->is_trace_enabled())
			_logger->trace("... %x leave %s::%s count=%d max=%d timeout=%d",
				(uintptr_t)this,
				__class__,
				__func__,
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
		const int rc = answer.recv(*this, tools::Timer{ _receive_timeout });

		_logger->debug(
			"... %x       %s::%s : rc=%d",
			(uintptr_t)this,
			__class__,
			__func__,
			rc
		);
		
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

		if (_logger->is_trace_enabled())
			_logger->trace("... %x leave %s::%s rc=%d max=%d timeout=%d",
				(uintptr_t)this,
				__class__,
				__func__,
				rc,
				_max_requests,
				_keepalive_timeout
			);

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
				escaped << (char)c;
			}
			else {
				escaped << "%";
				escaped << hexstr[(c & 0xF0) >> 4];
				escaped << hexstr[(c & 0x0F)];
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
				const int c1 = (int)str.at(i + 1);
				const int c2 = (int)str.at(i + 2);

				if (std::isxdigit(c1) && std::isxdigit(c2)) {
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

	const char* HttpsClient::__class__ = "HttpsClient";

}
