/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#include "net/TlsSocket.h"
#include "tools/Path.h"
#include "tools/Timer.h"


namespace net {

	TlsSocket::TlsSocket(const SslConfig& config) :
		TcpSocket(),
		_ssl_context(config.create_ssl_context(), ::sslctx_free)
	{
		DEBUG_CTOR(_logger, "TlsSocket");
	}


	TlsSocket::~TlsSocket()
	{
		DEBUG_DTOR(_logger, "TlsSocket");
	}


	mbed_errnum TlsSocket::connect(const Endpoint& ep)
	{
		DEBUG_ENTER(_logger, "TlsSocket", "connect");

		mbed_errnum errnum = TcpSocket::connect(ep);
		if (errnum)
			goto terminate;

		errnum = ::sslctx_set_netctx(_ssl_context.get(), get_ctx());
		if (errnum)
			goto terminate;

	terminate:
		if (_logger->is_debug_enabled())
			_logger->debug(
				"... %x leave TlsSocket::connect fd=%d rc=%d",
				std::addressof(this),
				get_fd(),
				errnum
			);

		return errnum;
	}



	sslctx_handshake_status TlsSocket::handshake(Timer& timer)
	{
		DEBUG_ENTER(_logger, "TlsSocket", "handshake");

		sslctx_handshake_status handshake_status;
		do {
			// perform handshake with the SSL server
			handshake_status = ::sslctx_handshake(_ssl_context.get());

			if (handshake_status.status_code == SSLCTX_HDK_WAIT_IO) {
				const netctx_poll_status poll_status = poll(true, true, timer.remaining_delay());

				if (poll_status.status_code != NETCTX_POLL_OK) {
					// Polling error
					handshake_status.status_code = SSLCTX_HDK_ERROR;
					handshake_status.errnum = poll_status.errnum;
				}
				else
					// Noop, poll succeeded
					;
			}
			else if (handshake_status.status_code == SSLCTX_HDK_WAIT_ASYNC) {
				if (timer.is_elapsed()) {
					handshake_status.status_code = SSLCTX_HDK_ERROR;
					handshake_status.errnum = mbedccl_get_timeout_error();
				}
				else {
					Sleep(100);
				}
			}
		} while (handshake_status.status_code == SSLCTX_HDK_WAIT_IO || 
			handshake_status.status_code == SSLCTX_HDK_WAIT_ASYNC);

		if (_logger->is_debug_enabled())
			_logger->debug(
				"... %x leave TlsSocket::handshake status=%d error=%d",
				std::addressof(this),
				handshake_status.status_code,
				handshake_status.errnum
			);

		return handshake_status;
	}


	mbed_errnum TlsSocket::close()
	{
		DEBUG_ENTER(_logger, "TlsSocket", "close");

		// ignore all errors while closing the connection
		::sslctx_close(_ssl_context.get());
		TcpSocket::close();

		return 0;
	}


	uint32_t TlsSocket::get_crt_check() const
	{
		return ::sslctx_get_verify_result(_ssl_context.get());
	}


	std::string TlsSocket::get_ciphersuite() const
	{
		return ::sslctx_get_ciphersuite(_ssl_context.get());
	}


	std::string TlsSocket::get_tls_version() const
	{
		return ::sslctx_get_version(_ssl_context.get());
	}


	const x509crt* TlsSocket::get_peer_crt() const
	{
		return ::sslctx_get_peer_x509crt(_ssl_context.get());
	}


	netctx_rcv_status TlsSocket::recv(unsigned char* buf, const size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter TlsSocket::recv buffer=%x len=%d", this, buf, len);

		return ::sslctx_recv(_ssl_context.get(), buf, len);
	}


	netctx_snd_status TlsSocket::send(const unsigned char* buf, const size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter TlsSocket::send buffer=%x len=%d", this, buf, len);

		return ::sslctx_send(_ssl_context.get(), buf, len);
	}


	netctx_poll_status TlsSocket::poll_rcv(uint32_t timeout)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter TlsSocket::poll_rcv timeout=%d", this, timeout);

		return poll(true, true, timeout);
	}


	netctx_poll_status TlsSocket::poll_snd(uint32_t timeout)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter TlsSocket::poll_snd timeout=%d", this, timeout);

		return poll(true, true, timeout);
	}


/*	TODO: do we have to restore this ?
	mbed_err TlsSocket::flush()
	{
		DEBUG_ENTER(_logger, "TlsSocket", "flush");
		//return mbedtls_ssl_flush_output(&_ssl_context);
		return 0;
	}
*/
}