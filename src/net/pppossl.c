/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/

#include "netif/ppp/ppp_opts.h"
#if PPP_SUPPORT  /* don't build if not configured for use in lwipopts.h */

#include <string.h>

#include "lwip/arch.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/netif.h"
#include "lwip/snmp.h"
#include "lwip/priv/tcpip_priv.h"
#include "lwip/api.h"
#include "lwip/def.h"
#include "netif/ppp/ppp_impl.h"

#include "net/pppossl.h"


/* callbacks called from PPP core */
static err_t pppossl_write(ppp_pcb *ppp, void *ctx, struct pbuf *p);
static err_t pppossl_netif_output(ppp_pcb *ppp, void *ctx, struct pbuf *pb, u16_t protocol);
static void  pppossl_connect(ppp_pcb *ppp, void *ctx);
static void  pppossl_disconnect(ppp_pcb *ppp, void *ctx);
static err_t pppossl_destroy(ppp_pcb *ppp, void *ctx);
static void  pppossl_send_config(ppp_pcb *ppp, void *ctx, u32_t accm, int pcomp, int accomp);
static void  pppossl_recv_config(ppp_pcb *ppp, void *ctx, u32_t accm, int pcomp, int accomp);

static void  pppossl_input_free_current_packet(pppossl_pcb *pppos);

/* Callbacks structure for PPP core */
static const struct link_callbacks pppossl_callbacks = {
	pppossl_connect,
#if PPP_SERVER
	nullptr,
#endif /* PPP_SERVER */
	pppossl_disconnect,
	pppossl_destroy,
	pppossl_write,
	pppossl_netif_output,
	pppossl_send_config,
	pppossl_recv_config
};


/*
* Create a new PPP connection using the given device.
*
*/
ppp_pcb*
pppossl_create(struct netif *pppif, pppossl_output_cb_fn output_cb,
	ppp_link_status_cb_fn link_status_cb, void *ctx_cb)
{
	pppossl_pcb *pppossl;
	ppp_pcb *ppp;

	pppossl = (pppossl_pcb *)mem_malloc(sizeof(pppossl_pcb));
	if (pppossl == NULL) {
		return NULL;
	}

	ppp = ppp_new(pppif, &pppossl_callbacks, pppossl, link_status_cb, ctx_cb);
	if (ppp == NULL) {
		mem_free(pppossl);
		return NULL;
	}

	memset(pppossl, 0, sizeof(pppossl_pcb));
	pppossl->ppp = ppp;
	pppossl->output_cb = output_cb;
	return ppp;
}


/* Called by PPP core */
static err_t
pppossl_write(ppp_pcb *ppp, void *ctx, struct pbuf *pbuf)
{
	pppossl_pcb* const pppos = (pppossl_pcb *)ctx;
	int err = ERR_OK;

	/* Send buffer into a PPP frame */
	if (pbuf->tot_len > 0) {
		// create a PPP header
		struct pbuf* const frame = pbuf_alloc(PBUF_RAW, sizeof(ppp_header) + pbuf->tot_len, PBUF_RAM);
		if (frame == NULL) {
			PPPDEBUG(LOG_WARNING, ("pppossl_write[%d]: alloc fail\n", ppp->netif->num));
			LINK_STATS_INC(link.memerr);
			LINK_STATS_INC(link.drop);
			MIB2_STATS_NETIF_INC(ppp->netif, ifoutdiscards);
			pbuf_free(pbuf);
			return ERR_MEM;
		}

		// Fill the fortigate PPP header
		ppp_header* const header = frame->payload;
		(*header)[0] = lwip_htons(pbuf->tot_len + sizeof(ppp_header));
		(*header)[1] = 0x5050;
		(*header)[2] = lwip_htons(pbuf->tot_len);

		// Append the payload
		int offset = sizeof(ppp_header);
		for (struct pbuf* p = pbuf; p; p = p->next) {
			memcpy((uint8_t *)frame->payload + offset, p->payload, p->len);
			offset += p->len;
		}

		// output the PPP frame
		u32_t lp = pppos->output_cb(ppp, frame, ppp->ctx_cb);
		pbuf_free(frame);
		if (lp != pbuf->tot_len + sizeof(ppp_header)) {
			err = ERR_IF;
			goto failed;
		}
	}

	pppos->last_xmit = sys_now();
	MIB2_STATS_NETIF_ADD(ppp->netif, ifoutoctets, pbuf->tot_len + sizeof(ppp_header));
	MIB2_STATS_NETIF_INC(ppp->netif, ifoutucastpkts);
	LINK_STATS_INC(link.xmit);
	pbuf_free(pbuf);

	return err;

failed:
	pppos->last_xmit = 0; 
	LINK_STATS_INC(link.err);
	LINK_STATS_INC(link.drop);
	MIB2_STATS_NETIF_INC(ppp->netif, ifoutdiscards);
	pbuf_free(pbuf);

	return err;
}

/* Called by PPP core */
static err_t
pppossl_netif_output(ppp_pcb *ppp, void *ctx, struct pbuf *pb, u16_t protocol)
{
	// Fill PPP frame...
	// configure the address control  protocol
	const u8_t header[4] = { PPP_ALLSTATIONS, PPP_UI, (protocol >> 8) & 0xFF, protocol & 0xFF };

	// prepare a network buffer to hold the header and the payload
	struct pbuf* const nb = pbuf_alloc(PBUF_RAW, sizeof(header), PBUF_RAM);
	if (nb == NULL) {
		PPPDEBUG(LOG_WARNING, ("pppos_netif_output[%d]: alloc fail\n", ppp->netif->num));
		LINK_STATS_INC(link.memerr);
		LINK_STATS_INC(link.drop);
		MIB2_STATS_NETIF_INC(ppp->netif, ifoutdiscards);
		return ERR_MEM;
	}

	// assign the header
	pbuf_take(nb, header, sizeof(header));

	// followed by the payload
	pbuf_chain(nb, pb);

	// Output everything
	return pppossl_write(ppp, ctx, nb);
}


void ppossl_send_ka(ppp_pcb *pcb)
{
	if (pcb->lcp_fsm.state != PPP_FSM_OPENED)
		return;

	struct discard_request {
		u8_t code;
		u8_t identifier;
		u16_t lengh;
		u32_t magicnumber;
	} ;

	struct pbuf* pbuf = pbuf_alloc(PBUF_RAW, sizeof(struct discard_request), PBUF_RAM);
	if (pbuf) {
		struct discard_request* const request = pbuf->payload;

		request->code = DISCREQ;
		request->identifier = ++pcb->lcp_fsm.id;
		request->lengh = lwip_htons(sizeof(struct discard_request));
		request->magicnumber = pcb->lcp_gotoptions.magicnumber;

		pppossl_netif_output(pcb, pcb->link_ctx_cb, pbuf, 0xC021);
		pbuf_free(pbuf);
	}
}


static void
pppossl_connect(ppp_pcb *ppp, void *ctx)
{
	pppossl_pcb* const pppossl = (pppossl_pcb *)ctx;

	/* reset PPPoSsl control block to its initial state */
	memset(&pppossl->last_xmit, 0, sizeof(pppossl_pcb) - offsetof(pppossl_pcb, last_xmit));

	/* disable unsupported Fortigate LCP negotiation */
	ppp->lcp_wantoptions.neg_accompression = 0;
	ppp->lcp_wantoptions.neg_pcompression = 0;
	ppp->lcp_wantoptions.neg_asyncmap = 0;

	/*
	* Start the connection and handle incoming events (packet or timeout).
	*/
	PPPDEBUG(LOG_INFO, ("pppossl_connect: unit %d: connecting\n", ppp->netif->num));
	ppp_start(ppp); /* notify upper layers */
}


static void
pppossl_disconnect(ppp_pcb *ppp, void *ctx)
{
	ppp_link_end(ppp); /* notify upper layers */
}


static err_t
pppossl_destroy(ppp_pcb *ppp, void *ctx)
{
	pppossl_pcb* const pppossl = (pppossl_pcb *)ctx;
	LWIP_UNUSED_ARG(ppp);

	pppossl_input_free_current_packet(pppossl);
	mem_free(pppossl);
	return ERR_OK;
}


/** Pass received raw characters to PPPoSsl to be decoded.
*
* @param ppp	PPP descriptor, returned by pppossl_create()
* @param s		received data
* @param l		length of received data
*/
int
pppossl_input(ppp_pcb *ppp, u8_t* s, int l)
{
	err_t err = PPPERR_NONE;
	pppossl_pcb*const pppossl = (pppossl_pcb *)ppp->link_ctx_cb;

	PPPDEBUG(LOG_DEBUG, ("pppossl_input[%d]: got %d bytes\n", ppp->netif->num, l));
	
	while (l > 0) {
		if (pppossl->in.state == PP_HEADER) {
			u8_t* in_header = (u8_t *)&pppossl->in.header;
			in_header[pppossl->in.counter++] = *s++;
			l--;
			if (pppossl->in.counter == sizeof(ppp_header)) {
				pppossl->in.state = PP_DATA;
				pppossl->in.counter = 0;
				pppossl->in.header[0] = lwip_ntohs(pppossl->in.header[0]);
				pppossl->in.header[2] = lwip_ntohs(pppossl->in.header[2]);
				int frame_size = pppossl->in.header[2];

				// Check header consistency
				if ((pppossl->in.header[0] != frame_size + sizeof(ppp_header)) ||
					(pppossl->in.header[1] != 0x5050)) {
					// Invalid header arguments, drop the frame.  It is not possible to 
					// resynchronize as it is with the standard PPP over serial protocol
					err = PPPERR_PROTOCOL;
					goto drop;
				}

				if (frame_size > 16 * 1024) {
					PPPDEBUG(LOG_WARNING, ("pppossl_input[%d]: ppp frame larger than 16k bytes\n", ppp->netif->num));
					err = PPPERR_PROTOCOL;
					goto drop;
				}

				// Allocate enough storage for the payload
				pppossl->in.data = pbuf_alloc(PBUF_RAW, frame_size, PBUF_RAM);
				if (pppossl->in.data == NULL) {
					err = PPPERR_ALLOC;
					goto drop;
				}
			}
		}
		else if (pppossl->in.state == PP_DATA) {
			int len = min(pppossl->in.header[2] - pppossl->in.counter, l);

			// append the payload to the buffer
			pbuf_take_at(pppossl->in.data, s, len, pppossl->in.counter);
			l -= len;
			s += len;

			pppossl->in.counter += len;
			if (pppossl->in.counter == pppossl->in.header[2]) {
				// payload is available, pass to ppp processing
				ppp_input(ppp, pppossl->in.data);

				// reset the state of the PPP packet parser
				memset(&pppossl->in, 0, sizeof(pppossl->in));
			}
		}
	}

	goto exit;

drop:
	LINK_STATS_INC(link.proterr);
	memset(&pppossl->in, 0, sizeof(pppossl->in));

exit:
	return err;
}



static void
pppossl_send_config(ppp_pcb *ppp, void *ctx, u32_t accm, int pcomp, int accomp)
{
	pppossl_pcb* const pppos = (pppossl_pcb *)ctx;
	LWIP_UNUSED_ARG(ppp);
	LWIP_UNUSED_ARG(pcomp);
	LWIP_UNUSED_ARG(accomp);
}


static void
pppossl_recv_config(ppp_pcb *ppp, void *ctx, u32_t accm, int pcomp, int accomp)
{
	pppossl_pcb* const pppos = (pppossl_pcb *)ctx;
	LWIP_UNUSED_ARG(ppp);
	LWIP_UNUSED_ARG(pcomp);
	LWIP_UNUSED_ARG(accomp);
}


/*
* Drop the input packet.
*/
static void
pppossl_input_free_current_packet(pppossl_pcb *pppos)
{
	if (pppos->in.data != NULL) {
		pbuf_free(pppos->in.data);
	}
	pppos->in.data = NULL;
}


#endif /* PPP_SUPPORT */
