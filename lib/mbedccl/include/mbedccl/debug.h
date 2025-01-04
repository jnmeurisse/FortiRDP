/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

typedef void(*mbedccl_debug_cb)(int level, const char* buffer, void* data);

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Configure a debug callback function.
 */
void mbedccl_set_debug_cb(mbedccl_debug_cb cb, void* data);

/*
 * Configure the debug threshold.
 */
void mbedccl_set_debug_treshod(int treshold);

#ifdef __cplusplus
}
#endif
