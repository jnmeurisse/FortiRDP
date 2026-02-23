/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "net/if/tunif.h"
#include <lwip/arch.h>
#include <lwip/err.h>


err_t tunif_init(struct netif* netif)
{
	netif->name[0] = 't';
	netif->name[1] = 'u';

	netif->state = NULL;
	netif->output = NULL;

	return ERR_OK;
}






#ifdef 0

/**
 * Creates a new TUN connection using the given device.
 *
*/
tun_pcb *tun_create(struct netif* tunif, tun_output_cb_fn output_cb, void* ctx)
{
	tun_pcb *pcb = mem_malloc(sizeof(tun_pcb));

	if (pcb) {
		pcb->netif = tunif;
		pcb->output_cb = output_cb;
		pcb->ctx_cb = ctx;
	}

	return pcb;
}


void tun_free(tun_pcb* pcb)
{
	if (pcb) {
		netif_remove(pcb->netif);
		mem_free(pcb);
	}
}


int tunossl_input(tun_pcb* tun, u8_t* s, size_t l)
{
    return 0;
}

#endif
