/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Socket.h"

#include <iostream>
#include "tools/Logger.h"


namespace net {

	using namespace tools;

	Socket::Socket() :
		_mutex(),
		_logger(Logger::get_logger()),
		_send_timeout(0),
		_recv_timeout(0),
		_no_delay(false),
		_blocking(true)
	{
		DEBUG_CTOR(_logger, "Socket");
		mbedtls_net_init(&_netctx);
	}


	Socket::~Socket()
	{
		DEBUG_DTOR(_logger, "Socket");
		close();
	}


	bool Socket::attach(const mbedtls_net_context& netctx)
	{
		if (_logger->is_debug_enabled())
			_logger->debug("... %x enter Socket::attach fd=%d", (uintptr_t)this, netctx.fd);
		Mutex::Lock lock{ _mutex };
		_netctx = netctx;

		return apply_opt(SocketOptions::all);
	}


	mbed_err Socket::connect(const Endpoint& ep)
	{
		DEBUG_ENTER(_logger, "Socket", "connect");
		Mutex::Lock lock{ _mutex };

		return do_connect(ep);
	}


	void Socket::close()
	{
		DEBUG_ENTER(_logger, "Socket", "close");
		Mutex::Lock lock{ _mutex };
		
		do_close();
	}


	bool Socket::set_timeout(DWORD send_timeout, DWORD recv_timeout)
	{
		bool rc = true;

		if (_logger->is_debug_enabled())
			_logger->debug("... %x enter Socket::set_timeout s=%d r=%d", (uintptr_t)this, send_timeout, recv_timeout);

		if (send_timeout != _send_timeout || recv_timeout != _recv_timeout) {
			_send_timeout = send_timeout;
			_recv_timeout = recv_timeout;

			rc = apply_opt(SocketOptions::timeout);
		}

		return rc;
	}


	bool Socket::set_nodelay(bool no_delay)
	{
		bool rc = true;

		if (_logger->is_debug_enabled())
			_logger->debug("... %x enter Socket::set_nodelay=%d", (uintptr_t)this, no_delay ? 1 : 0);
		
		if (no_delay != _no_delay) {
			_no_delay = no_delay;

			rc = apply_opt(SocketOptions::nodelay);
		}

		return rc;
	}


	bool Socket::set_blocking(bool blocking)
	{
		bool rc = true;

		if (_logger->is_debug_enabled())
			_logger->debug("... %x enter Socket::set_noblocking=%d", (uintptr_t)this, blocking ? 1 : 0);

		if (blocking != _blocking) {
			_blocking = blocking;

			rc = apply_opt(SocketOptions::blocking);
		}

		return rc;
	}


	int Socket::recv(unsigned char* buf, const size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter Socket::recv buffer=%x len=%d", (uintptr_t)this, buf, len);

		return mbedtls_net_recv(&_netctx, buf, len);
	}


	int Socket::send(const unsigned char* buf, const size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter Socket::send buffer=%x len=%d", (uintptr_t)this, buf, len);

		return mbedtls_net_send(&_netctx, buf, len);
	}


	mbed_err Socket::flush()
	{
		return 0;
	}


	int Socket::read(unsigned char *buf, const size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter Socket::read buffer=%x len=%d", (uintptr_t)this, buf, len);

		int rc = 0;
		size_t cnt = len;

		while (cnt > 0) {
			rc = recv(buf, cnt);
			if (rc <= 0) return rc;

			cnt -= rc;
			buf += rc;
		}

		return (int)len;
	}


	int Socket::write(const unsigned char *buf, const size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter Socket::write buffer=%x len=%d", (uintptr_t)this, buf, len);

		int rc = 0;
		size_t cnt = (int)len;

		while (cnt > 0) {
			rc = send(buf, cnt);
			if (rc < 0) return rc;

			cnt -= rc;
			buf += rc;
		}

		return (int)len;
	}


	bool Socket::connected() const noexcept
	{
		return _netctx.fd != -1;
	}


	mbed_err Socket::do_connect(const Endpoint& ep)
	{
		if (_logger->is_debug_enabled())
			_logger->debug("... %x enter Socket::do_connect ep=%s", (uintptr_t)this, ep.to_string().c_str());

		const std::string& host = ep.hostname();
		const std::string port{ std::to_string(ep.port()) };
		int rc;

		rc = mbedtls_net_connect(&_netctx, host.c_str(), port.c_str(), MBEDTLS_NET_PROTO_TCP);
		if (rc)
			goto terminate;

		// configure all options
		if (!apply_opt(SocketOptions::all)) {
			rc = MBEDTLS_ERR_NET_CONNECT_FAILED;
			goto terminate;
		}

	terminate:
		if (_logger->is_debug_enabled())
			_logger->debug("... %x leave Socket::do_connect fd=%d rc=%d", (uintptr_t)this, _netctx.fd, rc);

		return rc;
	}


	void Socket::do_close()
	{
		if (_logger->is_debug_enabled())
			_logger->debug("... %x enter Socket::do_close fd=%d", (uintptr_t)this, _netctx.fd);

		mbedtls_net_free(&_netctx);
	}


	bool Socket::apply_opt(enum SocketOptions option)
	{
		bool rc = true;

		if (connected()) {
			if (_logger->is_debug_enabled())
				_logger->debug("... %x enter Socket::apply_opt fd=%d", (uintptr_t)this, _netctx.fd);

			if (option == SocketOptions::all || option == SocketOptions::nodelay) {
				const int tcp_nodelay = _no_delay ? 1 : 0;
				if (setsockopt(_netctx.fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&tcp_nodelay, sizeof(tcp_nodelay)) != 0) {
					_logger->error("ERROR: set nodelay error %x on socket %x, fd=%d",
						::WSAGetLastError(),
						(uintptr_t)this,
						_netctx.fd);

					rc = false;
				}
			}

			if (option == SocketOptions::all || option == SocketOptions::timeout) {
				if (setsockopt(_netctx.fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&_recv_timeout, sizeof(_recv_timeout)) != 0) {
					_logger->error("ERROR: set receive time-out error %x on socket %x, fd=%d",
						::WSAGetLastError(),
						(uintptr_t)this,
						_netctx.fd);

					rc = false;
				}

				if (setsockopt(_netctx.fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&_send_timeout, sizeof(_send_timeout)) != 0) {
					_logger->error("ERROR: set send time-out error %x on socket %x, fd=%d",
						::WSAGetLastError(),
						(uintptr_t)this,
						_netctx.fd);

					rc = false;
				}
			}
		}

		if (option == SocketOptions::all || option == SocketOptions::blocking) {
			int rc = _blocking
				? mbedtls_net_set_block(&_netctx)
				: mbedtls_net_set_nonblock(&_netctx);
			if (rc) {
				_logger->error("ERROR: set socket blocking mode error %x on socket %x, fd=%d",
					::WSAGetLastError(),
					(uintptr_t)this,
					_netctx.fd);

				rc = false;
			}
		}
		
		return rc;
	}

}
