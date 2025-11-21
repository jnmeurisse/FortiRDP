/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <winsock2.h>
#include <Ws2ipdef.h>

#include "Socket.h"
#include <cstdint>



namespace net {
	using namespace tools;

	Socket::Socket() :
		_logger(Logger::get_logger()),
		_netctx{}
	{
		DEBUG_CTOR(_logger);

		::mbedtls_net_init(&_netctx);
	}


	Socket::~Socket()
	{
		DEBUG_DTOR(_logger);

		::mbedtls_net_close(&_netctx);
	}


	mbed_err Socket::connect(const Endpoint& ep, net::net_protocol protocol, const Timer& timer)
	{
		LWIP_UNUSED_ARG(timer);
		if (get_fd() != -1) {
			// The socket is connected.
			return MBEDTLS_ERR_NET_INVALID_CONTEXT;
		}

		const std::string host{ ep.hostname() };
		const std::string port{ std::to_string(ep.port()) };

		return ::mbedtls_net_connect(&_netctx, host.c_str(),port.c_str(), protocol);
	}


	mbed_err Socket::bind(const Endpoint& ep, net::net_protocol protocol)
	{
		mbed_err rc = MBEDTLS_ERR_NET_INVALID_CONTEXT;

		if (get_fd() == -1) {
			// The socket must be unconnected.

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

			const std::string host{ ep.hostname() };
			const std::string port{ std::to_string(ep.port()) };

			rc = ::mbedtls_net_bind(&_netctx, host.c_str(), port.c_str(), protocol);
		}

		return rc;
	}


	void Socket::close()
	{
		::mbedtls_net_close(&_netctx);
	}


	void Socket::shutdown()
	{
		// Gracefully shutdown the connection and close the socket.
		// The file descriptor is reset to -1 by the function.
		::mbedtls_net_free(&_netctx);
	}


	mbed_err Socket::set_blocking_mode(bool enable)
	{
		int rc = MBEDTLS_ERR_NET_INVALID_CONTEXT;

		if (get_fd() != -1) {
			rc = enable
				? ::mbedtls_net_set_block(&_netctx)
				: ::mbedtls_net_set_nonblock(&_netctx);

			if (rc)
				rc = MBEDTLS_ERR_NET_INVALID_CONTEXT;
		}

		return rc;
	}


	mbed_err Socket::set_nodelay(bool no_delay)
	{
		int rc = MBEDTLS_ERR_NET_INVALID_CONTEXT;

		if (get_fd() != -1) {
			const int tcp_nodelay = no_delay ? 1 : 0;

			rc = ::setsockopt(
				get_fd(),
				IPPROTO_TCP,
				TCP_NODELAY,
				(const char*)&tcp_nodelay,
				sizeof(tcp_nodelay));

			if (rc)
				rc = MBEDTLS_ERR_NET_INVALID_CONTEXT;
		}

		return rc;
	}


	net::rcv_status Socket::recv_data(unsigned char* buf, size_t len)
	{
		net::rcv_status status { rcv_status_code::NETCTX_RCV_ERROR, MBEDTLS_ERR_NET_INVALID_CONTEXT, 0 };

		const int rc = ::mbedtls_net_recv(&_netctx, buf, len);

		if (rc > 0) {
			status.code = rcv_status_code::NETCTX_RCV_OK;
			status.rc = 0;
			status.rbytes = rc;
		}
		else if (rc == 0) {
			status.code = rcv_status_code::NETCTX_RCV_EOF;
			status.rc = 0;
			status.rbytes = 0;
		}
		else if (rc == MBEDTLS_ERR_SSL_WANT_READ) {
			status.code = rcv_status_code::NETCTX_RCV_RETRY;
			status.rc = MBEDTLS_NET_POLL_READ;
			status.rbytes = 0;
		}
		else {
			status.code = rcv_status_code::NETCTX_RCV_ERROR;
			status.rc = rc;
		}

		return status;
	}


	net::snd_status Socket::send_data(const unsigned char* buf, size_t len)
	{
		net::snd_status status { NETCTX_SND_ERROR, MBEDTLS_ERR_NET_INVALID_CONTEXT, 0 };

		const int rc = ::mbedtls_net_send(&_netctx, buf, len);

		if (rc > 0) {
			status.code = NETCTX_SND_OK;
			status.rc = 0;
			status.sbytes = rc;
		}
		else if (rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
			status.code = NETCTX_SND_RETRY;
			status.rc = MBEDTLS_NET_POLL_WRITE;
		}
		else {
			status.code = NETCTX_SND_ERROR;
			status.rc = rc;
		}

		return status;
	}


	bool Socket::get_port(uint16_t& port) const noexcept
	{
		bool rc = false;

		if (get_fd() != -1) {
			struct sockaddr_storage sock_addr;
			int len = sizeof(sock_addr);

			if (::getsockname(get_fd(), (struct sockaddr*)&sock_addr, &len) == 0) {
				if (sock_addr.ss_family == AF_INET) {
					const struct sockaddr_in* addr4 = (const struct sockaddr_in*)&sock_addr;
					port = ntohs(addr4->sin_port);
					rc = true;
				}
				else if (sock_addr.ss_family == AF_INET6) {
					const struct sockaddr_in6* addr6 = (const struct sockaddr_in6*)&sock_addr;
					port = ntohs(addr6->sin6_port);
					rc = true;
				}
			}
		}

		return rc;
	}


	net::Socket::poll_status Socket::poll(int rw, uint32_t timeout)
	{
		Socket::poll_status status { poll_status_code::NETCTX_POLL_ERROR, MBEDTLS_ERR_NET_INVALID_CONTEXT };

		// Wait to be ready for read or write.
		const int rc = ::mbedtls_net_poll(&_netctx, rw, timeout);
		if (rc < 0) {
			// An error has occurred.
			status.rc = rc;
		}
		else if (rc == 0) {
			// Wait timed out.
			status.code = poll_status_code::NETCTX_POLL_ERROR;
			status.rc = MBEDTLS_ERR_SSL_TIMEOUT;
		}
		else {
			// Ready for read and/or write.  rc contains a bit mask
			// that indicates if the socket is ready for reading or writing.
			status.code = poll_status_code::NETCTX_POLL_OK;
			status.rc = rc;
		}

		return status;
	}


	mbed_err Socket::accept(Socket& client_socket)
	{
		if (!is_connected())
			return MBEDTLS_ERR_NET_INVALID_CONTEXT;

		if (client_socket.is_connected())
			return MBEDTLS_ERR_NET_INVALID_CONTEXT;

		int rc;
		do {
			rc = ::mbedtls_net_accept(&_netctx, client_socket.netctx(), 0, 0, 0);
		} while (rc == MBEDTLS_ERR_SSL_WANT_READ);

		return rc;
	}

	const char* Socket::__class__ = "Socket";

}
