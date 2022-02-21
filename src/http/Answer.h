/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <string>
#include <vector>
#include <list>

#include "http/Headers.h"
#include "http/Cookies.h"

#include "net/Socket.h"

#include "tools/ErrUtil.h"
#include "tools/ObfuscatedString.h"


namespace http {
	using namespace tools;

	/**
	* Defines the HTTP answer. 
	* The message body and all headers are made available from this object.
	*/
	class Answer
	{
	public:
		/* Allocates an HTTP answer
		*/
		Answer();

		/* Clears this HTTP Answer
		*/
		void clear();

		/* Reads the response for the HTTP server and initialize this object.
		*
		* @param socket The socket connected to the server
		* @return false if the socket has been closed.
		*
		* In case of failure, the function may raise the following exceptions
		* mbed_error : mbed tls communication error
		* http_error : http protocol error
		*
		*/
		bool recv(net::Socket& socket);

		/* Returns the HTTP version
		*/
		const std::string& get_version() const;

		/* Returns the HTTP status code
		*/
		const int get_status_code() const;

		/* Returns the HTTP reason phrase
		*/
		const std::string& get_reason_phrase() const;

		/* Returns the headers collection.  The cookies are stored apart in  
		 * obfuscated strings
		*/
		inline const Headers& headers() const { return _headers; }

		/* Returns the cookies collection
		*/
		inline const Cookies& cookies() const { return _cookies; }

		/* Returns the body of the answer
		*/
		inline const tools::ByteBuffer& body() const { return _body; }

	private:
		// The HTTP version
		std::string _version;

		// The HTTP status code
		std::string _status_code;

		// The HTTP reason phrase
		std::string _reason_phrase;

		// All headers except cookies
		Headers _headers;

		// All cookies
		Cookies _cookies;

		// The body of the answer
		tools::ByteBuffer _body;

		static const int MAX_HEADER_SIZE = (8 * 1024);
		static const int MAX_BODY_SIZE = (32 * 1014 * 1024);
		static const int MAX_CHUNCK_SIZE = (2 * 1014 * 1024);

		/* Reads a single character.
		*
		* @param socket The socket connected to the server
		* @return false if the socket has been closed.
		*
		* In case of failure, the function may raise mbed_error.
		*/
		bool read_char(net::Socket& socket, char& c);

		/* Reads a string until \r\n is found. The \r\n are not included
		 * in the line.
		 *
		 * @param socket The socket connected to the server
		 * @param maxlen The maximum number of bytes this client accept when reading
		 *               the line. If the effective value of this parameter is 0,
		 *               the functions does not check for a limit.
		 * @param line The string read from the socket
		 * @return false if the socket has been closed.
		 *
		 * In case of failure, the function may raise mbed_error.
		 *
		*/
		bool read_line(net::Socket& socket, int maxlen, tools::obfstring& line);

		/* Reads the HTTP status response
		*
		* @param socket The socket connected to the server
		* @return false if the socket has been closed.
		*
		* In case of failure, the function may raise mbed_error.
		*/
		bool read_status(net::Socket& socket);

		/* Reads the body
		*
		* @param socket The socket connected to the server
		* @param size The size of the body (in bytes)
		* @return false if the socket has been closed.
		*
		* In case of failure, the function may raise mbed_error.
		*/
		bool read_body(net::Socket& socket, size_t size);
	};
}

