/*!
* This file is part of FortiRDP
*
* Copyright (C) 2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <stdint.h>
#include <mbedtls/ssl.h>
#include "net/NetContext.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef enum tlsctx_hdk_status_code {
	SSLCTX_HDK_OK = 0,
	SSLCTX_HDK_WAIT_IO = 1,
	SSLCTX_HDK_WAIT_ASYNC = 2,
	SSLCTX_HDK_ERROR = 3
} tlsctx_hdk_status_code;

typedef struct tlsctx_handshake_status {
	tlsctx_hdk_status_code status_code;
	int errnum;
} tls_handshake_status;


/**
 * Allocate a TLS context.
 * 
 * Return a pointer to a mbedtls_ssl_context or NULL if memory allocation failed.
 */
mbedtls_ssl_context* tlsctx_alloc();

/**
* Free a TLS context.
*/
void tlsctx_free(mbedtls_ssl_context* ctx);

/**
 * Configure a TLS context.
 * 
 * Return 0 if successful, or one of:
 *			MBEDTLS_ERR_SSL_BAD_INPUT_DATA,
 *          MBEDTLS_ERR_SSL_ALLOC_FAILED
 */
int tlsctx_configure(mbedtls_ssl_context* ctx, const mbedtls_ssl_config* cfg);

/**
 * Configure the TLS context for read and write operation.
 * 
 * Return 0 if successful, or MBEDTLS_ERR_SSL_BAD_INPUT_DATA.
 */
int tlsctx_set_netctx(mbedtls_ssl_context* ctx, mbedtls_net_context* netctx);

/**
 * Perform the TLS handshake.
 *
 * The function must be called multiple times until the handshake is complete
 * or an error occurs.
 * 
 * The function returns a compound status.
 *    code
 *       SSLCTX_HDK_OK :
 *          The handshake succeeded.
 *
 *       SSLCTX_HDK_WAIT_IO :
 *          The handshake is incomplete and waiting for data to	be available
 *          for reading from or writing.  The function must be called again
 *          when data are available.
 * 
 *       SSLCTX_HDK_WAIT_ASYNC :
 *          An asynchronous operation is in progress.  The function must be
 *          called again.
 *
 *       SSLCTX_HDK_ERROR :
 *          The handshake failed. The errnum field contains the
 *          error code.
 */
tls_handshake_status tlsctx_handshake(mbedtls_ssl_context* ctx);

/*
 * Notify the peer that the connection is being closed.
 * 
 * Return 0 if successful, or one of:
 *			MBEDTLS_ERR_SSL_BAD_INPUT_DATA,
 *          see mbedtls_ssl_close_notify
 */
int tlsctx_close(mbedtls_ssl_context* ctx);

/**
 * Send data.
 *
 * The function returns a compound status.
 *    code :
 *       SND_OK :
 *          The send operation succeeded, sbytes contains the number of
 *          bytes effectively sent.
 *       SND_RETRY :
 *          No bytes have been sent, the send operation must be retried.
 *          The errnum field contains the mbedTLS error code.
 *       SND_ERROR :
 *           No bytes have been sent, the send operation failed.
 *           The errnum field contains the mbedTLS error code.
 *
 *    errnum : A mbedTLS error code if an error or if a timeout
 *             occurred.

 *    sbytes : The number of bytes sent.
 *
 *
 * Possible values :
 *     code        | SND_OK | SND_RETRY | SND_ERROR
 *     sbytes      | > 0    | = 0       | = 0
 *     errnum      | 0      | < 0       | < 0
 *
 */
netctx_snd_status tlsctx_send(mbedtls_ssl_context* ctx, const unsigned char* buf, size_t len);

/**
 * Receive data.
 *
 * The function returns a compound status.
 *    code :
 *       RCV_OK :
 *          The receive operation succeeded, rbytes contains the number of
 *          bytes received.
 *       RCV_RETRY :
 *          No bytes have been received, the receive operation must be retried.
 *          The errnum field contains the mbedTLS error code
 *       RCV_ERROR :
 *          No bytes have been received, the receive operation failed.
 *          The errnum field contains the mbedTLS error code.
 *       RCV_EOF :
 *          No bytes have been received, the peer closed the network connection.
 *
 *    errnum : A mbedTLS error code if an error or if a timeout
 *             occurred.
 *
 * 
 *    rbytes : The number of bytes received.
 *
 * Possible values :
 *
 *     code        | RCV_OK | RCV_RETRY | RCV_ERROR | RCV_EOF
 *     rbytes      | > 0    | = 0       | = 0       | = 0
 *     errnum      | 0      | < 0       | < 0       | = 0
 *
 */
netctx_rcv_status tlsctx_recv(mbedtls_ssl_context* ctx, unsigned char* buf, size_t len);


#ifdef __cplusplus
}
#endif
