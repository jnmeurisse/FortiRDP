/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#include <lwipopts.h>

#if PPP_SUPPORT  /* don't build if not configured for use in lwipopts.h */

#ifndef PPPOSSL_H
#define PPPOSSL_H

#include <lwip/arch.h>
#include <lwip/pbuf.h>
#include <lwip/netif.h>
#include <netif/ppp/ppp.h>

#ifdef __cplusplus
extern "C" {
#endif
	/* PPP packet parser states. */
	enum {
		PP_HEADER = 0,   /* Receiving header */
		PP_DATA = 1      /* Receiving data */
	};

	/* PPPossl output callback function prototype */
	typedef u32_t(*pppossl_output_cb_fn)(ppp_pcb *pcb, struct pbuf* pbuf, void *ctx);

	/*
	* Fortinet ppp header
	*   header[0] = payload len + sizeof(ppp_header)
	*   header[1] = 0x5050 (tag)
	*   header[2] = payload len
	*/
	typedef u16_t ppp_header[3];


	/*
	* PPPossl interface control block.
	*/
	typedef struct pppossl_pcb_s pppossl_pcb;
	struct pppossl_pcb_s {
		ppp_pcb *ppp;                      /* PPP PCB */
		pppossl_output_cb_fn output_cb;    /* PPP output callback */
		u32_t last_xmit;                   /* Time stamp of last transmission. */
		struct {
			int state;                     /* The input process state. */
			uint16_t counter;              /* ..number of bytes processed */
			ppp_header header;             /* ..PPP header */
			struct pbuf* data;             /* ..payload */
		} in;
	};


	/* Create a new PPPossl session. */
	ppp_pcb *pppossl_create(struct netif *pppif, pppossl_output_cb_fn output_cb,
		ppp_link_status_cb_fn link_status_cb, void *ctx_cb);

	/* This is the input function to be called for received data. */
	int pppossl_input(ppp_pcb *ppp, u8_t* s, size_t l);

	/* Send a keep alive packet */
	void ppossl_send_ka(ppp_pcb *ppp);

#ifdef __cplusplus
}
#endif


#endif /* PPPOS_H */
#endif /* PPP_SUPPORT && PPPSSL_SUPPORT */
