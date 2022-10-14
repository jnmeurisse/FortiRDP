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

#include "net/Socket.h"

#include "tools/Logger.h"
#include "tools/ByteBuffer.h"
#include "tools/ErrUtil.h"


namespace http {
	using namespace tools;

	class Request final
	{
	public:
		/* Allocates a HTTP request message
		 *
		 * @param verb The HTTP verb
		 * @param url  The url
		 * @param cookies the cookies store
		*/
		explicit Request(const std::string& verb, const Url& url, const Cookies& cookies);

		/* Clears the request */
		void clear();

		/* Sets the body
		 * @param data The buffer to send with this request
		 * @param size The size of the specified data buffer
		*/
		Request& set_body(const unsigned char* data, size_t size);

		/* Returns the request's url
		*/
		inline const Url& url() const noexcept { return _url; }

		/* The request's headers
		*/
		inline Headers& headers() { return _headers; }
		
		/* The request's cookies
		*/
		inline const Cookies& cookies() const noexcept { return _cookies; }

		/* Sends this request to the server
		 *
		 * @param socket The socket connected to the server
		 * Throws an mbed_error in case of failure.
		 */
		void send(net::Socket& socket);

		/* Most common HTTP verbs */
		static const std::string GET_VERB;
		static const std::string POST_VERB;
		static const std::string PUT_VERB;
		static const std::string DELETE_VERB;
		static const std::string HEAD_VERB;
		static const std::string OPTIONS_VERB;
		static const std::string TRACE_VERB;

	private:
		// a reference to the application logger
		tools::Logger* const _logger;

		// Fixed components of the HTTP request
		const std::string _verb;
		const Url _url;
		const Cookies _cookies;

		// Dynamic components of the HTTP request
		Headers _headers;
		tools::ByteBuffer _body;
	};

}