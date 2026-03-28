/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <netif/ppp/ppp_opts.h>
#if PPP_SUPPORT  /* don't build if not configured for use in lwipopts.h */

#include "pppfgt.h"
#include <lwip/arch.h>
#include <lwip/err.h>
#include <lwip/pbuf.h>
#include <lwip/sys.h>
#include <lwip/mem.h>
#include <lwip/netif.h>
#include <lwip/snmp.h>
#include <lwip/def.h>
#include <netif/ppp/ppp_impl.h>

# define PPPFGT_RX_BUFFER	8192


/* PPP packet parser states. */
enum {
	PP_HEADER = 0,   /* Receiving header */
	PP_DATA = 1      /* Receiving data */
};

/*
 * FortiGate PPP header
 *   header[0] = payload length + sizeof(ppp_header)
 *   header[1] = 0x5050 (tag)
 *   header[2] = payload length
 */
typedef u16_t ppp_header[3];

/*
* PPP tunnel state.
*/
struct pppfgt_context_s {
	ppp_pcb* ppp_pcb;

	u32_t last_xmit;                   /* Time stamp of last transmission.	*/

	/* RX streaming state */
	u8_t rx_buf[PPPFGT_RX_BUFFER];
	u16_t rx_len;
	u16_t rx_needed;
	u8_t header_parsed;
};


/* Callbacks called from PPP core */
static void  ppp_connect_cb(ppp_pcb* ppp, void* ctx);
static void  ppp_disconnect_cb(ppp_pcb* ppp, void* ctx);
static err_t ppp_destroy_cb(ppp_pcb* ppp, void* ctx);
static err_t ppp_write_cb(ppp_pcb* ppp, void* ctx, struct pbuf* p);
static err_t ppp_netif_output_cb(ppp_pcb* ppp, void* ctx, struct pbuf* pb, u16_t protocol);
static void  ppp_send_config_cb(ppp_pcb* ppp, void* ctx, u32_t accm, int pcomp, int accomp);
static void  ppp_recv_config_cb(ppp_pcb* ppp, void* ctx, u32_t accm, int pcomp, int accomp);


/* Callbacks structure for PPP core */
static const struct link_callbacks pppossl_callbacks = {
	ppp_connect_cb,
#if PPP_SERVER
	nullptr,
#endif /* PPP_SERVER */
	ppp_disconnect_cb,
	ppp_destroy_cb,
	ppp_write_cb,
	ppp_netif_output_cb,
	ppp_send_config_cb,
	ppp_recv_config_cb
};


pppfgt_context* pppfgt_create(struct netif* netif, ppp_link_status_cb_fn link_status_cb, void* ctx_cb)
{
	pppfgt_context* context = mem_malloc(sizeof(struct pppfgt_context_s));

	if (context) {
		context->last_xmit = 0;
		context->rx_len = 0;
		context->header_parsed = 0;

		context->ppp_pcb = ppp_new(netif, &pppossl_callbacks, context, link_status_cb, ctx_cb);
		if (context->ppp_pcb == NULL) {
			mem_free(context);
			return NULL;
		}

		// IP traffic is routed through that interface.
		ppp_set_default(context->ppp_pcb);
	}

	return context;
}


err_t pppfgt_free(pppfgt_context* context)
{
	err_t rc = ERR_OK;

	if (context) {
		rc = ppp_free(context->ppp_pcb);
		mem_free(context);
	}

	return rc;
}


err_t pppfgt_connect(pppfgt_context* context)
{
	if (!context || !context->ppp_pcb)
		return ERR_ARG;

	return ppp_connect(context->ppp_pcb, 0);
}


err_t pppfgt_disconnect(pppfgt_context* context)
{
	if (!context || !context->ppp_pcb)
		return ERR_ARG;

	return ppp_close(context->ppp_pcb, 0);
}


err_t pppfgt_input_bytes(pppfgt_context* context, const u8_t* data, size_t len)
{
	const u8_t* src = data;

	while (len) {
		context->rx_buf[context->rx_len++] = *src++;
		len--;

		if (!context->header_parsed)
		{
			if (context->rx_len == sizeof(ppp_header)) {
				ppp_header* header = (ppp_header*)context->rx_buf;
				const u16_t total_len = lwip_ntohs((*header)[0]);
				const u16_t frame_size = lwip_ntohs((*header)[2]);


				// Check header consistency
				if (total_len != frame_size + sizeof(ppp_header) || (*header)[1] != 0x5050)
					return ERR_IF;

				// Check if frame fits the buffer
				if (frame_size > sizeof(context->rx_buf)) {
					PPPDEBUG(LOG_WARNING, ("pppossl_input[%d]: ppp frame larger than buffer\n", ppp->netif->num));
					return ERR_IF;
				}

				context->rx_len = 0;
				context->rx_needed = frame_size;
				context->header_parsed = 1;
			}
		}
		else {
			if (context->rx_len == context->rx_needed)
			{
				struct pbuf* p = pbuf_alloc(PBUF_RAW, context->rx_needed, PBUF_RAM);
				if (!p) return ERR_MEM;

				ppp_input(context->ppp_pcb, p);

				context->rx_len = 0;
				context->header_parsed = 0;
			}
		}
	}

	return ERR_OK;
}



/* Called by PPP core */
static void
ppp_connect_cb(ppp_pcb* ppp, void* ctx)
{
	pppfgt_context* const context = ctx;

	/* reset PPP over SSL context to its initial state */
	context->last_xmit = 0;
	context->header_parsed = 0;
	context->rx_len = 0;

	/* ask DNS  */
	ppp_set_usepeerdns(ppp, 1);

	/* disable unsupported FortiGate LCP negotiation */
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
ppp_disconnect_cb(ppp_pcb* ppp, void* ctx)
{
	LWIP_UNUSED_ARG(ctx);
	ppp_link_end(ppp); /* notify upper layers */
}


static err_t
ppp_destroy_cb(ppp_pcb* ppp, void* ctx)
{
	LWIP_UNUSED_ARG(ppp);
	return ERR_OK;
}


static err_t
ppp_write_cb(ppp_pcb* ppp, void* ctx, struct pbuf* pbuf)
{
	pppfgt_context* const context = ctx;
	err_t err = ERR_OK;

	if (!pbuf) {
		err = ERR_BUF;
	}
	else {
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

			// Fill the fortiGate PPP header.
			ppp_header* const header = frame->payload;
			(*header)[0] = lwip_htons(pbuf->tot_len + sizeof(ppp_header));
			(*header)[1] = 0x5050;
			(*header)[2] = lwip_htons(pbuf->tot_len);

			// Append the payload.
			int offset = sizeof(ppp_header);
			for (struct pbuf* p = pbuf; p; p = p->next) {
				memcpy((u8_t*)frame->payload + offset, p->payload, p->len);
				offset += p->len;
			}

			// Output the PPP frame.
			u32_t lp = ppp->netif->linkoutput(ppp->netif, frame);
			pbuf_free(frame);
			if (lp != pbuf->tot_len + sizeof(ppp_header)) {
				err = ERR_IF;
				goto failed;
			}
		}

		context->last_xmit = sys_now();
		MIB2_STATS_NETIF_ADD(ppp->netif, ifoutoctets, pbuf->tot_len + sizeof(ppp_header));
		MIB2_STATS_NETIF_INC(ppp->netif, ifoutucastpkts);
		LINK_STATS_INC(link.xmit);
		pbuf_free(pbuf);
	}

	return err;

failed:
	context->last_xmit = 0;
	LINK_STATS_INC(link.err);
	LINK_STATS_INC(link.drop);
	MIB2_STATS_NETIF_INC(ppp->netif, ifoutdiscards);
	pbuf_free(pbuf);

	return err;
}


static err_t
ppp_netif_output_cb(ppp_pcb* ppp, void* ctx, struct pbuf* pb, u16_t protocol)
{
	// Fill the PPP frame...
	// - configure the address control  protocol
	const u8_t header[4] = { PPP_ALLSTATIONS, PPP_UI, (protocol >> 8) & 0xFF, protocol & 0xFF };

	// - prepare a network buffer to hold the header and the payload
	struct pbuf* const nb = pbuf_alloc(PBUF_RAW, sizeof(header), PBUF_RAM);
	if (nb == NULL) {
		PPPDEBUG(LOG_WARNING, ("pppos_netif_output[%d]: alloc fail\n", ppp->netif->num));
		LINK_STATS_INC(link.memerr);
		LINK_STATS_INC(link.drop);
		MIB2_STATS_NETIF_INC(ppp->netif, ifoutdiscards);
		return ERR_MEM;
	}

	// - assign the header
	pbuf_take(nb, header, sizeof(header));

	// - followed by the payload
	pbuf_chain(nb, pb);

	// Output everything
	return ppp_write_cb(ppp, ctx, nb);
}


static void
ppp_send_config_cb(ppp_pcb* ppp, void* ctx, u32_t accm, int pcomp, int accomp)
{
	LWIP_UNUSED_ARG(ctx);
	LWIP_UNUSED_ARG(accm);
	LWIP_UNUSED_ARG(ppp);
	LWIP_UNUSED_ARG(pcomp);
	LWIP_UNUSED_ARG(accomp);
}


static void
ppp_recv_config_cb(ppp_pcb* ppp, void* ctx, u32_t accm, int pcomp, int accomp)
{
	LWIP_UNUSED_ARG(ctx);
	LWIP_UNUSED_ARG(accm);
	LWIP_UNUSED_ARG(ppp);
	LWIP_UNUSED_ARG(pcomp);
	LWIP_UNUSED_ARG(accomp);
}


#else

err_t ppif_init(struct netif* netif)
{
	return ERR_IF;
}

#endif /* PPP_SUPPORT */
