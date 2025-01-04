/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <stdint.h>
#include "mbedccl/error.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initialize the Mbed TLS library.
 * The function initializes the threading and the psa modules.
 */
mbed_errnum mbedccl_initialize();

/**
 * Un-initialize the Mbed TLS library.
 */
void mbedccl_terminate();

/**
 * Return the Mbed TLS library version.
 */
const char* mbedccl_get_version();

/**
 * Store in the given buffer an informational string about the verification 
 * status of a certificate.  
 * 
 * The function returns an error code if the buffer is too small.
 */
mbed_errnum mbedccl_get_verify_info(char *buf, size_t size, const char *prefix, uint32_t status);

#ifdef __cplusplus
}
#endif
