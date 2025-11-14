/*!
* This file is part of FortiRDP
*
* Copyright (C) 2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <mbedtls/ssl.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/x509_crt.h>

#include "tools/Logger.h"
#include "tools/ErrUtil.h"


namespace net {
	using namespace tools;

	class TlsConfig {
	public:
		explicit TlsConfig();
		TlsConfig(const TlsConfig& config) = delete;
		~TlsConfig();

		/**
		 * Defines the CA certificates.
		*/
		void set_ca_crt(mbedtls_x509_crt& ca_crt);

		/**
		 * Defines the client certificate.
		*/
		mbed_err set_user_crt(mbedtls_x509_crt& own_crt, mbedtls_pk_context& own_key);

		/**
		 * @return the mbedtls_ssl_config.
		*/
		const mbedtls_ssl_config* get_cfg() const;

	private:
		// A reference to the application logger.
		tools::Logger* const _logger;

		// All data required to initialize a TLS socket.
		mbedtls_entropy_context _entropy_ctx;
		mbedtls_ctr_drbg_context _ctr_drbg;
		mbedtls_ssl_config _ssl_config;
	};
}