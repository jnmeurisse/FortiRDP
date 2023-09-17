/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <stdint.h>

#include "mbedccl/nettypes.h"
#include "mbedccl/cclerr.h"

/** A network context*/
typedef struct net_context netctx;

#ifdef __cplusplus
extern "C" {
#endif

/*! Allocate a network context.
*/
netctx* netctx_alloc();

/*! Free a network context.
*/
void netctx_free(netctx* ctx);

/*! Close a network connection.
*/
void netctx_close(netctx* ctx);

/*! Disable transmission and reception and close the network connection.
*/
void netctx_shutdown(netctx* ctx);

/*! Return the socket file descriptor.
    The function returns -1 if the network context is not valid.
*/
int netctx_getfd(netctx* ctx);

/*! Initiate a connection with host:port and the given protocol (TCP or UPD).
*/
mbed_err netctx_connect(netctx* ctx, const char* hostname, const char* port, netctx_protocol protocol);

/*! Create a listening socket on host:port.
*/
mbed_err netctx_bind(netctx* ctx, const char* hostname, const char* port, netctx_protocol protocol);

/*! Accept a connection from a remote client.
*/
mbed_err netctx_accept(netctx* bind_ctx, netctx* accepting_ctx);

/*! Enable or disable the blocking mode.
*/
int netctx_set_blocking(netctx* ctx, int blocking);

/*! Enable or disable the no delay mode.
*/
int netctx_set_nodelay(netctx* ctx, int nodelay);

/*! Get local port
*/
int netctx_get_port(netctx* ctx);

/*! Check and wait for the context to be ready for read and or write.
    The function waits until data is available or a timeout occurred.

    Return :
      Bitmask composed of MBEDTLS_NET_POLL_READ/WRITE in case of success.
      0 in case of timeout.
      A negative value (mbed_err) in case of error
*/
int netctx_poll(netctx* ctx, netctx_poll_mode mode, uint32_t timeout);

/*!  Send data.
*/
netctx_snd_status netctx_send(netctx* ctx, const unsigned char* buf, size_t len);

/*! Receive data.
*/
netctx_rcv_status netctx_recv(netctx* ctx, unsigned char* buf, size_t len);

#ifdef __cplusplus
}
#endif
