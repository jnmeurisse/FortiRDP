/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#include "mbedccl/debug.h"
#include "mbedtls/debug.h"

static mbedccl_debug_cb debug_cb = 0;
static void* debug_data = 0;


void mbedccl_set_debug_cb(mbedccl_debug_cb cb, void* data)
{
	debug_cb = cb;
	debug_data = data;
}


void mbedccl_set_debug_treshod(int treshold)
{
	mbedtls_debug_set_threshold(treshold);
}


void mbedccl_debug_fn(void *ctx, int level, const char *file, int line, const char *str)
{
	if (debug_cb) {
		char buffer[512] = { 0 };

		snprintf(
			buffer,
			sizeof(buffer),
			".... %s:%05d: %s",
			file,
			line,
			str);

		// Remove new line
		buffer[strcspn(buffer, "\r\n")] = '\0';

		// output
		debug_cb(level, buffer, debug_data);
	}
}

