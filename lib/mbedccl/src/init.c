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
static const char* mbedtls_version = MBEDTLS_VERSION_STRING;

mbed_errnum mbedccl_initialize()
{
	mbed_errnum errnum = 0;

	if (initialization_flag == 0) {
		// configure the platform.
		errnum = mbedtls_platform_setup(NULL);
		if (errnum != 0)
			goto abort;
		initialization_flag = MBEDCCL_INIT_PLATFORM;

		// configure the PSA crypto layer only
		int psa_err = psa_crypto_init();
		if (psa_err != 0) {
			//TODO : convert psa error code to mbedtls error code
			// mbedtls_pk_error_from_psa is deprecated so we return
			// a generic error message.
			errnum = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
			goto abort;
		}
		initialization_flag |= MBEDCCL_INIT_PSA;
	}

abort:
	return errnum;
}


void mbedccl_terminate()
{
	if ((initialization_flag & MBEDCCL_INIT_PSA) != 0)
		mbedtls_psa_crypto_free();

	if ((initialization_flag & MBEDCCL_INIT_PLATFORM) != 0) 
		mbedtls_platform_teardown(NULL);

	initialization_flag = 0;
	return;
}


const char* mbedccl_get_version()
{
	return mbedtls_version;
}


mbed_errnum mbedccl_get_verify_info(char *buf, size_t size, const char *prefix, uint32_t status)
{
	mbed_errnum errnum = MBEDTLS_ERR_SSL_BAD_INPUT_DATA;

	if (buf && prefix) {
		const int rc = mbedtls_x509_crt_verify_info(buf, size, prefix, status);

		// TODO: explain why this
		if (rc > 0)
			errnum = 0;
		else
			errnum = rc;
	}

	return errnum;
}

