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
#include "net/TcpSocket.h"
#include "tools/ByteBuffer.h"
#include "tools/Logger.h"
#include "tools/ObfuscatedString.h"
#include "tools/Timer.h"


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

		/**
		 * Reads the response from the HTTP server and initializes this object.
		 *
		 * This function reads the HTTP response from the server, processes the status line,
		 * headers, and body, and initializes the object accordingly. If the response is valid,
		 * the function returns an appropriate error code based on the specific conditions encountered
		 * during parsing. If a network error, SSL error, or any other failure occurs while reading
		 * the response, the function throws an `mbed_error` exception.
		 *
		 * @param socket The socket connected to the HTTP server, from which the response will
		 *               be read.
		 * @param timer A timer that specifies the timeout for reading the response.
		 *
		 * @return An error code indicating the result of the operation:
		 *         - `ERR_NONE`: If the response is valid and fully processed.
		 *         - `ERR_INVALID_VERSION`: If the HTTP version in the response is invalid.
		 *         - `ERR_INVALID_STATUS_LINE`: If the HTTP status line is malformed.
		 *         - `ERR_INVALID_STATUS_CODE`: If the HTTP status code is invalid.
		 *         - `ERR_INVALID_HEADER`: If there is an error reading the headers.
		 *         - `ERR_CHUNK_SIZE`: If the chunk size cannot be decoded in chunked transfer encoding.
		 *         - `ERR_BODY_SIZE`: If the body size cannot be decoded.
		 *         - `ERR_CONTENT_ENCODING`: If the content encoding is unsupported.
		 *         - `ERR_TRANSFER_ENCODING`: If the transfer encoding is unsupported.
		 *         - `ERR_BODY`: If there is an error while reading the body of the response.
		 *
		 * @throws mbed_error If a network, SSL, or other critical error occurs during the
		 *                    response reading process, indicating failure at a system or
		 *                    connection level.
		 */
		int recv(net::TcpSocket& socket, tools::Timer& timer);

		/**
		 * Returns the HTTP version.
		*/
		inline const std::string& get_version() const noexcept { return _version; }

		/**
		 * Returns the HTTP status code.
		*/
		inline const int get_status_code() const noexcept { return _status_code; }

		/**
		 * Returns the HTTP reason phrase.
		*/
		inline const std::string& get_reason_phrase() const noexcept { return _reason_phrase; }

		/**
		 * Returns the headers collection.  The cookies are stored apart in
		 * obfuscated strings
		*/
		inline const Headers& headers() const { return _headers; }

		/**
		 * Returns the cookies collection.
		*/
		inline const Cookies& cookies() const { return _cookies; }

		/**
		 * Returns the body of the answer.
		*/
		inline const tools::ByteBuffer& body() const { return _body; }

	private:
		// A reference to the application logger
		tools::Logger* const _logger;

		// The HTTP version
		std::string _version;

		// The HTTP status code
		int _status_code;

		// The HTTP reason phrase
		std::string _reason_phrase;

		// All headers except cookies
		Headers _headers;

		// All cookies received in the answer.
		Cookies _cookies;

		// The body of the answer.
		tools::ByteBuffer _body;

		static const int MAX_LINE_SIZE = (8 * 1024);
		static const int MAX_HEADER_SIZE = (4 * 1024);
		static const int MAX_BODY_SIZE = (32 * 1014 * 1024);
		static const int MAX_CHUNK_SIZE = (2 * 1014 * 1024);

		/* Reads a sequence of bytes from the socket.
		 *
		 * This function reads data from the socket and stores it in the buffer pointed to
		 * by the `buf` parameter. The reading continues until either the buffer is completely
		 * filled (as specified by the `len` parameter) or the specified `timer` elapses,
		 * whichever occurs first. If the socket is closed, the function returns 0.
		 *
		 * @param socket The socket from which data will be read.
		 * @param buf Pointer to the buffer where the received data will be stored.
		 * @param len The number of bytes to read, which is the size of the provided buffer.
		 * @param timer The maximum amount of time (in milliseconds) to wait for the operation
		 *              to complete before timing out.
		 *
		 * @return The number of bytes actually read, or 0 if the socket is closed.
		 *
		 * @throws mbed_error If an error occurs while reading from the socket, such as network
		 *                    or socket-related failures.
		 */
		int read_buffer(net::TcpSocket& socket, unsigned char* buf, const size_t len, tools::Timer& timer);

		/* Reads a single character from the socket.
		 *
		 * This function reads one character from the socket connected to the server and
		 * stores it in the variable `c`. If the socket is closed, it returns 0. If the
		 * read operation is successful, it returns 1.
		 *
		 * @param socket The socket connected to the server from which the character will be read.
		 * @param c Reference to a character variable where the received data will be stored.
		 * @param timer The maximum amount of time to wait for the operation to complete before
		 *              timing out.
		 *
		 * @return 1 if the character was successfully read, 0 if the socket is closed.
		 *
		 * @throws mbed_error If an error occurs during the read operation (e.g., network or
		 *                    socket-related failures).
		 */
		int read_char(net::TcpSocket& socket, char& c, tools::Timer& timer);

		/* Reads a string from the socket until a newline sequence (\r\n) is encountered.
		 * The \r\n characters are not included in the returned line.
		 *
		 * This function reads characters from the socket and stores them in the `line`
		 * parameter until the end of the line is reached (i.e., \r\n is encountered) or
		 * the maximum length specified by `max_len` is reached. The function will return
		 * the number of characters in the line, excluding the \r\n sequence.
		 *
		 * @param socket The socket connected to the server from which data will be read.
		 * @param max_len The maximum number of characters that can be read (the maximum
		 *                size of the buffer).
		 * @param line The string where the read characters will be stored.
		 * @param timer A timer specifying the maximum time to wait before the operation times out.
		 *
		 * @return The number of characters in the line (not including the \r\n).
		 *
		 * @throws mbed_error If a failure occurs while reading from the socket (e.g.,
		 *                    network or socket-related errors).
		 */
		int read_line(net::TcpSocket& socket, int max_len, tools::obfstring& line, tools::Timer& timer);

		/* Reads the HTTP status response from the server.
		 *
		 * This function reads the HTTP status line from the server's response, which includes
		 * the HTTP version, status code, and status message. The function validates the status
		 * line and returns an appropriate error code if there are any issues with the format or
		 * content of the status line. If the socket is closed, the function returns
		 * `ERR_INVALID_STATUS_LINE`.
		 *
		 * @param socket The socket connected to the server from which the status response will be read.
		 * @param timer A timer that specifies the maximum time to wait for the operation to complete.
		 *
		 * @return
		 *         - `ERR_NONE` if the status line was successfully read and valid.
		 *         - `ERR_INVALID_STATUS_LINE`: If the HTTP status line is invalid.
		 *         - `ERR_INVALID_VERSION`: If the HTTP version is not HTTP/1.1.
		 *         - `ERR_INVALID_STATUS_CODE`: If the HTTP status code is not valid.
		 *
		 * @throws mbed_error If a failure occurs while reading from the socket (e.g.,
		 *                    network or socket-related errors).
		 */
		int read_http_status(net::TcpSocket& socket, tools::Timer& timer);

		/* Reads and decompresses a gzip body from the server.
		 *
		 * This function reads the gzip-compressed body of the HTTP response, decompresses it,
		 * and stores the result. The reading process continues until the specified size (in bytes)
		 * is reached, the timer expires, or the decompressed body exceeds the maximum size specified
		 * by `max_size`. If any of these conditions occur, the function stops reading.
		 *
		 * @param socket The socket connected to the server from which the compressed body will be read.
		 * @param size The size of the gzip-compressed body (in bytes) that needs to be read and decompressed.
		 * @param max_size The maximum size allowed for the decompressed body. If the decompressed body
		 *                 exceeds this size, the function will stop reading.
		 * @param timer The maximum amount of time allowed for reading and decompressing the body. The function
		 *              stops if the timer expires before the operation is complete.
		 *
		 * @return `false` if decompression fails (e.g., if an error occurs during decompression,
		 *         if the size exceeds `max_size`, or if the timer expires). Returns `true` if
		 *         decompression is successful.
		 *
		 * @throws mbed_error If a failure occurs while reading from the socket or other network-related issues.
		 */
		bool read_gzip_body(net::TcpSocket& socket, size_t size, size_t max_size, tools::Timer& timer);

		/* Reads the body of the HTTP response from the server in chunks and appends it to the body.
		 *
		 * This function reads the HTTP body from the server in chunks of 1024 bytes. The body is
		 * read until the specified size is reached, the maximum size of the body is reached, or the
		 * reading is interrupted by the expiration of the timer. The function ensures that the body
		 * is stored in the `_body` member, but will stop appending once the `max_size` limit is reached.
		 *
		 * @param socket The socket connected to the server from which the body data will be read.
		 * @param size The total size (in bytes) of the body to be read from the server. The function
		 *             will attempt to read this amount of data in chunks.
		 * @param max_size The maximum size (in bytes) that can be stored in the body buffer. The function
		 *                 will stop appending data once this limit is reached.
		 * @param timer A timer specifying the maximum time to wait before the operation times out.
		 *
		 * @return `true` if all the specified bytes (as indicated by `size`) were successfully read
		 *         and stored in the body buffer. Returns `false` if the reading process was interrupted
		 *         by the timer expiration or any other failure condition (e.g., socket issues).
		 *
		 * @throws mbed_error If a failure occurs while reading from the socket, such as network or socket-related errors.
		 */
		bool read_body(net::TcpSocket& socket, size_t size, size_t max_size, tools::Timer& timer);
	};

}
