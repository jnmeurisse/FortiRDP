/*!
* This file is part of FortiRDP
*
* Copyright (C) 2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "TlsConfig.h"
#include <mbedtls/debug.h>


namespace net {

	static void mbedtls_debug_fn(void* ctx, int level,
		const char* file, int line, const char* str)
	{
		((void)level);
		tools::Logger* const logger = static_cast<tools::Logger*>(ctx);

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

			logger->trace("%s:%04d: %s", filename.c_str(), line, message.c_str());
		}
	}


	// Recommended ciphers from https://ciphersuite.info. 
	static const int default_ciphers[] = {
		// TLS 1.3 cipher suites
		MBEDTLS_TLS1_3_CHACHA20_POLY1305_SHA256,
		MBEDTLS_TLS1_3_AES_128_GCM_SHA256,

		// TLS 1.2 ciphe rsuites
		//    Key exchange   : elliptic curve diffie-hellman key exchange
		//    Authentication : RSA
		//    Encryption     : CHACHA20 or AES
		//    Message auth   : SHA256 
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


	TlsConfig::TlsConfig() :
		_logger(tools::Logger::get_logger())
	{
		DEBUG_CTOR(_logger);
		mbedtls_entropy_init(&_entropy_ctx);

		mbedtls_ctr_drbg_init(&_ctr_drbg);
		mbedtls_ctr_drbg_seed(&_ctr_drbg, mbedtls_entropy_func, &_entropy_ctx, nullptr, 0);

		mbedtls_ssl_config_init(&_ssl_config);
		mbedtls_ssl_config_defaults(&_ssl_config, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
		mbedtls_ssl_conf_authmode(&_ssl_config, MBEDTLS_SSL_VERIFY_REQUIRED);
		mbedtls_ssl_conf_rng(&_ssl_config, mbedtls_ctr_drbg_random, &_ctr_drbg);

		// 1.2 and 1.3 are accepted
		mbedtls_ssl_conf_min_tls_version(&_ssl_config, MBEDTLS_SSL_VERSION_TLS1_2);

		// set cipher list
		mbedtls_ssl_conf_ciphersuites(&_ssl_config, default_ciphers);
		
#if defined _DEBUG
		// verify if the ciphers are available.  The MbedTLS configuration is
		// complex and it could be possible to define ciphers that are not
		// configured or disabled.
		for (int idx = 0; default_ciphers[idx] != 0; idx++) {
			if (!mbedtls_ssl_ciphersuite_from_id(default_ciphers[idx]))
				_logger->error("INTERNAL ERROR: missing cipher index=%d id=%d", idx, default_ciphers[idx]);
		}
#endif

		if (_logger->is_trace_enabled()) {
			// define a debug callback
			mbedtls_ssl_conf_dbg(&_ssl_config, mbedtls_debug_fn, _logger);
#ifndef _DEBUG
			mbedtls_debug_set_threshold(0);
#else
			mbedtls_debug_set_threshold(2);
#endif
		}

	}

	TlsConfig::~TlsConfig()
	{
		DEBUG_DTOR(_logger);

		// free all memory allocated by SSL library
		mbedtls_ssl_config_free(&_ssl_config);
		mbedtls_ctr_drbg_free(&_ctr_drbg);
		mbedtls_entropy_free(&_entropy_ctx);
	}

    
	void TlsConfig::set_ca_crt(mbedtls_x509_crt& ca_crt)
	{
		DEBUG_ENTER(_logger);

		mbedtls_ssl_conf_ca_chain(&_ssl_config, &ca_crt, nullptr);
		mbedtls_ssl_conf_authmode(&_ssl_config, MBEDTLS_SSL_VERIFY_OPTIONAL);
	}


	mbed_err TlsConfig::set_user_crt(mbedtls_x509_crt& own_crt, mbedtls_pk_context& own_key)
	{
		DEBUG_ENTER(_logger);

		return mbedtls_ssl_conf_own_cert(&_ssl_config, &own_crt, &own_key);
	}


	const mbedtls_ssl_config* TlsConfig::get_cfg() const
	{
		return &_ssl_config;
	}


	const char* TlsConfig::__class__ = "TlsConfig";
}
