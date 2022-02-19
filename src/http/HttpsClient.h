/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>

#include "http/Request.h"
#include "http/Answer.h"

#include "net/Endpoint.h"
#include "net/TlsSocket.h"

#include "tools/Timer.h"
#include "tools/Logger.h"
#include "tools/ErrUtil.h"


namespace http {

	using namespace tools;


	class HttpsClient : public net::TlsSocket
	{
	public:
		explicit HttpsClient(const net::Endpoint& ep);
		virtual ~HttpsClient();
		
		/* Returns the endpoint of this client
		*/
		inline const net::Endpoint& host() const { return _host_ep; }

		/* Returns true if we must reconnect the socket
		 *
		 * The client must reconnect the server if the keep alive timer
		 * has expired or if the number of requests has exceeded the maximum
		 * number of requests a client can send to the server. These two
		 * parameters are returned by the server. If not returned, we use default
		 * values (60 seconds and 100 requests)
		*/
		bool must_reconnect() const;

		/* Connects this client to the specified endpoint.
		 * Returns a negative value in an error has occurred
		 * (see mbedtls_net_connect)
		*/
		mbed_err connect();

		/* Disconnects this client from the specified endpoint
		*/
		void disconnect();

		/* Sends a request to the server.
		*/
		void send_request(Request& request);

		/* Receives a response from the server
		*  Returns true if an answer is received
		*/
		bool recv_answer(Answer& answer);

		/* Encodes a string to be used in a query part of a URL
		*/
		static std::string url_encode(const std::wstring& str);

		/* Decodes a URL string
		*/
		static std::string url_decode(const std::string& str);

		/* Most useful status code */
		static const int STATUS_OK;
		static const int STATUS_MOVED_PERMANENTLY;
		static const int STATUS_FOUND;
		static const int STATUS_SEE_OTHER;
		static const int STATUS_TEMPORARY_REDIRECT;
		static const int STATUS_UNAUTHORIZED;
		static const int STATUS_FORBIDDEN;

	private:
		// The https server endpoint
		const net::Endpoint _host_ep;

		// Connection keep alive timer
		tools::Timer _keepalive_timer;

		// Maximum number of requests this client can issue during a session
		int _max_requests;

		// Maximum amount of time the session can be idle (in seconds)
		int _keepalive_timeout;

		// Number of requests sent since last connection
		int _request_count;
	};

}