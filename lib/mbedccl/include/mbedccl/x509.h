/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <stddef.h>
#include"mbedccl/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct x509crt x509crt;

x509crt* x509crt_alloc();
void x509crt_free(x509crt* crt);

/*
 *
 */
int x509crt_parse_file(const x509crt* crt, const char* filename);

mbed_errnum x509crt_digest(const x509crt* crt, unsigned char* digest, size_t len);
mbed_errnum x509crt_info(char *buf, size_t size, const char *prefix, const x509crt *crt);

#ifdef __cplusplus
}
#endif
