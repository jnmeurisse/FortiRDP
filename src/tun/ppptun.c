/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <netif/ppp/ppp_opts.h>
#if PPP_SUPPORT  /* don't build if not configured for use in lwipopts.h */

#include "pposif.h"
#include <lwip/arch.h>
#include <lwip/err.h>
#include <lwip/pbuf.h>
#include <lwip/sys.h>
#include <lwip/mem.h>
#include <lwip/netif.h>
#include <lwip/snmp.h>
#include <lwip/def.h>
#include <netif/ppp/ppp_impl.h>


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
* PPP over SSL device context.
*/
struct pppossl_state_s {
	ppp_pcb* ppp_pcb;
	u32_t last_xmit;                   /* Time stamp of last transmission. */
	struct {
		int state;                     /* The input process state. */
		uint16_t counter;              /* ..number of bytes processed */
		ppp_header header;             /* ..PPP header */
		struct pbuf* data;             /* ..payload */
	} in;
};

typedef struct pppossl_state_s pppossl_context;


/* Callbacks called from PPP core */
static void  pppossl_connect_cb(ppp_pcb* ppp, void* ctx);
static void  pppossl_disconnect_cb(ppp_pcb* ppp, void* ctx);
static err_t pppossl_destroy_cb(ppp_pcb* ppp, void* ctx);
static err_t pppossl_write_cb(ppp_pcb* ppp, void* ctx, struct pbuf* p);
static err_t pppossl_netif_output_cb(ppp_pcb* ppp, void* ctx, struct pbuf* pb, u16_t protocol);
static void  pppossl_send_config_cb(ppp_pcb* ppp, void* ctx, u32_t accm, int pcomp, int accomp);
static void  pppossl_recv_config_cb(ppp_pcb* ppp, void* ctx, u32_t accm, int pcomp, int accomp);

static void pppossl_netif_removed_cb(struct netif* netif);

/* Callbacks structure for PPP core */
static const struct link_callbacks pppossl_callbacks = {
	pppossl_connect_cb,
#if PPP_SERVER
	nullptr,
#endif /* PPP_SERVER */
	pppossl_disconnect_cb,
	pppossl_destroy_cb,
	pppossl_write_cb,
	pppossl_netif_output_cb,
	pppossl_send_config_cb,
	pppossl_recv_config_cb
};


err_t pppif_init(struct netif* netif)
{
	pppossl_context* context = mem_malloc(sizeof(pppossl_context));
	if (context == NULL)
		return ERR_MEM;
	context->last_xmit = 0;
	context->in.state = PP_HEADER;

	context->ppp_pcb = ppp_new(netif, &pppossl_callbacks, context, link_status_cb, ctx_cb);
	if (context->ppp_pcb == NULL) {
		mem_free(context);
		return ERR_MEM;
	}

	// FortiGate does not support these options, disable it.
	context->ppp_pcb->lcp_wantoptions.neg_accompression = 0;
	context->ppp_pcb->lcp_wantoptions.neg_pcompression = 0;
	context->ppp_pcb->lcp_wantoptions.neg_asyncmap = 0;

	netif->state = context;
	netif_set_remove_callback(netif, pppossl_netif_removed_cb);

	return ERR_OK;
}


err_t pppif_connect(struct netif* netif)
{
	if (!netif || !netif->state)
		return ERR_IF;

	pppossl_context* const ppp_ctx = netif->state;
	if (!ppp_ctx->ppp_pcb)
		return ERR_IF;

	// Start the connection.  The ppp_link_status_cb will be called
	// by the lwIP stack to report the connection success/failure.
	return ppp_connect(ppp_ctx->ppp_pcb, 0);
}


/*
* Drop the input packet.
*/
static void
pppossl_input_free_current_packet(pppossl_context* ppp_ctx)
{
	if (ppp_ctx->in.data != NULL) {
		pbuf_free(ppp_ctx->in.data);
	}
	ppp_ctx->in.data = NULL;
}


/* Called by PPP core */
static void
pppossl_connect_cb(ppp_pcb* ppp, void* ctx)
{
	pppossl_context* const ppp_ctx = ctx;

	/* reset PPP over SSL context to its initial state */
	ppp_ctx->last_xmit = 0;
	memset(&ppp_ctx->in, 0, sizeof ppp_ctx->in);

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
pppossl_disconnect_cb(ppp_pcb* ppp, void* ctx)
{
	LWIP_UNUSED_ARG(ctx);
	ppp_link_end(ppp); /* notify upper layers */
}


static err_t
pppossl_destroy_cb(ppp_pcb* ppp, void* ctx)
{
	pppossl_context* const ppp_ctx = ctx;
	LWIP_UNUSED_ARG(ppp);

	pppossl_input_free_current_packet(ppp_ctx);
	mem_free(ppp_ctx);
	return ERR_OK;
}


static err_t
pppossl_write_cb(ppp_pcb* ppp, void* ctx, struct pbuf* pbuf)
{
	pppossl_context* const ppp_ctx = ctx;
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

		ppp_ctx->last_xmit = sys_now();
		MIB2_STATS_NETIF_ADD(ppp->netif, ifoutoctets, pbuf->tot_len + sizeof(ppp_header));
		MIB2_STATS_NETIF_INC(ppp->netif, ifoutucastpkts);
		LINK_STATS_INC(link.xmit);
		pbuf_free(pbuf);
	}

	return err;

failed:
	ppp_ctx->last_xmit = 0;
	LINK_STATS_INC(link.err);
	LINK_STATS_INC(link.drop);
	MIB2_STATS_NETIF_INC(ppp->netif, ifoutdiscards);
	pbuf_free(pbuf);

	return err;
}


static err_t
pppossl_netif_output_cb(ppp_pcb* ppp, void* ctx, struct pbuf* pb, u16_t protocol)
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
	return pppossl_write_cb(ppp, ctx, nb);
}


static void
pppossl_send_config_cb(ppp_pcb* ppp, void* ctx, u32_t accm, int pcomp, int accomp)
{
	LWIP_UNUSED_ARG(ctx);
	LWIP_UNUSED_ARG(accm);
	LWIP_UNUSED_ARG(ppp);
	LWIP_UNUSED_ARG(pcomp);
	LWIP_UNUSED_ARG(accomp);
}


static void
pppossl_recv_config_cb(ppp_pcb* ppp, void* ctx, u32_t accm, int pcomp, int accomp)
{
	LWIP_UNUSED_ARG(ctx);
	LWIP_UNUSED_ARG(accm);
	LWIP_UNUSED_ARG(ppp);
	LWIP_UNUSED_ARG(pcomp);
	LWIP_UNUSED_ARG(accomp);
}


static void
pppossl_netif_removed_cb(struct netif* netif)
{
	if (netif && netif->state) {
		pppossl_context* const ppp_ctx = netif->state;
		if (ppp_ctx->ppp_pcb)
			ppp_free(ppp_ctx->ppp_pcb);
		mem_free(netif->state);
	}
}


#else

err_t ppif_init(struct netif* netif)
{
	return ERR_IF;
}

#endif /* PPP_SUPPORT */
