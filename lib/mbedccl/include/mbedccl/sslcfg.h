/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#pragma once

#include "mbedccl/x509.h"
#include "mbedccl/rngctx.h"
#include "mbedccl/error.h"


#ifdef __cplusplus
extern "C" {
#endif

/** A SSL configuration context */
typedef struct ssl_config sslcfg;


/**
 * Allocate a SSL configuration context.
 */
sslcfg* sslcfg_alloc();

/**
* Free a SSL configuration context.
*/
void sslcfg_free(sslcfg* cfg);

/**
 * Configure a SSL configuration context.
 * The function initializes the context with all options required by fortirdp.
 */
mbed_err sslcfg_configure(sslcfg* cfg, rngctx* ctx);

/**
 * Enable/disable the debug callback.
 */
void sslcfg_enable_debug(sslcfg* cfg, int onoff);

/**
*/
mbed_err sslfcg_set_ca_chain(sslcfg* cfg, x509crt* crt);

#ifdef __cplusplus
}
#endif
