/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <stdlib.h>

#include <mbedtls/ssl.h>
#include "TlsContext.h"


mbedtls_ssl_context* tlsctx_alloc()
{
	mbedtls_ssl_context* const ctx = malloc(sizeof(mbedtls_ssl_context));

	if (ctx)
		mbedtls_ssl_init(ctx);

	return ctx;
}


void tlsctx_free(mbedtls_ssl_context* ctx)
{
	if (ctx) {
		mbedtls_ssl_free(ctx);
		free(ctx);
	}
}


int tlsctx_configure(mbedtls_ssl_context* ctx, const mbedtls_ssl_config* cfg)
{
	int rc = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;

	if (ctx && cfg) {
		rc = mbedtls_ssl_setup(ctx, cfg);
	}

	return rc;
}


int tlsctx_set_netctx(mbedtls_ssl_context* ctx, mbedtls_net_context* netctx)
{
	int rc = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;

	if (ctx && netctx) {
		mbedtls_ssl_set_bio(ctx, netctx, mbedtls_net_send, mbedtls_net_recv, NULL);
		rc = 0;
	}

	return rc;
}


tls_handshake_status tlsctx_handshake(mbedtls_ssl_context* ctx)
{
	tls_handshake_status status = { SSLCTX_HDK_ERROR, MBEDTLS_ERR_SSL_BAD_INPUT_DATA};

	if (ctx) {
		status.errnum = mbedtls_ssl_handshake(ctx);
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


int tlsctx_close(mbedtls_ssl_context* ctx)
{
	int rc = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;

	if (ctx) {
		do {
			rc = mbedtls_ssl_close_notify(ctx);
		} while (rc == MBEDTLS_ERR_SSL_WANT_WRITE);
	}

	return rc;
}


netctx_snd_status tlsctx_send(mbedtls_ssl_context* ctx, const unsigned char* buf, size_t len)
{
	netctx_snd_status status = { NETCTX_SND_ERROR, MBEDTLS_ERR_SSL_BAD_INPUT_DATA, 0 };

	if (ctx) {
		int rc = mbedtls_ssl_write(ctx, buf, len);

		if (rc > 0) {
			status.code = NETCTX_SND_OK;
			status.errnum = 0;
			status.sbytes = rc;
		}
		else if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
			status.code = NETCTX_SND_RETRY;
			status.errnum = rc;
		}
		else {
			status.code = NETCTX_SND_ERROR;
			status.errnum = rc;
		}
	}

	return status;
}


netctx_rcv_status tlsctx_recv(mbedtls_ssl_context* ctx, unsigned char* buf, size_t len)
{
	netctx_rcv_status status = { NETCTX_RCV_ERROR, MBEDTLS_ERR_SSL_BAD_INPUT_DATA, 0 };

	if (ctx) {
		int rc = mbedtls_ssl_read(ctx, buf, len);

		if (rc > 0) {
			status.code = NETCTX_RCV_OK;
			status.errnum = 0;
			status.rbytes = rc;
		}
		else if (rc == 0) {
			status.code = NETCTX_RCV_EOF;
			status.errnum = 0;
		}
		else if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE) {
			status.code = NETCTX_RCV_RETRY;
			status.errnum = rc;
		}
		else {
			status.code = NETCTX_RCV_ERROR;
			status.errnum = rc;
		}
	}

	return status;
}
