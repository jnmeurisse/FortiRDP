/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include "mbedTLS/net_sockets.h"

#include "net/Endpoint.h"
#include "tools/ErrUtil.h"
#include "tools/Mutex.h"
#include "tools/Logger.h"

namespace net {

	using namespace tools;

	class Socket
	{
	public:
		Socket();
		virtual ~Socket();

		/* Connects this socket to the specified end point
		*/
		mbed_err connect(const Endpoint& ep);

		/* Attaches the net context to this socket
		*/
		void attach(const mbedtls_net_context& netctx);

		/* Closes gracefully the socket
		*/
		void close();

		/* Sets time-outs involved in send and receive operations.
		 * Time-out values are expressed in milliseconds. 
		*/
		void set_timeout(DWORD send_timeout, DWORD recv_timeout);

		/* Sets no delay option (disable Nagle algorithm)
		*/
		void set_nodelay(bool no_delay);

		/* Sets the socket blocking mode
		*/
		mbed_err set_blocking(bool blocking);

		/* Receives data from the socket. The buffer pointed to by the
		 * buf parameter will contain the data received. The maximum amount
		 * of data to be received at once is specified by len.
		 *
		 * The function returns the number of bytes received, which can
		 * be less than the number requested to be received. The function
		 * returns 0 if the socket has been closed, or a negative error code
		 * if an error has occurred.
		*/
		virtual int recv(unsigned char* buf, const size_t len);

		/* Sends data to the socket. The buffer pointed to by the by buf
		 * parameter must contain the data. The amount of data to sent is
		 * specified by the parameter len.
		 *
		 * The function returns the number of bytes sent, which can be less
		 * than the number requested to be sent. If an error has occurred,
		 * the function returns a negative error code.
		*/
		virtual int send(const unsigned char* buf, const size_t len);

		/* Flushes data that can be in the local socket buffer and forces
		 * to send it to the server.
		*/
		virtual mbed_err flush();

		/* Reads data from the socket. The buffer pointed to by the buf
		 * parameter will contain the data received. The number of bytes to
		 * read is specified by the len parameter.
		 *
		 * The function returns 0 if the socket has been closed, or a negative
		 * error code if an error has occurred.
		*/
		int read(unsigned char* buf, const size_t len);

		/* Writes data from the socket. The buffer pointed to by the buf
		 * parameter must contain the data. The number of bytes to write is
		 * specified by the parameter len.
		 *
		 * The function returns the number of bytes sent or a negative error
		 * code if an error has occurred.
		*/
		int write(const unsigned char* buf, const size_t len);

		/* Returns true if the socket is connected
		*/
		bool connected() const noexcept;

		/* Returns the socket descriptor
		*/
		inline int get_fd() const noexcept { return _netctx.fd; };

	protected:
		virtual mbed_err do_connect(const Endpoint& ep);
		virtual void do_close();

	protected:
		// a reference to the application logger
		tools::Logger* const _logger;

		// the ssl socket 
		mbedtls_net_context _netctx;

	private:
		// a mutex to guarantee thread safety
		tools::Mutex _mutex;

		// send and receive time-out values
		DWORD _send_timeout;
		DWORD _recv_timeout;

		// Disable Naggle algorithm
		BOOL _no_delay;

		void apply_opt();
	};

}
