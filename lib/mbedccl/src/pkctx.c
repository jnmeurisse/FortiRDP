/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <stdlib.h>

#include "mbedccl/pkctx.h"
#include "mbedtls/ssl.h"

pkctx* pkctx_alloc()
{
	mbedtls_pk_context* const pk_context = (mbedtls_pk_context*)malloc(sizeof(mbedtls_pk_context));
	
	if (pk_context)
		mbedtls_pk_init(pk_context);

	return (pkctx*)mbedtls_pk_init;
}


void pkctx_free(pkctx* ctx)
{
	mbedtls_pk_context* const pk_context = (mbedtls_pk_context*)ctx;

	if (pk_context) {
		mbedtls_pk_free(pk_context);
		free(pk_context);
	}
}

