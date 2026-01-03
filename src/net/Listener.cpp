/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <WinSock2.h>
#include "Listener.h"


namespace net {

	using namespace utl;

	Listener::Listener() :
		Socket(),
		_endpoint()
	{
		DEBUG_CTOR(_logger);
	}


	Listener::~Listener()
	{
		DEBUG_DTOR(_logger);
	}


	mbed_err Listener::bind(const Endpoint& endpoint)
	{
		DEBUG_ENTER_FMT(_logger, "ep=%s", endpoint.to_string().c_str());

		mbed_err rc = 0;

		rc = Socket::bind(endpoint, net_protocol::NETCTX_PROTO_TCP);
		if (rc) {
			// Unable to bind this host.
			_endpoint = endpoint;
			goto terminate;
		}

		// Get the port that has been assigned during the bind.
		uint16_t bind_port;
		if (!Socket::get_port(bind_port)) {
			_logger->error("ERROR: get_port error %d", ::WSAGetLastError());

			rc = MBEDTLS_ERR_NET_BIND_FAILED;
			goto terminate;
		}
		_endpoint = Endpoint(endpoint.hostname(), bind_port);

		// Set the socket in non blocking mode.
		if (Socket::set_blocking_mode(false) != 0) {
			_logger->error("ERROR: set_blocking error %d", ::WSAGetLastError());

			rc = MBEDTLS_ERR_NET_BIND_FAILED;
			goto terminate;
		}

terminate:
		LOG_DEBUG(_logger, "ep=%s fd=%d rc=%d", endpoint.to_string().c_str(), get_fd(), rc);

		return rc;
	}


	mbed_err Listener::accept(Socket& client_socket)
	{
		DEBUG_ENTER_FMT(_logger, "fd=%d", get_fd());

		const int rc = Socket::accept(client_socket);

		LOG_DEBUG(_logger, "fd=%d rc=%d", get_fd(), rc);

		return rc;
	}


	void Listener::close()
	{
		DEBUG_ENTER_FMT(_logger, "fd=%d", get_fd());
		Socket::close();
	}


	bool Listener::is_ready() const
	{
		int opt_val;
		int opt_len = sizeof(opt_val);

		if (::getsockopt(get_fd(),
			SOL_SOCKET,
			SO_ACCEPTCONN,
			reinterpret_cast<char*>(&opt_val),
			&opt_len) == SOCKET_ERROR)
			return false;

		return opt_val != 0;
	}

	const char* Listener::__class__ = "Listener";
}
