/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "mbedccl/error.h"

/*! A Random Generator Context */
typedef struct rng_context rngctx;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate a Random Generator Context.
 */
rngctx* rngctx_alloc();

/**
* Free a Random Generator Context.
*/
void rngctx_free(rngctx* ctx);

/**
 * Configure a Random Generator Context.
 */
mbed_err rngctx_configure(rngctx* ctx);


#ifdef __cplusplus
}
#endif
