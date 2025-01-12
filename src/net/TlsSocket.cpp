/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "TlsSocket.h"

#include <mbedtls/ssl_internal.h>
#include <mbedtls/ssl_ciphersuites.h>
#include <mbedtls/debug.h>


namespace net {

	using namespace tools;

	static void mbedtls_debug_fn(void *ctx, int level,
		const char *file, int line, const char *str)
	{
		((void)level);
		Logger* logger = (tools::Logger *)ctx;

		if (logger && strlen(str) > 1) {
			// remove \n from str
			std::string message{ str };
			message[message.size() - 1] = '\0';

			// get filename from whole path
			std::string path{ file };
			std::string filename;
			const size_t last_delim = path.find_last_of('\\');

			if (last_delim == std::wstring::npos) {
				filename = path;
			}
			else {
				filename = path.substr(last_delim + 1);
			}

			logger->trace(
				".... %s:%04d: %s", filename.c_str(), line, message.c_str());
		}
	}


	// Recommended ciphers from https://ciphersuite.info. 
	//    Key exchange   : elliptic curve diffie-hellman key exchange
	//    Authentication : RSA
	//    Encryption     : CHACHA20 or AES
	//    Message auth   : SHA256 
	static const int default_ciphers[] = {
		// recommended
		MBEDTLS_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
		MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,

		// RFC 6460 : suite B TLS 1.2
		MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
		MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,

		// secure (no perfect Forward Secrecy)
		MBEDTLS_TLS_RSA_WITH_AES_128_GCM_SHA256,
		MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA256,

		// mandatory supported by all tls 1.2 server (RFC5246)
		MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA,

		// end of list
		0
	};


	TlsSocket::TlsSocket() :
		Socket()
	{
		DEBUG_CTOR(_logger, "TlsSocket");

		mbedtls_entropy_init(&_entropy_ctx);

		mbedtls_ctr_drbg_init(&_ctr_drbg);
		mbedtls_ctr_drbg_seed(&_ctr_drbg, mbedtls_entropy_func, &_entropy_ctx, nullptr, 0);

		mbedtls_ssl_config_init(&_ssl_config);
		mbedtls_ssl_config_defaults(&_ssl_config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
		mbedtls_ssl_conf_authmode(&_ssl_config, MBEDTLS_SSL_VERIFY_REQUIRED);
		mbedtls_ssl_conf_rng(&_ssl_config, mbedtls_ctr_drbg_random, &_ctr_drbg);

		// 1.1 and 1.2 are accepted
		mbedtls_ssl_conf_min_version(&_ssl_config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_2);
		mbedtls_ssl_conf_max_version(&_ssl_config, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);

		// set cipher list
		mbedtls_ssl_conf_ciphersuites(&_ssl_config, default_ciphers);

		if (_logger->is_trace_enabled()) {
			// define a debug callback
			mbedtls_ssl_conf_dbg(&_ssl_config, mbedtls_debug_fn, _logger);
#ifndef _DEBUG
			mbedtls_debug_set_threshold(0);
#else
			mbedtls_debug_set_threshold(1);
#endif
		}

		mbedtls_ssl_init(&_ssl_context);
		_enable_hostname_verification = false;
	}


	TlsSocket::~TlsSocket()
	{
		DEBUG_DTOR(_logger, "TlsSocket");

		// close the socket if not yet done
		close();

		// free all memory allocated by SSL library
		mbedtls_ssl_config_free(&_ssl_config);
		mbedtls_ctr_drbg_free(&_ctr_drbg);
		mbedtls_entropy_free(&_entropy_ctx);
		mbedtls_ssl_free(&_ssl_context);
	}


	void TlsSocket::set_ca_crt(mbedtls_x509_crt* ca_crt)
	{
		DEBUG_ENTER(_logger, "TlsSocket", "set_ca_ctr");

		mbedtls_ssl_conf_ca_chain(&_ssl_config, ca_crt, nullptr);
		mbedtls_ssl_conf_authmode(&_ssl_config, MBEDTLS_SSL_VERIFY_OPTIONAL);
	}


	const mbedtls_x509_crt* TlsSocket::get_ca_crt() const
	{
		return _ssl_config.ca_chain;
	}


	mbed_err TlsSocket::set_user_crt(mbedtls_x509_crt* own_crt, mbedtls_pk_context *own_key)
	{
		return mbedtls_ssl_conf_own_cert(&_ssl_config, own_crt, own_key);
	}


	void TlsSocket::set_hostname_verification(bool enable_verification)
	{
		_enable_hostname_verification = enable_verification;
	}


	mbed_err TlsSocket::connect(const Endpoint& ep)
	{
		if (_logger->is_debug_enabled())
			_logger->debug("... %x enter TlsSocket::connect ep=%s", (uintptr_t)this, ep.to_string().c_str());

		mbed_err rc = 0;

		// connect the socket to the specified end point
		if ((rc = Socket::connect(ep)) < 0)
			goto abort;

		// configure the ssl context
		if ((rc = mbedtls_ssl_setup(&_ssl_context, &_ssl_config)) != 0)
			goto abort;
		mbedtls_ssl_set_bio(&_ssl_context, &_netctx, mbedtls_net_send, mbedtls_net_recv, NULL);

		// enable/disable hostname verification
		if ((rc = mbedtls_ssl_set_hostname(&_ssl_context, _enable_hostname_verification? ep.hostname().c_str() : nullptr)) != 0)
			goto abort;

		// perform handshake with the SSL server
		if ((rc = mbedtls_ssl_handshake(&_ssl_context)) != 0)
			goto abort;

	abort:
		if (_logger->is_debug_enabled())
			_logger->debug("... %x leave TlsSocket::connect rc=%d", (uintptr_t)this, rc);
		return rc;
	}


	mbed_err TlsSocket::get_crt_check() const
	{
		return mbedtls_ssl_get_verify_result(&_ssl_context);
	}


	std::string TlsSocket::get_ciphersuite() const
	{
		return mbedtls_ssl_get_ciphersuite(&_ssl_context);
	}


	std::string TlsSocket::get_tls_version() const
	{
		return mbedtls_ssl_get_version(&_ssl_context);
	}


	const mbedtls_x509_crt* TlsSocket::get_peer_crt() const
	{
		return mbedtls_ssl_get_peer_cert(&_ssl_context);
	}


	int TlsSocket::recv(unsigned char* buf, const size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter TlsSocket::recv buffer=%x len=%d", (uintptr_t)this, (uintptr_t)buf, len);

		return mbedtls_ssl_read(&_ssl_context, buf, len);
	}


	int TlsSocket::send(const unsigned char* buf, const size_t len)
	{
		if (_logger->is_trace_enabled())
			_logger->trace(".... %x enter TlsSocket::send buffer=%x len=%d", (uintptr_t)this, (uintptr_t)buf, len);

		return mbedtls_ssl_write(&_ssl_context, buf, len);
	}


	mbed_err TlsSocket::flush()
	{
		DEBUG_ENTER(_logger, "TlsSocket", "flush");
		return mbedtls_ssl_flush_output(&_ssl_context);
	}


	void TlsSocket::do_close()
	{
		DEBUG_ENTER(_logger, "TlsSocket", "do_close");

		if (connected() && _ssl_context.p_bio) {
			mbedtls_ssl_close_notify(&_ssl_context);
			mbedtls_ssl_session_reset(&_ssl_context);
		}

		Socket::do_close();
	}

}
