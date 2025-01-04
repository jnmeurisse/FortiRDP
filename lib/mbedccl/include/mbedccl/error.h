/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// mbed tls error code
typedef int mbed_errnum;

mbed_errnum mbedccl_get_timeout_error();
mbed_errnum mbedccl_get_bind_error();
void mbedccl_strerror(mbed_errnum errnum, char *buffer, size_t buflen);

#ifdef __cplusplus
}
#endif
