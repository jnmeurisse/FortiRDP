/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "TlsSocket.h"


namespace net {

	using namespace tools;


	TlsSocket::TlsSocket(const TlsConfig& tls_config) :
		TcpSocket(),
		_tlscfg{ tls_config },
		_enable_hostname_verification{ false }
	{
		DEBUG_CTOR(_logger, "TlsSocket");
	}


	TlsSocket::~TlsSocket()
	{
		DEBUG_DTOR(_logger, "TlsSocket");
	}


	void TlsSocket::set_hostname_verification(bool enable_verification)
	{
		_enable_hostname_verification = enable_verification;
	}


	mbed_err TlsSocket::connect(const Endpoint& ep, Timer& timer)
	{
		if (_logger->is_debug_enabled())
			_logger->debug(
				"... %x enter TlsSocket::connect ep=%s",
				(uintptr_t)this,
				ep.to_string().c_str()
			);

		mbed_err rc = 0;

		// connect the socket to the specified end point
		rc = TcpSocket::connect(ep, timer);
		if (rc)
			goto abort;

		rc = _tlsctx.configure(*_tlscfg.get_cfg(), *netctx());
		if (rc)
			goto abort;

		(rc = _tlsctx.set_hostname(ep.hostname()));
		if (rc)
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
			if (_logger->is_trace_enabled())
				_logger->trace(
					".... %x TlsSocket  - call tlsctx_handshake",
					(uintptr_t)this
				);

			// perform handshake with the SSL server
			handshake_status = _tlsctx.handshake();

			if (_logger->is_trace_enabled())
				_logger->trace(
					".... %x TlsSocket - return tlsctx.handshake, status_code=%d rc=%d",
					(uintptr_t)this,
					handshake_status.status_code,
					handshake_status.rc
				);

			if (handshake_status.status_code == hdk_status_code::SSLCTX_HDK_WAIT_IO) {
				const poll_status poll_status = poll(handshake_status.rc, timer.remaining_time());

				if (poll_status.code != poll_status_code::NETCTX_POLL_OK) {
					// Polling error
					handshake_status.status_code = hdk_status_code::SSLCTX_HDK_ERROR;
					handshake_status.rc = poll_status.rc;
				}
				else
					// Noop, poll succeeded
					;
			}
			else if (handshake_status.status_code == hdk_status_code::SSLCTX_HDK_WAIT_ASYNC) {
				if (timer.is_elapsed()) {
					handshake_status.status_code = hdk_status_code::SSLCTX_HDK_ERROR;
					handshake_status.rc = MBEDTLS_ERR_SSL_TIMEOUT;
				}
				else {
					::Sleep(100);
				}
			}
		} while (handshake_status.status_code == hdk_status_code::SSLCTX_HDK_WAIT_IO ||
			handshake_status.status_code == hdk_status_code::SSLCTX_HDK_WAIT_ASYNC);

		if (_logger->is_debug_enabled())
			_logger->debug(
				"... %x leave TlsSocket::handshake status=%d error=%d",
				(uintptr_t)this,
				handshake_status.status_code,
				handshake_status.rc
			);

		return handshake_status;
	}


    void TlsSocket::shutdown()
    {
		if (_logger->is_debug_enabled())
			_logger->debug(
				"... %x enter TlsSocket::shutdown fd=%d",
				(uintptr_t)this,
				get_fd()
			);

		if (is_connected()) {
			// Notify the peer that the connection is being closed.
			mbed_err rc = _tlsctx.close();
			if (rc)
				_logger->error("ERROR: close notify error %d", rc);

			// shutdown and close the socket.
			TcpSocket::shutdown();
		}

		// Free all memory allocated by the library to manage the tls context.
		_tlsctx.clear();

		return;
	}


	mbed_err TlsSocket::get_crt_check() const
	{
		return _tlsctx.get_crt_check();
	}


	std::string TlsSocket::get_ciphersuite() const
	{
		return _tlsctx.get_ciphersuite();
	}


	std::string TlsSocket::get_tls_version() const
	{
		return _tlsctx.get_tls_version();
	}


	const mbedtls_x509_crt* TlsSocket::get_peer_crt() const
	{
		return _tlsctx.get_peer_crt();
	}


	net::rcv_status TlsSocket::recv_data(unsigned char* buf, const size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter TlsSocket::recv_data buffer=%x len=%zu", this, buf, len);

		return _tlsctx.recv_data(buf, len);
	}


	net::snd_status TlsSocket::send_data(const unsigned char* buf, const size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter TlsSocket::send_data buffer=%x len=%zu", this, buf, len);

		return _tlsctx.send_data(buf, len);
	}

}
