/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "mbedccl/error.h"

/** A public/private key context*/
typedef struct pk_context pkctx;

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Allocate a public/private key context.
*/
pkctx* pkctx_alloc();

/**
* Free a public/private key context.
*/
void pkctx_free(pkctx* ctx);

mbed_errnum pkctx_parse_keyfile(pkctx* ctx);

#ifdef __cplusplus
}
#endif
