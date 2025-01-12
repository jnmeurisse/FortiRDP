/*!
* This file is part of FortiRDP
*
* Copyright (C) 2025 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <mbedtls/net_sockets.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef enum netctx_protocol {
	NETCTX_PROTO_TCP = MBEDTLS_NET_PROTO_TCP,
	NETCTX_PROTO_UDP = MBEDTLS_NET_PROTO_UDP
} netctx_protocol;


typedef struct netctx_poll_mode {
	unsigned int read : 1;
	unsigned int write : 1;
} netctx_poll_mode;


typedef enum netctx_rcv_status_code {
	NETCTX_RCV_OK = 0,
	NETCTX_RCV_RETRY = 1,
	NETCTX_RCV_ERROR = 2,
	NETCTX_RCV_EOF = 3
} netctx_rcv_status_code;


typedef enum netctx_snd_status_code {
	NETCTX_SND_OK = 0,
	NETCTX_SND_RETRY = 1,
	NETCTX_SND_ERROR = 2
} netctx_snd_status_code;


typedef enum netctx_poll_status_code {
	NETCTX_POLL_OK = 0,
	NETCTX_POLL_TIMEOUT = 1,
	NETCTX_POLL_ERROR = 2
} netctx_poll_status_code;


typedef struct netctx_poll_status {
	netctx_poll_status_code code;
	netctx_poll_mode event;
	int errnum;
} netctx_poll_status;


typedef struct netctx_snd_status {
	netctx_snd_status_code code;
	size_t sbytes;
	int errnum;
} netctx_snd_status;


typedef struct netctx_rcv_status {
	netctx_rcv_status_code code;
	size_t rbytes;
	int errnum;
} netctx_rcv_status;


/**
 * Allocate and initialize a network context.
 * 
 * Return a pointer to a mbedtls_net_context or NULL if memory allocation failed.
 */
mbedtls_net_context* netctx_alloc();

/**
 * Free a network context.
 */
void netctx_free(mbedtls_net_context* ctx);

/**
 * Close a network connection.
 * 
 */
void netctx_close(mbedtls_net_context* ctx);

/**
 * Disable transmission and reception and close the network connection.
 * 
 */
void netctx_shutdown(mbedtls_net_context* ctx);

/**
 * Return the socket file descriptor.
 * 
 * Note: The function returns -1 if the network context is not valid.
 */
int netctx_getfd(mbedtls_net_context* ctx);

/**
 * Initiate a connection with host:port and the given protocol (TCP or UPD).
 * 
 * Return 0 if successful, or one of:
 *			MBEDTLS_ERR_NET_INVALID_CONTEXT,
 *          MBEDTLS_ERR_NET_SOCKET_FAILED,
 *          MBEDTLS_ERR_NET_UNKNOWN_HOST,
 *          MBEDTLS_ERR_NET_CONNECT_FAILED
 */
int netctx_connect(mbedtls_net_context* ctx, const char* hostname, const char* port, netctx_protocol protocol);

/**
 * Create a listening socket on host:port and the given protocol (TCP or UPD).
 * 
 * Return 0 if successful, or one of:
 *			MBEDTLS_ERR_NET_INVALID_CONTEXT,
 *          MBEDTLS_ERR_NET_SOCKET_FAILED,
 *          MBEDTLS_ERR_NET_UNKNOWN_HOST,
 *          MBEDTLS_ERR_NET_BIND_FAILED,
 *          MBEDTLS_ERR_NET_LISTEN_FAILED
*
 */
int netctx_bind(mbedtls_net_context* ctx, const char* hostname, const char* port, netctx_protocol protocol);

/**
 * Accept a connection from a remote client.
 * 
 * Return 0 if successful, or one of:
 *			MBEDTLS_ERR_NET_INVALID_CONTEXT,
 *          MBEDTLS_ERR_NET_SOCKET_FAILED,
 *          MBEDTLS_ERR_NET_BIND_FAILED,
 *          MBEDTLS_ERR_NET_ACCEPT_FAILED
 *          MBEDTLS_ERR_SSL_WANT_READ if bind_ctx was set to
 *              non-blocking and accept() would block.
 *
 */
int netctx_accept(mbedtls_net_context* bind_ctx, mbedtls_net_context* accepting_ctx);

/**
 * Enable or disable the blocking mode.
 * 
 * Return 0 if successful or MBEDTLS_ERR_NET_INVALID_CONTEXT.
 */
int netctx_set_blocking(mbedtls_net_context* ctx, int blocking);

/**
 * Enable or disable the no delay mode.
 * 
 * Return 0 if successful or MBEDTLS_ERR_NET_INVALID_CONTEXT.
 */
int netctx_set_nodelay(mbedtls_net_context* ctx, int nodelay);

/**
 * Get the local port to which the network context is bound.
 * 
 * Return -1 if the context is not valid or if the function
 * is unable to determine the port number.
 */
int netctx_get_port(mbedtls_net_context* ctx);

/**
 * Check and wait for the context to be ready for read and or write.
 * The function waits until data is available or a timeout occurred.
 *
 * The function returns a compound status.
 *    code :
 *       NETCTX_POLL_OK : 
 *          The operation succeeded. The event flags indicate if the
 *          context is ready for reading or writing.
 *       NETCTX_POLL_TIMEOUT : 
 *          This code indicates that the call timed out before the
 *          context became ready for reading or writing.
 *       NETCTX_POLL_ERROR : 
 *          An error has occurred.  The errnum field contains the
 *          error code.
 *
 *    event : Read and write flags indicate if the context is ready
 *            for reading or writing.
 *
 *    errnum : A mbedTLS error code if an error or if a timeout
 *             occurred.
 */
netctx_poll_status netctx_poll(mbedtls_net_context* ctx, netctx_poll_mode mode, uint32_t timeout);

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
 *    sbytes : The number of bytes sent.
 * 
 *    errnum : A mbedTLS error code if an error or if a timeout
 *             occurred.
 *
 * Possible values :
 *     code        | SND_OK | SND_RETRY | SND_ERROR
 *     sbytes      | > 0    | = 0       | = 0
 *     errnum      | 0      | < 0       | < 0
 *
 */
netctx_snd_status netctx_send(mbedtls_net_context* ctx, const unsigned char* buf, size_t len);

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
 *    rbytes : The number of bytes received.
 *
 *    errnum : A mbedTLS error code if an error or if a timeout
 *             occurred.
 *
 * Possible values :
 *
 *     code        | RCV_OK | RCV_RETRY | RCV_ERROR | RCV_EOF
 *     rbytes      | > 0    | = 0       | = 0       | = 0
 *     errnum      | 0      | < 0       | < 0       | = 0
 *
 */
netctx_rcv_status netctx_recv(mbedtls_net_context* ctx, unsigned char* buf, size_t len);

#ifdef __cplusplus
}
#endif
