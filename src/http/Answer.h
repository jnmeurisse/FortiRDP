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

#include "tools/Logger.h"
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
		// Error codes
		static const int ERR_NONE = 0;
		static const int ERR_INVALID_VERSION = 1;
		static const int ERR_INVALID_STATUS_LINE = 2;
		static const int ERR_INVALID_STATUS_CODE = 3;
		static const int ERR_INVALID_HEADER = 4;
		static const int ERR_CHUNK_SIZE = 5;
		static const int ERR_BODY_SIZE = 6;
		static const int ERR_CONTENT_ENCODING = 7;
		static const int ERR_TRANSFER_ENCODING = 8;
		static const int ERR_BODY = 9;

		/* Allocates an HTTP answer
		*/
		Answer();

		/* Clears this HTTP Answer
		*/
		void clear();

		/* Reads the response for the HTTP server and initialize this object.
		*
		* @param socket The socket connected to the server
		* @return ERR_NONE if answer is valid
		*         ERR_INVALID_VERSION: Invalid HTTP version
		*         ERR_INVALID_STATUS_LINE: Invalid HTTP status line
		*         ERR_INVALID_STATUS_CODE: Invalid HTTP status code
		*         ERR_INVALID_HEADER: Error while reading headers
		*         ERR_CHUNCK_SIZE: Unable to decode chunk size
		*         ERR_BODY_SIZE: Unable to decode body size;
		*         ERR_CONTENT_ENCODING: Unsupported content encoding
		*         ERR_TRANSFER_ENCODING: Unsupported transfer encoding
		*         ERR_BODY: Error while reading body
		*
		* Throws an mbed_error in case of failure.
		*
		*/
		int recv(net::Socket& socket);

		/* Returns the HTTP version
		*/
		inline const std::string& get_version() const noexcept { return _version; }

		/* Returns the HTTP status code
		*/
		inline const int get_status_code() const noexcept { return _status_code; }

		/* Returns the HTTP reason phrase
		*/
		inline const std::string& get_reason_phrase() const noexcept { return _reason_phrase; }

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
		// a reference to the application logger
		tools::Logger* const _logger;

		// The HTTP version
		std::string _version;

		// The HTTP status code
		int _status_code;

		// The HTTP reason phrase
		std::string _reason_phrase;

		// All headers except cookies
		Headers _headers;

		// All cookies
		Cookies _cookies;

		// The body of the answer
		tools::ByteBuffer _body;

		static const int MAX_LINE_SIZE = (8 * 1024);
		static const int MAX_HEADER_SIZE = (4 * 1024);
		static const int MAX_BODY_SIZE = (32 * 1014 * 1024);
		static const int MAX_CHUNCK_SIZE = (2 * 1014 * 1024);

		/* Reads a sequence of bytes from the socket. The buffer pointed to by 
		* the buffer parameter will contain the data received. The number of bytes to
		* read is specified by the len parameter.
		*
		* @param socket The socket connected to the server
		* @param buffer The buffer to store received data
		* @param len Number of bytes to read
		*
		* @return number of bytes, 0 if the socket is closed.
		*
		* Throws an mbed_error in case of failure.
		*/
		int read(net::Socket& socket, unsigned char* buf, const size_t len);

		/* Reads a single character.
		*
		* @param socket The socket connected to the server
		* @return 1 in case of success, 0 if the socket is closed.
		*
		* Throws an mbed_error in case of failure.
		*/
		int read_char(net::Socket& socket, char& c);

		/* Reads a string until \r\n is found. The \r\n are not included
		 * in the line.
		 *
		 * @param socket  The socket connected to the server
		 * @param max_len The maximum number of bytes this client accept when reading
		 *                the line..
		 * @param line    The string read from the socket
		 * @return the number of characters in the line.
		 *
		 * Throws an mbed_error in case of failure.
		 */
		int read_line(net::Socket& socket, int max_len, tools::obfstring& line);

		/* Reads the HTTP status response
		*
		* @param socket The socket connected to the server
		* @return false if the socket has been closed.
		*
		* @return .
		*         ERR_INVALID_STATUS_LINE :	HTTP status line is invalid
		*         ERR_INVALID_VERSION :		HTTP version is not HTTP/1.1
		*         ERR_INVALID_STATUS_CODE :	HTTP status code is not valid
		*
		* Throws an mbed_error in case of failure.
		*/
		int read_status(net::Socket& socket);

		/* Reads a gzip body
		*
		* @param socket The socket connected to the server
		* @param size The size of the body (in bytes)
		* @param max_size The maximum size of the body we can load (in bytes)
		*
		* @return
		* Throws an mbed_error in case of failure.
		*/
		bool read_gzip_body(net::Socket& socket, int size, int max_size);

		/* Reads the body
		*
		* @param socket The socket connected to the server
		* @param size The size of the body (in bytes)
		* @param max_size The maximum size of the body we can load (in bytes)
		*
		* @return 
		* Throws an mbed_error in case of failure.
		*/
		bool read_body(net::Socket& socket, int size, int max_size);
	};
}

