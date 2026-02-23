/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#ifndef TUNOSSL_H
#define TUNOSSL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lwip/netif.h>
#include <lwip/err.h>
#include <lwip/pbuf.h>

#ifdef 0
	/* TUNossl output callback function prototype */
	typedef u32_t(*tun_output_cb_fn)(struct tun_pcb_s* pcb, struct pbuf* pbuf, void* ctx);

	struct tun_pcb_s
	{
		tun_output_cb_fn output_cb;			/* Output callback */
		void* ctx_cb;						/* Callback optional pointer */
		struct netif* netif;				/* TUN interface */
	};

	typedef struct tun_pcb_s tun_pcb;

	tun_pcb* tun_create(struct netif* tunif, tun_output_cb_fn output_cb, void* ctx);
	void tun_free(tun_pcb* pcb);
	void tun_set_default(tun_pcb* pcb);

	/* This is the input function to be called for received data. */
	int tunossl_input(tun_pcb* tun, u8_t* s, size_t l);

#endif

err_t tunif_init(struct netif* netif);


#ifdef __cplusplus
}
#endif

#endif