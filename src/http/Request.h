/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <cstdint>
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
	using namespace net;

	class Request final
	{
	public:
		using byte = uint8_t;

		/* Allocates a HTTP request message
		 *
		 * @param verb The HTTP verb
		 * @param url  The url
		 * @param cookies the cookies store
		*/
		explicit Request(const std::string& verb, const Url& url, const Cookies& cookies, int timeout);

		/* Clears the request */
		void clear();

		/* Sets the request body
		 *
		 * @param data The buffer to send with this request
		 * @param size The size of the specified data buffer
		*/
		Request& set_body(const byte* data, size_t size);

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
		 * Throws an ossl_error in case of failure.
		 */
		void send(Socket& socket);

		/* Most common HTTP verbs */
		static const std::string GET_VERB;
		static const std::string POST_VERB;
		static const std::string PUT_VERB;
		static const std::string DELETE_VERB;
		static const std::string HEAD_VERB;
		static const std::string OPTIONS_VERB;
		static const std::string TRACE_VERB;

	private:
		// A reference to the application logger
		Logger* const _logger;

		// Fixed components of the HTTP request
		const std::string _verb;
		const Url _url;
		const Cookies _cookies;

		// Dynamic components of the HTTP request
		Headers _headers;
		tools::ByteBuffer _body;

		//
		const int _timeout;

		// 
		bool write(Socket& socket, const byte* buffer, size_t len, size_t &sbytes);

		bool write_buffer(Socket& socket, const byte* buffer, size_t len);
	};

}