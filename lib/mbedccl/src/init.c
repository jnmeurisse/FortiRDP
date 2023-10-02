/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <stdlib.h>
#include <string.h>

#include "mbedccl/init.h"

#include "mbedtls/version.h"
#include "mbedtls/platform.h"
#include "mbedtls/psa_util.h"
#include "mbedtls/ssl.h"

#define MBEDCCL_INIT_PLATFORM 0x01
#define MBEDCCL_INIT_PSA      0x02

static int initialization_flag = 0;
static char mbedtls_version[10] = "unknown";

mbed_err mbedccl_initialize()
{
	mbed_err rc = 0;

	if (initialization_flag == 0) {
		mbedtls_version_get_string(mbedtls_version);

		// configure the platform.
		rc = mbedtls_platform_setup(NULL);
		if (rc != 0)
			goto abort;
		initialization_flag = MBEDCCL_INIT_PLATFORM;

		// configure the PSA crypto layer only if compiled with MBEDTLS_USE_PSA_CRYPTO
#ifdef MBEDTLS_USE_PSA_CRYPTO
		int psa_err = psa_crypto_init();
		if (psa_err != 0) {
			//TODO : convert psa error code to mbedtls error code
			goto abort;
		}
		initialization_flag |= MBEDCCL_INIT_PSA;
#endif
	}

abort:
	return rc;
}


void mbedccl_terminate()
{
#ifdef MBEDTLS_USE_PSA_CRYPTO
	if ((initialization_flag & MBEDCCL_INIT_PSA) != 0)
		mbedtls_psa_crypto_free();
#endif

	if ((initialization_flag & MBEDCCL_INIT_PLATFORM) != 0) 
		mbedtls_platform_teardown(NULL);

	initialization_flag = 0;
	return;
}


const char* mbedccl_get_version()
{
	return mbedtls_version;
}


mbed_err mbedccl_get_verify_info(char *buf, size_t size, const char *prefix, uint32_t status)
{
	mbed_err rc = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;

	if (buf && prefix) {
		rc = mbedtls_x509_crt_verify_info(buf, size, prefix, status);

		// Return only error cod
		if (rc > 0)
			rc = 0;
	}

	return rc;
}

