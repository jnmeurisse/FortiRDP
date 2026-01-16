/*!
* This file is part of FortiRDP
*
* Copyright (C) 2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "TlsContext.h"

namespace net {

	TlsContext::TlsContext() :
		_sslctx{}
	{
		::mbedtls_ssl_init(&_sslctx);
	}


	TlsContext::~TlsContext()
	{
		clear();
	}


	utl::mbed_err TlsContext::configure(const mbedtls_ssl_config& config, mbedtls_net_context& netctx)
	{
		::mbedtls_ssl_set_bio(&_sslctx, &netctx, mbedtls_net_send, mbedtls_net_recv, nullptr);

		return ::mbedtls_ssl_setup(&_sslctx, &config);
	}


	void TlsContext::clear()
	{
		::mbedtls_ssl_free(&_sslctx);
	}


	utl::mbed_err TlsContext::set_hostname(const std::string& hostname)
	{
		return ::mbedtls_ssl_set_hostname(&_sslctx, hostname.c_str());
	}


	utl::mbed_err TlsContext::close()
	{
		int rc;

		do {
			rc = ::mbedtls_ssl_close_notify(&_sslctx);
		} while (rc == MBEDTLS_ERR_SSL_WANT_WRITE);

		return rc;
	}


	net::tls_handshake_status TlsContext::handshake()
	{
		tls_handshake_status status { hdk_status_code::SSLCTX_HDK_ERROR, MBEDTLS_ERR_SSL_BAD_INPUT_DATA };

		status.rc = ::mbedtls_ssl_handshake(&_sslctx);
		switch (status.rc) {
		case 0:
			status.status_code = hdk_status_code::SSLCTX_HDK_OK;
			break;

		case MBEDTLS_ERR_SSL_WANT_READ:
			status.status_code = hdk_status_code::SSLCTX_HDK_WAIT_IO;
			status.rc = MBEDTLS_NET_POLL_READ;
			break;

		case MBEDTLS_ERR_SSL_WANT_WRITE:
			status.status_code = hdk_status_code::SSLCTX_HDK_WAIT_IO;
			status.rc = MBEDTLS_NET_POLL_WRITE;
			break;

		case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
		case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
			status.status_code = hdk_status_code::SSLCTX_HDK_WAIT_ASYNC;
			break;

		default:
			status.status_code = hdk_status_code::SSLCTX_HDK_ERROR;
		}

		return status;
	}


	net::rcv_status TlsContext::recv_data(unsigned char* buf, size_t len)
	{
		rcv_status status { rcv_status_code::NETCTX_RCV_ERROR, MBEDTLS_ERR_SSL_BAD_INPUT_DATA, 0 };

		const int rc = ::mbedtls_ssl_read(&_sslctx, buf, len);

		if (rc > 0) {
			status.code = rcv_status_code::NETCTX_RCV_OK;
			status.rc = 0;
			status.rbytes = rc;
		}
		else if (rc == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || rc == 0) {
			status.code = rcv_status_code::NETCTX_RCV_EOF;
			status.rc = 0;
		}
		else if (rc == MBEDTLS_ERR_SSL_WANT_READ) {
			status.code = rcv_status_code::NETCTX_RCV_RETRY;
			status.rc = MBEDTLS_NET_POLL_READ;
		}
		else if (rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
			status.code = rcv_status_code::NETCTX_RCV_RETRY;
			status.rc = MBEDTLS_NET_POLL_WRITE;
		}
		else {
			status.code = rcv_status_code::NETCTX_RCV_ERROR;
			status.rc = rc;
		}

		return status;
	}


	net::snd_status TlsContext::send_data(const unsigned char* buf, size_t len)
	{
		snd_status status{ snd_status_code::NETCTX_SND_ERROR, MBEDTLS_ERR_SSL_BAD_INPUT_DATA, 0 };

		const int rc = ::mbedtls_ssl_write(&_sslctx, buf, len);

		if (rc > 0) {
			status.code = snd_status_code::NETCTX_SND_OK;
			status.rc = 0;
			status.sbytes = rc;
		}
		else if (rc == MBEDTLS_ERR_SSL_WANT_READ) {
			status.code = snd_status_code::NETCTX_SND_RETRY;
			status.rc = MBEDTLS_NET_POLL_READ;
		}
		else if (rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
			status.code = snd_status_code::NETCTX_SND_RETRY;
			status.rc = MBEDTLS_NET_POLL_WRITE;
		}
		else {
			status.code = snd_status_code::NETCTX_SND_ERROR;
			status.rc = rc;
		}

		return status;
	}


	utl::mbed_err TlsContext::get_crt_check() const
	{
		return ::mbedtls_ssl_get_verify_result(&_sslctx);
	}


	std::string TlsContext::get_ciphersuite() const
	{
		return ::mbedtls_ssl_get_ciphersuite(&_sslctx);
	}


	std::string TlsContext::get_tls_version() const
	{
		return ::mbedtls_ssl_get_version(&_sslctx);
	}


	const mbedtls_x509_crt* TlsContext::get_peer_crt() const
	{
		return ::mbedtls_ssl_get_peer_cert(&_sslctx);
	}

}
