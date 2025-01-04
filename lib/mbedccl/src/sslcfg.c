/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#include <stdlib.h>
#include <string.h>

#include "mbedccl/sslcfg.h"
#include "rngctx_private.h"
#include "debug_private.h"


static const int default_ciphers[] = {
	MBEDTLS_TLS1_3_CHACHA20_POLY1305_SHA256,
	MBEDTLS_TLS1_3_AES_128_GCM_SHA256,
	MBEDTLS_TLS1_3_AES_256_GCM_SHA384,
	0
};


sslcfg* sslcfg_alloc()
{
	mbedtls_ssl_config* const ssl_config = malloc(sizeof(mbedtls_ssl_config));

	if (ssl_config)
		mbedtls_ssl_config_init(ssl_config);

	return (sslcfg*)ssl_config;
}


void sslcfg_free(sslcfg* cfg)
{
	mbedtls_ssl_config* const ssl_config = (mbedtls_ssl_config*)cfg;

	if (ssl_config) {
		// free all memory allocated by MBED tls library
		mbedtls_ssl_config_free(ssl_config);
		free(ssl_config);
	}
}


mbed_errnum sslcfg_configure(sslcfg* cfg, rngctx* ctx)
{
	mbed_errnum errnum = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	mbedtls_ssl_config* const ssl_config = (mbedtls_ssl_config*)cfg;

	if (ssl_config && ctx) {
		errnum = mbedtls_ssl_config_defaults(
			ssl_config,
			MBEDTLS_SSL_IS_CLIENT,
			MBEDTLS_SSL_TRANSPORT_STREAM,
			MBEDTLS_SSL_PRESET_DEFAULT);
		if (errnum)
			goto terminate;

		// override defaults
		mbedtls_ssl_conf_authmode(
			ssl_config,
			MBEDTLS_SSL_VERIFY_REQUIRED);
		mbedtls_ssl_conf_rng(
			ssl_config,
			mbedtls_ctr_drbg_random,
			&ctx->ctr_drbg_context);

		// only 1.3 is accepted (should be the default if mbedtls is compiled with only tls1.3 support)
		mbedtls_ssl_conf_min_tls_version(ssl_config, MBEDTLS_SSL_VERSION_TLS1_3);
		mbedtls_ssl_conf_max_tls_version(ssl_config, MBEDTLS_SSL_VERSION_TLS1_3);

		// set cipher list
		mbedtls_ssl_conf_ciphersuites(ssl_config, default_ciphers);

		// key exchange mode ?
		//mbedtls_ssl_conf_tls13_key_exchange_modes();

		// signatures ?
		// mbedtls_ssl_conf_sig_algs(ssl_config)
	
		// 
	}

terminate:
	return errnum;
}


void sslcfg_enable_debug(sslcfg* cfg, int onoff)
{
	mbedtls_ssl_config* const ssl_config = (mbedtls_ssl_config*)cfg;

	if (ssl_config) {
		mbedtls_ssl_conf_dbg(
			ssl_config,
			onoff ? mbedccl_debug_fn : 0,
			0);
	}
}


mbed_errnum sslfcg_set_ca_chain(sslcfg* cfg, x509crt* crt)
{
	mbed_errnum errnum = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	mbedtls_ssl_config* const ssl_config = (mbedtls_ssl_config*)cfg;
	mbedtls_x509_crt* const x509_crt = (mbedtls_x509_crt *)crt;

	if (ssl_config && x509_crt) {
		mbedtls_ssl_conf_ca_chain(ssl_config, x509_crt, 0);
		errnum = 0;
	}

	return errnum;
}


