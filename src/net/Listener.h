/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "mbedtls/net_sockets.h"

#include "net/Endpoint.h"
#include "net/Socket.h"

#include "tools/ErrUtil.h"
#include "tools/Logger.h"


namespace net {
	using namespace tools;

	/**
	* A socket listener
	*/
	class Listener
	{
	public:
		explicit Listener();
		~Listener();

		/* Binds this listener to the specified end point. The function
		 * returns 0 if the bind is successful or a negative error code.
		*/
		mbed_err bind(const Endpoint& endpoint);

		/* Waits for an incoming connection. The connection is assigned
		 * to the given socket parameter. The function returns 0 if
		 * the accept is successful or a negative error code.
		*/
		mbed_err accept(Socket& socket);

		/* Closes the listener. The listener stops immediately to listen for
		 * incoming connection.
		*/
		void close();

		/* Returns the end point to which this listener was bound.
		*/
		inline const Endpoint& endpoint() const { return _endpoint; }

		/* Returns true if the listener is ready to accept a connection
		*/
		bool is_ready() const;

		/* Returns the socket descriptor
		*/
		inline int get_fd() const { return _netctx.fd; };

	private:
		// a reference to the application logger
		tools::Logger* const _logger;

		mbedtls_net_context _netctx;
		Endpoint _endpoint;
	};

}
