/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <cstdint>
#include <openssl/bio.h>

#include "net/Endpoint.h"
#include "tools/Logger.h"

namespace net {
	using namespace tools;

	class Socket
	{
	public:
		using byte = uint8_t;

		// Receive status
		enum rcv_status {
			rcv_ok,
			rcv_retry,
			rcv_eof,
			rcv_error
		};


		enum snd_status {
			snd_ok,
			snd_retry,
			snd_error
		};


		virtual ~Socket();

		/* Connects this socket to the specified end point.  The function
		 * returns true if the bind is successful.
		 *
		*/
		bool connect(const Endpoint& ep, int timeout);

		/* Closes the socket.
		 *
		*/
		bool close();

		/* Receives data from the socket. The buffer 'buf' will contain the
		 * data received. The maximum amount of data to be received at once is 
		 * specified by the 'len' parameter.
		 *
		*/
		rcv_status recv(byte* buf, const size_t len, size_t& rbytes);

		/* Sends data to the socket. The buffer 'buf' must contain the data.  The 
		 * amount of data to sent is specified by the 'len' parameter.
		 *
		*/
		snd_status send(const byte* buf, const size_t len, size_t& sbytes);

		/* Sets no delay option (disable Nagle algorithm).
		 *
		*/
		virtual bool set_nodelay(bool no_delay) = 0;

		/* Flushes data that can be in the local socket buffer and forces
		 * to send it to the server.
		*/
		virtual bool flush();

		/* Returns the socket descriptor.
		*/
		virtual int get_fd() const;

		/* Returns the endpoint.
		*/
		Endpoint get_endpoint() const;

		/* Returns true if the connect was called and succeeded.
		*/
		bool is_connected() const noexcept;

	protected:
		// Constructs a socket from an OpenSSL BIO
		Socket(::BIO* bio);

		// A reference to the application logger
		Logger* const _logger;

		// Implementation of the connect method.
		virtual bool do_connect(int timeout) = 0;

		// Returns the BIO object.
		inline ::BIO* get_bio() const noexcept { return _bio; }

	private:
		// Is true when the connect method succeeds
		bool _connected;

		// A reference to the basic I/O object
		::BIO* const _bio;

		// A convenient method used to configure the host and port
		bool set_endpoint(const Endpoint& ep);
	};
}
