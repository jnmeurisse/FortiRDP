/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include "http/Request.h"
#include "http/Answer.h"
#include "http/Url.h"
#include "net/Endpoint.h"
#include "net/TlsSocket.h"
#include "util/Timer.h"


namespace http {

	class HttpsClient : public net::TlsSocket
	{
	public:
		explicit HttpsClient(const net::Endpoint& ep, const net::TlsConfig& config);
		~HttpsClient() override;

		/**
		 * Sets the timeout values for an HTTPS transaction.
		 *
		 * This function sets the timeouts for connection, sending, and receiving data in
		 * an HTTPS transaction. It must be called before establishing a connection with
		 * the server. If the HttpsClient is already connected, the function returns false,
		 * and the timeout values remain unchanged.
		 *
		 * @param connect_timeout The timeout value (in milliseconds) for establishing
		 *                        the connection with the server.
		 * @param send_timeout The timeout value (in milliseconds) for sending data to
		 *                     the server.
		 * @param receive_timeout The timeout value (in milliseconds) for receiving
		 *                        data from the server.
		 *
		 * @return `true` if the timeouts are successfully set; `false` if the client is
		 *         already connected and the timeouts cannot be changed.
		 */
		bool set_timeouts(uint32_t connect_timeout, uint32_t send_timeout, uint32_t receive_timeout);
		
		/**
		 * Returns the endpoint to which this client is connected.
		*/
		inline const net::Endpoint& host() const noexcept { return _host_ep; }

		/**
		 * Connects this client to the endpoint.
		 *
		 * @throws mbed_error If a network, SSL, or other critical error occurs during the
		 *                    connection process.
		*/
		void connect();

		/**
		 * Disconnects this client from the endpoint.
		*/
		void disconnect();

		/**
		 * Sends a send_request to the server.
		 *
		 * This function sends an HTTP request to the server. The request is passed as
		 * an argument and transmitted over the active connection. It may throw an
		 * `mbed_error` if there are issues during the sending process, such as network
		 * or connection failures.
		 *
		 * @param request The HTTP request to be sent to the server.
		 *
		 * @throws mbed_error If an error occurs during the send_request transmission,
		 *                    such as network, socket-related, timeout issues.
		 */
		void send_request(Request& request);

		/**
		 * Receives and processes the HTTP response from the server.
		 *
		 * This function retrieves the response from the server and processes the answer.
		 * It clears any existing data in the provided `answer` object, receives the
		 * response, and checks for errors. If an error is encountered, an exception
		 * is thrown with a descriptive message.
		 *
		 * It also extracts the `Keep-Alive` header parameters, setting the connection
		 * timeout and maximum request count values accordingly.
		 *
		 * @param answer The `Answer` object that will hold the received response.
		 *
		 * @throws mbed_error If an error occurs while receiving the response
		 *                    such as network, socket-related, timeout issues.
		 * @throws http_error If an error occurs while receiving the response,
		 *                    such as invalid status line, version, or body.
		 */
		void recv_answer(Answer& answer);

		/**
		 * Encodes an utf8 string that can be used in a query part of a URL.
		*/
		static std::string encode_url(const std::string& str);

		/**
		 * Decodes a URL string into a utf8 string.
		*/
		static std::string decode_url(const std::string& str);

		/* Most useful status codes */
		static const int STATUS_OK;
		static const int STATUS_MOVED_PERMANENTLY;
		static const int STATUS_FOUND;
		static const int STATUS_SEE_OTHER;
		static const int STATUS_TEMPORARY_REDIRECT;
		static const int STATUS_UNAUTHORIZED;
		static const int STATUS_FORBIDDEN;

		/**
		 * Creates an Url from a path.
		*/
		http::Url make_url(const std::string& path) const;

		/**
		 * Creates an Url from a path and a query.
		*/
		http::Url make_url(const std::string& path, const std::string& query) const;

	protected:
		/**
		 * Determines whether the socket needs to be reconnected.
		 *
		 * Reconnection is required if the keep-alive timer has expired or if the
		 * number of requests exceeds the server-defined maximum. If the server does
		 * not provide these parameters, default values are used: 60 seconds for the
		 * keep-alive timer and 100 requests for the request limit.
		 */
		bool is_reconnection_required() const;

	private:
		// The class name
		static const char* __class__;

		// The endpoint.
		const net::Endpoint _host_ep;

		// Connection keep alive timer.
		utl::Timer _keepalive_timer;

		// Maximum number of requests this client can issue during a session.
		int _max_requests;

		// Maximum amount of time the session can be idle (in seconds).
		int _keepalive_timeout;

		// All configured timeout values.
		uint32_t _connect_timeout;
		uint32_t _send_timeout;
		uint32_t _receive_timeout;

		// Number of requests sent since last connection.
		int _request_count;
	};


	using HttpsClientPtr = std::unique_ptr<HttpsClient>;
}
