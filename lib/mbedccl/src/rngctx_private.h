/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "mbedccl\rngctx.h"

#include "mbedtls\entropy.h"
#include "mbedtls\ctr_drbg.h"
#include "mbedtls\ssl.h"

typedef struct rng_context {
	mbedtls_entropy_context entropy_context;
	mbedtls_ctr_drbg_context ctr_drbg_context;
} rngctx;
