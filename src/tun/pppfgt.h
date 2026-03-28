/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#ifndef PPPFGT_H
#define PPPFGT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lwip/netif.h>
#include <lwip/err.h>
#include <netif/ppp/ppp.h>

	typedef struct pppfgt_context_s pppfgt_context;

	pppfgt_context* pppfgt_create(struct netif* netif, ppp_link_status_cb_fn link_status_cb, void* ctx_cb);
	err_t pppfgt_free(pppfgt_context*);

	err_t pppfgt_connect(pppfgt_context* context);
	err_t pppfgt_disconnect(pppfgt_context* context);

	err_t pppfgt_input_bytes(pppfgt_context* context, const u8_t* data, size_t len);

#ifdef __cplusplus
}
#endif

#endif