/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include "http/Headers.h"
#include "http/Cookies.h"
#include "http/Url.h"
#include "net/TcpSocket.h"
#include "tools/Logger.h"
#include "tools/ByteBuffer.h"
#include "tools/Timer.h"


namespace http {

	using namespace tools;
	using namespace net;

	class Request final
	{
	public:
		/**
		 * Allocates and initializes an HTTP request message.
		 *
		 * This constructor creates a new HTTP request with the specified HTTP verb, URL,
		 * and cookie jar.  It is used to set up the components of an HTTP request message,
		 * including the verb (e.g., GET, POST), the URL to which the request will be sent,
		 * and any cookies that should be included in the request.
		 *
		 * @param verb The HTTP verb that defines the action to be performed.
		 * @param url The URL to which the HTTP request will be made.
		 * @param cookies The cookie jar containing cookies to be included in the request header.
		 */
		explicit Request(const std::string& verb, const Url& url, const Cookies& cookie_jar);

		/**
		 * Clears the request.
		 *
		 *  This function erases all headers and body.
		*/
		void clear();

		/**
		 * Sets the body of the HTTP request.
		 *
		 * This function sets the body content of the HTTP request message. It allows
		 * us to include raw data to be sent with the request, such as form data, or
		 * binary data. The provided data buffer and its size are copied into the
		 * request body.
		 *
		 * @param data The buffer containing the data to be sent with the HTTP request.
		 * @param size The size (in bytes) of the `data` buffer. This specifies the
		 *             amount of data to be included in the body of the request.
		 *
		 * @return A reference to the current `Request` object, allowing for method chaining.
		 */
		Request& set_body(const unsigned char* data, size_t size);

		/**
		 * Returns the request's url.
		*/
		inline const Url& url() const noexcept { return _url; }

		/**
		 * Returns the request's headers
		*/
		inline Headers& headers() { return _headers; }
		
		/**
		 * Sends this HTTP request to the server.
		 *
		 * This function transmits the HTTP request, including its method, headers, and body
		 * (if any), to the server using the provided socket connection. It ensures that the
		 * request is properly sent over the network, and will throw an exception if any error
		 * occurs during the transmission.
		 *
		 * @param socket The socket connected to the server through which the HTTP request
		 *               will be sent.
		 * @param timer A timer that specifies the maximum time allowed for the send operation.
		 *              If the operation takes longer than the specified time, it will be
		 *              canceled.
		 *
		 * @throws mbed_error If an error occurs while sending the request, such as network
		 *                    issues or socket failure.
		 */
		void send(net::TcpSocket& socket, tools::Timer& timer);

		/* Most common HTTP verbs */
		static const std::string GET_VERB;
		static const std::string POST_VERB;
		static const std::string PUT_VERB;
		static const std::string DELETE_VERB;
		static const std::string HEAD_VERB;
		static const std::string OPTIONS_VERB;
		static const std::string TRACE_VERB;

	private:
		// A reference to the application logger.
		tools::Logger* const _logger;

		// All cookies (a reference to the cookie jar).
		const Cookies& _cookies;

		// Fixed components of the HTTP request.
		const std::string _verb;
		const Url _url;

		// Dynamic components of the HTTP request.
		Headers _headers;
		tools::ByteBuffer _body;

		void write_buffer(net::TcpSocket& socket, const byte* buffer, size_t len, Timer& timer);
	};

}
