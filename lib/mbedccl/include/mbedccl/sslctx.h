/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#pragma once
#include <stdint.h>

#include "mbedccl/x509.h"
#include "mbedccl/netctx.h"
#include "mbedccl/sslcfg.h"
#include "mbedccl/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A SSL context */
typedef struct ssl_context sslctx;

/**
 * Allocate a SSL context.
 */
sslctx* sslctx_alloc();

/**
 * Configure a SSL context.
 */
mbed_err sslctx_config(sslctx*, sslcfg* cfg);

/**
 * Configure the network context for read and write operation.
 */
mbed_err sslctx_set_netctx(sslctx* ctx, netctx* netctx);

/**
* Free a SSL context.
*/
void sslctx_free(sslctx* ctx);


sslctx_handshake_status sslctx_handshake(sslctx* ctx);

mbed_err sslctx_close(sslctx* ctx);


netctx_snd_status sslctx_send(sslctx* ctx, const unsigned char* buf, size_t len);
netctx_rcv_status sslctx_recv(sslctx* ctx, unsigned char* buf, size_t len);


x509crt* sslctx_get_peer_x509crt(sslctx* ctx);
uint32_t sslctx_get_verify_result(sslctx* ctx);
const char* sslctx_get_ciphersuite(sslctx* ctx);
const char* sslctx_get_version(sslctx* ctx);

#ifdef __cplusplus
}
#endif