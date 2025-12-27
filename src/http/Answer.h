/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include "http/Cookies.h"
#include "http/Headers.h"
#include "net/TcpSocket.h"
#include "tools/ByteBuffer.h"
#include "tools/Logger.h"
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
		/* Allocates an HTTP answer
		*/
		Answer();

		/* Clears this HTTP Answer
		*/
		void clear();

		/**
		 * Receives and parses an HTTP response from a TCP socket.
		 *
		 * Reads an HTTP response message from the given socket using the following
		 * sequence:
		 *   1. Status line
		 *   2. Message headers
		 *   3. Optional message body
		 *
		 * Unsupported or invalid encoding, malformed headers, or body read errors
		 * result in an exception being thrown.
		 *
		 * The body data is stored internally in the Answer instance.
		 *
		 * @param socket TCP socket used to receive the response.
		 * @param timer  A timer that specifies the timeout for reading the response.
		 *
		 * @throws mbed_error If an error occurs while receiving the response
		 *                    such as network, socket-related, timeout issues.
		 * @throws http_error If the response is malformed, uses unsupported transfer
		 *                    or content encoding, exceeds configured size limits.
		 */
		void recv(net::TcpSocket& socket, const tools::Timer& timer);

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
		// The class name
		static const char* __class__;

		// A reference to the application logger
		tools::Logger* const _logger;

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

		// Error codes
		enum class answer_status {
			ERR_NONE = 0,					// The response is valid and fully processed.
			ERR_EOF,						// 
			ERR_INVALID_FIELD,				// The HTTP header contains invalid field.
			ERR_INVALID_VERSION,			// The HTTP version in the response is invalid.
			ERR_INVALID_STATUS_LINE,		// The HTTP status line is malformed.
			ERR_INVALID_STATUS_CODE,		// The HTTP status code is invalid.
			ERR_INVALID_HEADER,				// The HTTP headers are malformed.
			ERR_CHUNK_SIZE,					// The HTTP chunk size cannot be read in chunked transfer encoding.
			ERR_BODY_SIZE,					// The HTTP content length cannot be read.
			ERR_CONTENT_ENCODING,			// The HTTP content encoding is unsupported.
			ERR_TRANSFER_ENCODING,			// The HTTP transfer encoding is unsupported.
			ERR_BODY,						// The HTTP body is incomplete.
		};


		/**
		 * Reads a sequence of bytes from the socket.
		 *
		 * This function reads data from the socket and stores it in the buffer pointed to
		 * by the `buffer` parameter. The reading continues until either the buffer is completely
		 * filled (as specified by the `len` parameter) or the specified `timer` elapses,
		 * whichever occurs first. If the socket is closed, the function returns false.
		 *
		 * @param socket The socket from which data will be read.
		 * @param buffer Pointer to the buffer where the received data will be stored.
		 * @param len The number of bytes to read, which is the size of the provided buffer.
		 * @param timer The maximum amount of time (in milliseconds) to wait for the operation
		 *              to complete before timing out.
		 *
		 * @return true if read succeeded, or false if the socket is closed.
		 *
		 * @throws mbed_error If an error occurs while reading from the socket, such as network
		 *                    failures or read timeout.
		 */
		bool read_buffer(net::TcpSocket& socket, unsigned char* buffer, const size_t len, const tools::Timer& timer);

		/**
		 * Reads a single byte from the socket.
		 *
		 * This function reads one byte from the socket connected to the server and
		 * stores it in the variable `c`.
		 *
		 * @param socket The socket connected to the server from which the character will be read.
		 * @param c Reference to an unsigned character variable where the received data will be stored.
		 * @param timer The maximum amount of time to wait for the operation to complete before
		 *              timing out.
		 *
		 * @return true if the character was successfully read, false if the socket is closed.
		 *
		 * @throws mbed_error If an error occurs while reading from the socket, such as network
		 *                    failures or read timeout.
		 */
		bool read_byte(net::TcpSocket& socket, unsigned char& c, const tools::Timer& timer);

		/**
		 * Reads a string from the socket until a newline sequence (\r\\n) is encountered.
		 * The \r\\n characters are not included in the returned line.
		 *
		 * This function reads characters from the socket and stores them in the `line`
		 * parameter until the end of the line is reached (i.e., \r\\n is encountered) or
		 * the buffer capacity is reached. 
		 *
		 * @param socket The socket connected to the server from which data will be read.
		 * @param buffer The buffer where the read characters will be stored.
		 * @param timer A timer specifying the maximum time to wait before the operation times out.
		 *
		 * @retval `ERR_NONE` if the status line was successfully read and valid.
		 * @retval `ERR_EOF` if the socket was closed before receiving a whole line.
		 *
		 * @throws mbed_error If an error occurs while reading from the socket, such as network
		 *                    failures or read timeout.
		 */
		answer_status read_line(net::TcpSocket& socket, tools::ByteBuffer& buffer, const tools::Timer& timer);

		/**
		 * Reads the HTTP status response from the server.
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
		 * @retval `ERR_NONE` if the status line was successfully read and valid.
		 * @retval `ERR_INVALID_STATUS_LINE` if the HTTP status line is malformed.
		 * @retval `ERR_INVALID_VERSION` if the HTTP version is not HTTP/1.1.
		 * @retval `ERR_INVALID_STATUS_CODE` if the HTTP status code is not valid.
		 *
		 * @throws mbed_error If an error occurs while reading from the socket, such as network
		 *                    failures or read timeout.
		 */
		answer_status read_control_data(net::TcpSocket& socket, const tools::Timer& timer);

		/**
		 * Reads and parses HTTP  headers from a TCP socket.
		 *
		 * This function reads header lines from the socket until an empty line is
		 * encountered (end of headers) or an error occurs. Each line is expected to
		 * be in the form "Field-Name: Field-Value".
		 *
		 * - Regular headers are stored in the internal header collection.
		 * - "Set-Cookie" headers are parsed and added to the internal cookie store.
		 *
		 * Reading stops when:
		 * - An empty line is received (successful end of headers), or
		 * - read_line() returns an error status.
		 *
		 * @param socket TCP socket to read header data from.
		 * @param timer A timer that specifies the maximum time to wait for the operation to complete.
		 *
		 * @retval `ERR_NONE` on successful completion.
		 * @retval `ERR_INVALID_FIELD` if a header field name is invalid,
		 * or any error code returned by read_line().
		 * 
		 * @throws mbed_error If an error occurs while reading from the socket, such as network
		 *                    failures or read timeout.
		 */
		answer_status read_headers(net::TcpSocket& socket, const tools::Timer& timer);

		/**
		 * Reads and decompresses a gzip body from the server.
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
		 * @throws mbed_error If an error occurs while reading from the socket, such as network
		 *                    failures or read timeout.
		 */
		bool read_gzip_body(net::TcpSocket& socket, size_t size, size_t max_size, const tools::Timer& timer);

		/**
		 * Reads the body of the HTTP response from the server in chunks and appends it to the body.
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
		 * @throws mbed_error If an error occurs while reading from the socket, such as network
		 *                    failures or read timeout.
		 */
		bool read_body(net::TcpSocket& socket, size_t size, size_t max_size, const tools::Timer& timer);


		static std::string answer_status_msg(answer_status);
	};

}
