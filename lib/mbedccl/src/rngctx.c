/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#include <stdlib.h>
#include <string.h>

#include "rngctx_private.h"

rngctx* rngctx_alloc()
{
	rngctx* const ctx = malloc(sizeof(rngctx));

	if (ctx) {
		mbedtls_entropy_init(&ctx->entropy_context);
		mbedtls_ctr_drbg_init(&ctx->ctr_drbg_context);
	}

	return ctx;
}


void rngctx_free(rngctx* ctx)
{
	if (ctx) {
		mbedtls_ctr_drbg_free(&ctx->ctr_drbg_context);
		mbedtls_entropy_free(&ctx->entropy_context);
		free(ctx);
	}
}


mbed_errnum rngctx_configure(rngctx* ctx)
{
	mbed_errnum errnum = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;

	if (ctx) {
		errnum = mbedtls_ctr_drbg_seed(
					&ctx->ctr_drbg_context,
					mbedtls_entropy_func,
					&ctx->entropy_context,
					0,
					0);
	}

	return errnum;
}


