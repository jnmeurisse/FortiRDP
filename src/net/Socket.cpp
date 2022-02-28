/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <iostream>

#include "tools/Logger.h"
#include "net/Socket.h"

namespace net {
	using namespace tools;

	Socket::Socket() :
		_mutex(),
		_logger(Logger::get_logger()),
		_send_timeout(0),
		_recv_timeout(0),
		_no_delay(0)
	{
		DEBUG_CTOR(_logger, "Socket");
		mbedtls_net_init(&_netctx);
	}


	Socket::~Socket()
	{
		DEBUG_DTOR(_logger, "Socket");
		close();
	}


	void Socket::attach(const mbedtls_net_context& netctx)
	{
		if (_logger->is_debug_enabled())
			_logger->debug("... %x enter Socket::attach fd=%d", this, netctx.fd);
		Mutex::Lock lock(_mutex);
		_netctx = netctx;

		apply_opt();
	}


	mbed_err Socket::connect(const Endpoint& ep)
	{
		DEBUG_ENTER(_logger, "Socket", "connect");
		Mutex::Lock lock(_mutex);
		return do_connect(ep);
	}


	void Socket::close()
	{
		DEBUG_ENTER(_logger, "Socket", "close");
		Mutex::Lock lock(_mutex);
		do_close();
	}


	void Socket::set_timeout(DWORD send_timeout, DWORD recv_timeout)
	{
		if (_logger->is_debug_enabled())
			_logger->debug("... %x enter Socket::set_timeout s=%d r=%d", this, send_timeout, recv_timeout);
		_send_timeout = send_timeout;
		_recv_timeout = recv_timeout;

		apply_opt();
	}


	void Socket::set_nodelay(bool no_delay)
	{
		if (_logger->is_debug_enabled())
			_logger->debug("... %x enter Socket::set_nodelay=%d", this, no_delay);
		_no_delay = no_delay ? 1 : 0;

		apply_opt();
	}


	mbed_err Socket::set_blocking(bool blocking)
	{
		if (_logger->is_debug_enabled())
			_logger->debug("... %x enter Socket::set_noblocking=%d", this, blocking);
		int rc = (blocking)
					? mbedtls_net_set_block(&_netctx)
					: mbedtls_net_set_nonblock(&_netctx);
		return rc;
	}


	int Socket::recv(unsigned char* buf, const size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter Socket::recv buffer=%x len=%d", this, buf, len);

		return mbedtls_net_recv(&_netctx, buf, len);
	}


	int Socket::send(const unsigned char* buf, const size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter Socket::send buffer=%x len=%d", this, buf, len);

		return mbedtls_net_send(&_netctx, buf, len);
	}


	mbed_err Socket::flush()
	{
		return 0;
	}


	int Socket::read(unsigned char *buf, const size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter Socket::read buffer=%x len=%d", this, buf, len);

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
			_logger->trace(".... %x enter Socket::write buffer=%x len=%d", this, buf, len);

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
			_logger->debug("... %x enter Socket::do_connect ep=%s", this, ep.to_string().c_str());

		const std::string& host = ep.hostname();
		const std::string port(std::to_string(ep.port()));
		int rc;

		rc = mbedtls_net_connect(&_netctx, host.c_str(), port.c_str(), MBEDTLS_NET_PROTO_TCP);
		if (rc)
			goto terminate;

		rc = mbedtls_net_set_block(&_netctx);
		if (rc)
			goto terminate;

		// configure timeout and delay
		apply_opt();

		//int bufsize = 2048;
		//setsockopt(_netctx.fd, SOL_SOCKET, SO_SNDBUF, (char *) &bufsize, sizeof(bufsize));
		//setsockopt(_netctx.fd, SOL_SOCKET, SO_RCVBUF, (char *) &bufsize, sizeof(bufsize));


	terminate:
		if (_logger->is_debug_enabled())
			_logger->debug("... %x leave Socket::do_connect fd=%d %d", this, _netctx.fd, rc);

		return rc;
	}


	void Socket::do_close()
	{
		if (_logger->is_debug_enabled())
			_logger->debug("... %x enter Socket::do_close fd=%d", this, _netctx.fd);
		mbedtls_net_free(&_netctx);
	}


	void Socket::apply_opt()
	{
		if (connected()) {
			if (_logger->is_debug_enabled())
				_logger->debug("... %x enter Socket::apply_opt fd=%d", this, _netctx.fd);

			if (setsockopt(_netctx.fd, IPPROTO_TCP, TCP_NODELAY, (char *)&_no_delay, sizeof(_no_delay)) != 0) {
				_logger->error("ERROR : set nodelay error %x on socket %x, fd=%d",
					::WSAGetLastError(),
					this,
					_netctx.fd);
			}

			if (setsockopt(_netctx.fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&_recv_timeout, sizeof(_recv_timeout)) != 0) {
				_logger->error("ERROR : set receive time-out error %x on socket %x, fd=%d",
					::WSAGetLastError(),
					this,
					_netctx.fd);
			}

			if (setsockopt(_netctx.fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&_send_timeout, sizeof(_send_timeout)) != 0) {
				_logger->error("ERROR : set send time-out error %x on socket %x, fd=%d",
					::WSAGetLastError(),
					this,
					_netctx.fd);
			}
		}
	}

}