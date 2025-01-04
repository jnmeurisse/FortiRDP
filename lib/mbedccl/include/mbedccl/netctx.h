/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <stdlib.h>
#include <stdint.h>

#include "mbedccl/nettypes.h"
#include "mbedccl/error.h"

/** A network context*/
typedef struct net_context netctx;

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Allocate a network context.
 */
netctx* netctx_alloc();

/**
 * Free a network context.
 */
void netctx_free(netctx* ctx);

/**
 * Close a network connection
 */
mbed_errnum netctx_close(netctx* ctx);

/**
 * Disable transmission and reception and close the network connection.
 */
mbed_errnum netctx_shutdown(netctx* ctx);

/**
 * Return the socket file descriptor.
 * The function returns -1 if the network context is not valid.
 */
int netctx_getfd(netctx* ctx);

/**
 * Initiate a connection with host:port and the given protocol (TCP or UPD).
 */
mbed_errnum netctx_connect(netctx* ctx, const char* hostname, const char* port, netctx_protocol protocol);

/**
 * Create a listening socket on host:port and the given protocol (TCP or UPD).
 */
mbed_errnum netctx_bind(netctx* ctx, const char* hostname, const char* port, netctx_protocol protocol);

/**
 * Accept a connection from a remote client.
 */
mbed_errnum netctx_accept(netctx* bind_ctx, netctx* accepting_ctx);

/**
 * Enable or disable the blocking mode.
 */
mbed_errnum netctx_set_blocking(netctx* ctx, int blocking);

/**
 * Enable or disable the no delay mode.
 */
mbed_errnum netctx_set_nodelay(netctx* ctx, int nodelay);

/**
 * Get the local port to which the network context is bound.
 * The function returns -1 if the context is not valid or if the function
 * is unable to determine the port number.
 */
int netctx_get_port(netctx* ctx);

/**
 * Check and wait for the context to be ready for read and or write.
 * The function waits until data is available or a timeout occurred.
 *
 * The function returns a compound status.
 *    status_code : .
 *         NETCTX_POLL_OK : 
 *               The operation succeeded. The event flags
 *               indicate if the context is ready for
 *               read or write.
 *         NETCTX_POLL_TIMEOUT : 
 *               This code indicates that the call timed out
 *               before the context became ready for read or write.
 *         NETCTX_POLL_ERROR : 
 *               An error has occurred.  The errnum field contains
 *               the error code.
 *    event       : Read and write flags indicate if the context is
 *                  ready for reading or writing.
 *    errnum      : A mbed TLS error code if an error or if a timeout
 *                  occurred.
 */
netctx_poll_status netctx_poll(netctx* ctx, netctx_poll_mode mode, uint32_t timeout);

/**
 * Send data.
 *
 * The function returns a compound status.
 *     status_code | SND_OK | SND_RETRY | SND_ERROR
 *     errnum      | 0      | < 0       | < 0
 *     sbytes      | > 0    | = 0       | = 0
 *
 *  SND_OK     : The send operation succeeded, sbytes contains the number
 *               of bytes effectively sent.
 *  SND_RETRY : No bytes have been sent, the send operation must be retried.
 *              The errnum field contains the mbed TLS error code.
 *  SND_ERROR : No bytes have been sent, the send operation failed.
 *              The errnum field contains the mbed TLS error code.
 */
netctx_snd_status netctx_send(netctx* ctx, const unsigned char* buf, size_t len);

/**
 * Receive data.
 *
 * The function returns a compound status.
 *     status_code | RCV_OK | RCV_RETRY | RCV_ERROR | RCV_EOF
 *     errnum      | 0      | < 0       | < 0       | = 0
 *     rbytes      | > 0    | = 0       | = 0       | = 0
 *
 *  RCV_OK    : The receive operation succeeded, rbytes contains the number
 *              of bytes received.
 *  RCV_RETRY : No bytes have been received, the receive operation must be
 *              retried. The errnum field contains the mbed TLS error code
 *  RCV_ERROR : No bytes have been received, the receive operation failed.
 *              The errnum field contains the mbed TLS error code.
 *  RCV_EOF   : No bytes have been received, the peer closed the network 
 *              connection.
 */
netctx_rcv_status netctx_recv(netctx* ctx, unsigned char* buf, size_t len);

#ifdef __cplusplus
}
#endif
