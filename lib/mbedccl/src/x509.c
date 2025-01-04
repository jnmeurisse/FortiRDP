/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <stdlib.h>
#include <string.h>

#include "mbedtls/x509_crt.h"
#include "mbedtls/sha256.h"
#include "mbedtls/error.h"
#include "mbedtls/ssl.h"

#include "mbedccl/x509.h"


x509crt* x509crt_alloc()
{
	mbedtls_x509_crt* const x509_crt = malloc(sizeof(mbedtls_x509_crt));

	if (x509_crt)
		mbedtls_x509_crt_init(x509_crt);

	return (x509crt*) x509_crt;
}


void x509crt_free(x509crt* crt)
{
	mbedtls_x509_crt* const x509_crt = (mbedtls_x509_crt*)crt;
	
	if (x509_crt) {
		mbedtls_x509_crt_free(x509_crt);
		free(x509_crt);
	}
}


int x509crt_parse_file(const x509crt* crt, const char* filename)
{
	int rc = 0;

	mbedtls_x509_crt* const x509_crt = (mbedtls_x509_crt*)crt;

	if (x509_crt) {
		rc = mbedtls_x509_crt_parse_file(x509_crt, filename);
	}

	return rc;
}


mbed_errnum x509crt_digest(const x509crt* crt, unsigned char* digest, size_t len)
{
	mbed_errnum errnum = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
	mbedtls_x509_crt* const x509_crt = (mbedtls_x509_crt*)crt;

	if (x509_crt) {
		if (!digest || len < 32)
			errnum = MBEDTLS_ERR_X509_BUFFER_TOO_SMALL;
		else {
			mbedtls_sha256(x509_crt->raw.p, x509_crt->raw.len, digest, 0);
			errnum = 0;
		}
	}

	return errnum;
}


mbed_errnum x509crt_info(char *buf, size_t size, const char *prefix, const x509crt *crt)
{
	mbed_errnum errnum = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
	mbedtls_x509_crt* const x509_crt = (mbedtls_x509_crt*)crt;

	if (x509_crt) {
		mbedtls_x509_crt_info(buf, size, "   ", x509_crt);
		errnum = 0;
	}

	return errnum;
}

