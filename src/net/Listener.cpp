/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <winsock2.h>
#include <new>
#include "Listener.h"


namespace net {

	using namespace tools;

	Listener::Listener() :
		_logger(Logger::get_logger()),
		_endpoint(),
		_bind_context(::netctx_alloc(), ::netctx_free)
	{
		DEBUG_CTOR(_logger, "Listener");

		if (!_bind_context.get())
			throw std::bad_alloc();
	}


	Listener::~Listener()
	{
		DEBUG_DTOR(_logger, "Listener");
		close();
	}


	mbed_err Listener::bind(const Endpoint& endpoint)
	{
		DEBUG_ENTER(_logger, "Listener", "bind");

		mbed_err rc = 0;
		std::string host{ endpoint.hostname() };
		std::string port{ std::to_string(endpoint.port()) };

		/*
		* SO_EXCLUSIVEADDRUSE can not be enabled, mbedtls_net_bind is setting SO_REUSEADDR
		* in mbedtls_net_bind. Until mbedtls is improved and allows to specify this option
		* it will not be possible to avoid that another process binds the same port.
		*
		* // set the exclusive address option
		* int option = 1;
		* if (setsockopt(_netctx.fd, SOL_SOCKET,
		* 	SO_EXCLUSIVEADDRUSE, (char *)&option, sizeof(option)) == SOCKET_ERROR) {
		* 	_logger->error("ERROR: Listener::bind setsockopt failed, error=%d", WSAGetLastError());
		*
		* 	rc = MBEDTLS_ERR_NET_BIND_FAILED;
		* 	goto terminate;
		* }
		*/

		rc = ::netctx_bind(_bind_context.get(), host.c_str(), port.c_str(), NETCTX_PROTO_TCP);
		if (rc) {
			// Unable to bind this host
			_endpoint = endpoint;
			goto terminate;
		}

		// get the port that has been assigned during the bind.
		const int bind_port = ::netctx_get_port(_bind_context.get());
		if (bind_port == -1) {
			_logger->error("ERROR: Listener::bind getsockname error %d", WSAGetLastError());

			rc = MBEDTLS_ERR_NET_BIND_FAILED;
			goto terminate;
		}
		_endpoint = Endpoint(endpoint.hostname(), bind_port);

		// set the socket in non blocking mode
		if (::netctx_set_blocking(_bind_context.get(), false) != 0) {
			_logger->error("ERROR: Listener::bind set_blocking error %d", WSAGetLastError());

			rc = MBEDTLS_ERR_NET_BIND_FAILED;
			goto terminate;
		}

terminate:
		if (_logger->is_debug_enabled()) {
			_logger->debug(
				"... %x Listener::bind endpoint=%s rc=%d",
				(uintptr_t)this,
				endpoint.to_string().c_str(),
				rc);
		}

		return rc;
	}


	mbed_err Listener::accept(mbedtls_net_context& accepting_ctx)
	{
		DEBUG_ENTER(_logger, "Listener", "accept");

		const int rc = ::netctx_accept(_bind_context.get(), &accepting_ctx);

		if (_logger->is_debug_enabled()) {
			_logger->debug(
				"... %x Listener::accept endpoint=%s rc=%d",
				(uintptr_t)this,
				_endpoint.to_string().c_str(),
				rc);
		}

		return rc;
	}


	void Listener::close()
	{
		DEBUG_ENTER(_logger, "Listener", "close");

		::netctx_close(_bind_context.get());
	}


	bool Listener::is_ready() const
	{
		int opt_val;
		int opt_len = sizeof(opt_val);

		if (::getsockopt(get_fd(),
			SOL_SOCKET,
			SO_ACCEPTCONN,
			(char*)&opt_val,
			&opt_len) == SOCKET_ERROR)
			return false;

		return opt_val != 0;
	}


	int Listener::get_fd() const
	{
		return ::netctx_getfd(_bind_context.get());
	}

}
