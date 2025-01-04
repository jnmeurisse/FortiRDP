/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "mbedccl/netctx.h"

#include "net/Endpoint.h"
#include "net/Socket.h"

#include "tools/ErrUtil.h"
#include "tools/Logger.h"


namespace net {
	using namespace tools;

	/**
	* A socket listener
	*/
	class Listener final
	{
	public:
		Listener();
		~Listener();

		/* Binds this listener to the specified end point. The function
		 * returns 0 if the bind is successful or a negative error code.
		*/
		mbed_errnum bind(const Endpoint& endpoint);

		/* Waits for an incoming connection. The connection is assigned
		 * to the given network context parameter. The function returns 0 if
		 * the accept is successful or a negative error code.
		*/
		mbed_errnum accept(netctx& accepting_ctx);

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
		int get_fd() const;

	private:
		// a reference to the application logger
		Logger* const _logger;

		ccl::netctx_ptr _bind_context;
		Endpoint _endpoint;
	};

}
