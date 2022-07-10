/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <winsock2.h>
#include "net/Listener.h"

namespace net {
	using namespace tools;

	Listener::Listener() :
		_logger(Logger::get_logger()),
		_endpoint()
	{
		DEBUG_CTOR(_logger, "Listener");
		mbedtls_net_init(&_netctx);
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

		rc = mbedtls_net_bind(&_netctx, host.c_str(), port.c_str(), MBEDTLS_NET_PROTO_TCP);
		if (rc) {
			// Unable to bind this host
			_endpoint = endpoint;
			goto terminate;
		}

		// get the port that has been assigned during the bind.
		struct sockaddr_in addr;
		int len = sizeof(sockaddr_in);
		if (getsockname(_netctx.fd, (SOCKADDR*)&addr, &len)) {
			_logger->error("ERROR: Listener::bind getsockname error %d", WSAGetLastError());

			rc = MBEDTLS_ERR_NET_BIND_FAILED;
			goto terminate;
		}
		_endpoint = Endpoint(endpoint.hostname(), ntohs(addr.sin_port));

		// set the socket in non blocking mode
		if (mbedtls_net_set_nonblock(&_netctx)) {
			_logger->error("ERROR: Listener::bind ioctlsocket error %d", WSAGetLastError());

			rc = MBEDTLS_ERR_NET_BIND_FAILED;
			goto terminate;
		}


terminate:
		if (_logger->is_debug_enabled()) {
			_logger->debug(
				"... %x Listener::bind endpoint=%s rc=%d",
				this,
				endpoint.to_string().c_str(),
				rc);
		}

		return rc;
	}


	mbed_err Listener::accept(Socket& socket)
	{
		DEBUG_ENTER(_logger, "Listener", "accept");

		int rc = 0;
		mbedtls_net_context client_ctx;

		rc = mbedtls_net_accept(&_netctx, &client_ctx, nullptr, 0, nullptr);
		if (rc == 0)
			socket.attach(client_ctx);

		if (_logger->is_debug_enabled()) {
			_logger->debug(
				"... %x Listener::accept endpoint=%s rc=%d",
				this,
				_endpoint.to_string().c_str(),
				rc);
		}

		return rc;
	}


	void Listener::close()
	{
		DEBUG_ENTER(_logger, "Listener", "close");

		mbedtls_net_free(&_netctx);
	}


	bool Listener::is_ready() const
	{
		int opt_val;
		int opt_len = sizeof(opt_val);

		if (getsockopt(_netctx.fd,
			SOL_SOCKET,
			SO_ACCEPTCONN,
			(char*)&opt_val,
			&opt_len) == SOCKET_ERROR)
			return false;

		return opt_val != 0;
	}

}