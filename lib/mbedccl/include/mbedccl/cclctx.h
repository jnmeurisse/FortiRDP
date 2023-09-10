/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "mbedccl/cclerr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*sslcfg_debug_cb)(int level, const char* buffer, void* data);

/*! Initializes the Mbed TLS library.
* The function initializes the threading and the psa modules.
*
*/
mbed_err mbedccl_initialize();

/*! Un-initializes the Mbed TLS library.
*/
void mbedccl_terminate();

/*! Returns the Mbed TLS library version.
*/
const char* mbedccl_get_version();

#ifdef __cplusplus
}
#endif
