/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#include "mbedccl/error.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"

mbed_errnum mbedccl_get_timeout_error()
{
	return MBEDTLS_ERR_SSL_TIMEOUT;
}


mbed_errnum mbedccl_get_bind_error()
{
	return MBEDTLS_ERR_NET_BIND_FAILED;
}


void mbedccl_strerror(mbed_errnum errnum, char *buffer, size_t buflen)
{
	mbedtls_strerror(errnum, buffer, buflen);
}

