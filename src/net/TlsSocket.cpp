/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "TlsSocket.h"

#include <new>


namespace net {

	using namespace tools;


	TlsSocket::TlsSocket(const TlsConfig& tls_config) :
		TcpSocket(),
		_tlscfg{ tls_config },
		_tlsctx{ ::tlsctx_alloc(), &::tlsctx_free },
		_enable_hostname_verification{ false }
	{
		DEBUG_CTOR(_logger, "TlsSocket");

		if (!_tlsctx.get())
			throw std::bad_alloc();
	}


	TlsSocket::~TlsSocket()
	{
		DEBUG_DTOR(_logger, "TlsSocket");

		// close the socket if not yet done
		close();
	}


	void TlsSocket::set_hostname_verification(bool enable_verification)
	{
		_enable_hostname_verification = enable_verification;
	}


	mbed_err TlsSocket::connect(const Endpoint& ep)
	{
		if (_logger->is_debug_enabled())
			_logger->debug(
				"... %x enter TlsSocket::connect ep=%s",
				(uintptr_t)this,
				ep.to_string().c_str()
			);

		mbed_err rc = 0;

		// connect the socket to the specified end point
		if ((rc = TcpSocket::connect(ep)) < 0)
			goto abort;

		if ((rc = ::tlsctx_configure(_tlsctx.get(), _tlscfg.get_cfg())) != 0)
			goto abort;

		if ((rc = ::tlsctx_set_netctx(_tlsctx.get(), get_ctx())) != 0)
			goto abort;

	abort:
		if (_logger->is_debug_enabled())
			_logger->debug(
				"... %x leave TlsSocket::connect fd=%d rc=%d",
				(uintptr_t)this,
				get_fd(),
				rc
			);

		return rc;
	}

	tls_handshake_status TlsSocket::handshake(Timer& timer)
	{
		DEBUG_ENTER(_logger, "TlsSocket", "handshake");

		tls_handshake_status handshake_status;

		do {
			// perform handshake with the SSL server
			handshake_status = ::tlsctx_handshake(_tlsctx.get());

			if (handshake_status.status_code == SSLCTX_HDK_WAIT_IO) {
				const netctx_poll_status poll_status = poll(true, true, timer.remaining_delay());

				if (poll_status.code != NETCTX_POLL_OK) {
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
					handshake_status.errnum = MBEDTLS_ERR_SSL_TIMEOUT;
				}
				else {
					//TODO: replace by a call to mbedtls_net_usleep
					::Sleep(100);
				}
			}
		} while (handshake_status.status_code == SSLCTX_HDK_WAIT_IO ||
			handshake_status.status_code == SSLCTX_HDK_WAIT_ASYNC);

		if (_logger->is_debug_enabled())
			_logger->debug(
				"... %x leave TlsSocket::handshake status=%d error=%d",
				(uintptr_t)this,
				handshake_status.status_code,
				handshake_status.errnum
			);

		return handshake_status;
	}


    mbed_err TlsSocket::close()
    {
		DEBUG_ENTER(_logger, "TlsSocket", "close");

		// ignore all errors while closing the connection
		::tlsctx_close(_tlsctx.get());
		TcpSocket::close();

		return 0;
	}


	mbed_err TlsSocket::get_crt_check() const
	{
		return mbedtls_ssl_get_verify_result(_tlsctx.get());
	}


	std::string TlsSocket::get_ciphersuite() const
	{
		return mbedtls_ssl_get_ciphersuite(_tlsctx.get());
	}


	std::string TlsSocket::get_tls_version() const
	{
		return mbedtls_ssl_get_version(_tlsctx.get());
	}


	const mbedtls_x509_crt* TlsSocket::get_peer_crt() const
	{
		return mbedtls_ssl_get_peer_cert(_tlsctx.get());
	}


	netctx_rcv_status TlsSocket::recv(unsigned char* buf, const size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter TlsSocket::recv buffer=%x len=%d", this, buf, len);

		return ::tlsctx_recv(_tlsctx.get(), buf, len);
	}


	netctx_snd_status TlsSocket::send(const unsigned char* buf, const size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter TlsSocket::send buffer=%x len=%d", this, buf, len);

		return ::tlsctx_send(_tlsctx.get(), buf, len);
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

}
