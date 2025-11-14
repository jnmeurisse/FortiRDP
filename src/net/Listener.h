/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "net/Endpoint.h"
#include "net/Socket.h"
#include "tools/Logger.h"


namespace net {

	using namespace tools;

	/**
	* A socket listener
	*/
	class Listener : public Socket
	{
	public:
		Listener();
		~Listener();

		/**
		 * Binds the listener to a specified endpoint.
		 *
		 * This function binds a listener to the provided endpoint, defined by a host name
		 * and port, and sets up the socket for non-blocking mode. The function ensures
		 * proper error handling and logging during the binding process.
		 *
		 * Notes:
		 * - If the bind operation is successful, the assigned port is retrieved and
		 *   stored in the `_endpoint` member.
		 *
		 * - The socket is configured for non-blocking mode after binding.
		 *
		 * @param endpoint The endpoint to bind the listener to, specified by its host name
		 *                 and port.
		 *
		 * @return mbed_err 0 on success; a negative error code on failure.
		 */
		mbed_err bind(const Endpoint& endpoint);

		/**
		 * Accepts an incoming connection on the listener's bound socket.
		 *
		 * This function waits for and accepts an incoming connection request on the
		 * listener's socket, which was previously bound to an endpoint. Upon success,
		 * the accepted connection's context is stored in the provided `accepting_ctx`
		 * parameter.
		 *
		 * Before calling this function, ensure the listener has already been bound
		 * to a valid endpoint using the `Listener::bind` function.
		 *
		 * @param accepting_ctx Reference to an `mbedtls_net_context` object that
		 *                      will be initialized with the context of the
		 *                      accepted connection.
		 *
		 * @return mbed_err Returns 0 on success, or a non-zero error code if the
		 *                  operation fails. The error codes are typically returned by
		 *                  the underlying `netctx_accept` function.
		 *
		 *
		 */
		virtual mbed_err accept(Socket& client_socket) override;

		/**
		 * Closes the listener.
		 *
		 * The listener stops immediately to listen for incoming connection.
		*/
		virtual void close() override;

		/**
		 * Returns the end point to which this listener was bound.
		*/
		inline const Endpoint& endpoint() const { return _endpoint; }

		/**
		 * Returns true if the listener is ready to accept a connection.
		*/
		bool is_ready() const;

	private:
		Endpoint _endpoint;
	};

}
