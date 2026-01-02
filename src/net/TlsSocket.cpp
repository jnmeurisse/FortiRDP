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
		DEBUG_CTOR(_logger);
	}


	TlsSocket::~TlsSocket()
	{
		DEBUG_DTOR(_logger);
	}


	void TlsSocket::set_hostname_verification(bool enable_verification)
	{
		_enable_hostname_verification = enable_verification;
	}


	mbed_err TlsSocket::connect(const Endpoint& ep, const Timer& timer)
	{
		DEBUG_ENTER_FMT(_logger, "ep=%s", ep.to_string().c_str());

		mbed_err rc = TcpSocket::connect(ep, timer);
		if (rc)
			goto terminate;

		rc = _tlsctx.configure(*_tlscfg.get_cfg(), *netctx());
		if (rc)
			goto terminate;

		rc = _tlsctx.set_hostname(ep.hostname());
		if (rc)
			goto terminate;

	terminate:
		LOG_DEBUG(_logger, "fd=%d rc=%d", get_fd(), rc);
			
		return rc;
	}


	tls_handshake_status TlsSocket::handshake(const Timer& timer)
	{
		DEBUG_ENTER(_logger);

		tls_handshake_status handshake_status;
		do {
			LOG_TRACE(_logger, "call tlsctx.handshake");

			handshake_status = _tlsctx.handshake();

			LOG_TRACE(_logger, "return from tlsctx.handshake, status_code=%d rc=%d",
				handshake_status.status_code,
				handshake_status.rc
			);

			if (handshake_status.status_code == hdk_status_code::SSLCTX_HDK_WAIT_IO) {
				const poll_status poll_status = poll(handshake_status.rc, timer.remaining_time());

				if (poll_status.code != poll_status_code::NETCTX_POLL_OK) {
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

		LOG_DEBUG(_logger, "status=%d error=%d",
			handshake_status.status_code,
			handshake_status.rc
		);

		return handshake_status;
	}


    void TlsSocket::shutdown()
    {
		DEBUG_ENTER_FMT(_logger, "fd=%d", get_fd());

		if (is_connected()) {
			// Notify the peer that the connection is being closed.
			mbed_err rc = _tlsctx.close();
			if (rc)
				_logger->error("ERROR: close notify error (%d)", rc);

			// shutdown and close the socket.
			TcpSocket::shutdown();
		}

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


	const TlsConfig& TlsSocket::get_tls_config() const
	{
		return _tlscfg;
	}


	net::rcv_status TlsSocket::recv_data(unsigned char* buf, const size_t len)
	{
		TRACE_ENTER_FMT(_logger, "buffer=0x%012Ix size=%zu", PTR_VAL(buf), len);
		return _tlsctx.recv_data(buf, len);
	}


	net::snd_status TlsSocket::send_data(const unsigned char* buf, const size_t len)
	{
		TRACE_ENTER_FMT(_logger, "buffer=0x%012Ix size=%zu", PTR_VAL(buf), len);
		return _tlsctx.send_data(buf, len);
	}


	const char* TlsSocket::__class__ = "TlsSocket";

}
