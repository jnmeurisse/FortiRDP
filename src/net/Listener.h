/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <openssl/bio.h>

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
		 * returns true if the bind is successful.
		*/
		bool bind(const Endpoint& endpoint);

		/* Waits for an incoming connection. The function returns a Socket if
		 * the accept is successful or a null pointer
		*/
		Socket* accept();

		/* Closes the listener. The listener stops immediately to listen for
		 * incoming connection.
		*/
		bool close();

		/* Returns the end point to which this listener was bound.
		*/
		Endpoint endpoint() const;

		/* Returns true if the listener is ready to accept a connection
		*/
		bool is_ready() const;

		/* Returns the socket descriptor
		*/
		int get_fd() const;

	private:
		// a reference to the application logger
		Logger* const _logger;

		::BIO* const _bio;
	};

}
