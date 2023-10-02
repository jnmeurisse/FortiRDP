/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#include <stdlib.h>
#include <string.h>

#include "mbedccl/sslctx.h"

#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"


sslctx* sslctx_alloc()
{
	mbedtls_ssl_context* ctx = malloc(sizeof(mbedtls_ssl_context));

	if (ctx) {
		mbedtls_ssl_init(ctx);
	}

	return (sslctx*) ctx;
}


mbed_err sslctx_config(sslctx* ctx, sslcfg* cfg)
{
	int rc = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	mbedtls_ssl_context* const ssl_context = (mbedtls_ssl_context*)ctx;
	mbedtls_ssl_config* const ssl_config = (mbedtls_ssl_config *)cfg;

	if (ssl_context && ssl_config) {
		//TODO: Do we need to call psa_crypto_init() ?
		rc = mbedtls_ssl_setup(ssl_context, ssl_config);
	}

	return rc;
}


void sslctx_free(sslctx* ctx)
{
	mbedtls_ssl_context* const ssl_context = (mbedtls_ssl_context*)ctx;

	if (ssl_context) {
		mbedtls_ssl_free(ssl_context);
		free(ssl_context);
	}
}


mbed_err sslctx_set_netctx(sslctx* ctx, netctx* netctx)
{
	mbed_err rc = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	mbedtls_ssl_context* const ssl_context = (mbedtls_ssl_context*)ctx;
	mbedtls_net_context* const net_context = (mbedtls_net_context*)netctx;

	if (ssl_context && net_context) {
		mbedtls_ssl_set_bio(ssl_context, net_context, mbedtls_net_send, mbedtls_net_recv, NULL);
		rc = 0;
	}

	return rc;
}


sslctx_handshake_status sslctx_handshake(sslctx* ctx)
{
	sslctx_handshake_status status = { SSLCTX_HDK_ERROR, MBEDTLS_ERR_SSL_BAD_INPUT_DATA};
	mbedtls_ssl_context* const ssl_context = (mbedtls_ssl_context*)ctx;

	if (ssl_context) {
		status.errnum = mbedtls_ssl_handshake(ssl_context);
		switch (status.errnum) {
		case 0:
			status.status_code = SSLCTX_HDK_OK;
			break;

		case MBEDTLS_ERR_SSL_WANT_READ:
		case MBEDTLS_ERR_SSL_WANT_WRITE:
			status.status_code = SSLCTX_HDK_WAIT_IO;
			break;

		case MBEDTLS_ERR_SSL_ASYNC_IN_PROGRESS:
		case MBEDTLS_ERR_SSL_CRYPTO_IN_PROGRESS:
			status.status_code = SSLCTX_HDK_WAIT_ASYNC;
			break;

		default:
			status.status_code = SSLCTX_HDK_ERROR;
		}
	}

	return status;
}


mbed_err sslctx_close(sslctx* ctx)
{
	mbed_err rc = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
	mbedtls_ssl_context* const ssl_context = (mbedtls_ssl_context*)ctx;

	if (ssl_context) {
		rc = mbedtls_ssl_close_notify(ssl_context);
		if (rc)
			goto terminate;
	}

terminate:
	return rc;
}



x509crt* sslctx_get_peer_x509crt(sslctx* ctx)
{
	x509crt* crt = 0;
	mbedtls_ssl_context* const ssl_context = (mbedtls_ssl_context*)ctx;

	if (ssl_context) {
		crt = (x509crt*)mbedtls_ssl_get_peer_cert(ssl_context);
	}

	return crt;
}


netctx_snd_status sslctx_send(sslctx* ctx, const unsigned char* buf, size_t len)
{
	netctx_snd_status status = { NETCTX_SND_ERROR, MBEDTLS_ERR_SSL_BAD_INPUT_DATA, 0 };
	mbedtls_ssl_context* const ssl_context = (mbedtls_ssl_context*)ctx;

	if (ssl_context) {
		int rc = mbedtls_ssl_write(ssl_context, buf, len);

		if (rc > 0) {
			status.status_code = NETCTX_SND_OK;
			status.errnum = 0;
			status.sbytes = rc;
		}
		else if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
			status.status_code = NETCTX_SND_RETRY;
			status.errnum = (mbed_err)rc;
		}
		else {
			status.status_code = NETCTX_SND_ERROR;
			status.errnum = (mbed_err)rc;
		}
	}

	return status;
}


netctx_rcv_status sslctx_recv(sslctx* ctx, unsigned char* buf, size_t len)
{
	netctx_rcv_status status = { NETCTX_RCV_ERROR, MBEDTLS_ERR_SSL_BAD_INPUT_DATA, 0 };
	mbedtls_ssl_context* const ssl_context = (mbedtls_ssl_context*)ctx;

	if (ssl_context) {
		int rc = mbedtls_ssl_read(ssl_context, buf, len);

		if (rc > 0) {
			status.status_code = NETCTX_RCV_OK;
			status.errnum = 0;
			status.rbytes = rc;
		}
		else if (rc == 0) {
			status.status_code = NETCTX_RCV_EOF;
			status.errnum = 0;
		}
		else if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
			status.status_code = NETCTX_RCV_RETRY;
			status.errnum = (mbed_err)rc;
		}
		else {
			status.status_code = NETCTX_RCV_ERROR;
			status.errnum = (mbed_err)rc;
		}
	}

	return status;
}


uint32_t sslctx_get_verify_result(sslctx* ctx)
{
	mbedtls_ssl_context* const ssl_context = (mbedtls_ssl_context*)ctx;

	return ssl_context
		? mbedtls_ssl_get_verify_result(ssl_context)
		: 0xFFFFFFFF;
}



const char* sslctx_get_ciphersuite(sslctx* ctx)
{
	mbedtls_ssl_context* const ssl_context = (mbedtls_ssl_context*)ctx;

	return ssl_context
		? mbedtls_ssl_get_ciphersuite(ssl_context)
		: "not available";
}


const char* sslctx_get_version(sslctx* ctx)
{
	mbedtls_ssl_context* const ssl_context = (mbedtls_ssl_context*)ctx;

	return ssl_context
		? mbedtls_ssl_get_version(ssl_context)
		: "not available";
}


