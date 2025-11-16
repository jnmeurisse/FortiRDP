/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "PPInterface.h"

#include <lwip/timeouts.h>
#include <lwip/stats.h>
#include "tools/ErrUtil.h"


namespace net {

	using namespace tools;

	PPInterface::PPInterface(TlsSocket& tunnel, Counters& counters) :
		_logger(Logger::get_logger()),
		_pcb(nullptr),
		_tunnel(tunnel),
		_counters(counters),
		_nif(),
		_output_queue(32 * 1024)
	{
		DEBUG_CTOR(_logger);
	}


	PPInterface::~PPInterface()
	{
		DEBUG_DTOR(_logger);

		if (_pcb)
			ppp_free(_pcb);
	}


	bool PPInterface::open()
	{
		DEBUG_ENTER(_logger);

		if (!_tunnel.is_connected()) {
			_logger->error("ERROR: %s - tunnel not connected");
			return false;
		}

		if (_pcb) {
			_logger->error("ERROR: %s already initialized");
			return false;
		}

		
		// initialize lwIP statistics
		stats_init();

		// Create a PPP over the SSLVPN connection.
		_pcb = pppossl_create(&_nif, ppp_output_cb, ppp_link_status_cb, this);
		if (_pcb == nullptr) {
			_logger->error("ERROR: pppossl_create - memory allocation failure");
			return false;
		}

		// IP traffic is routed through that interface.
		ppp_set_default(_pcb);

		// FortiGate does not support these options, disable it.
		_pcb->lcp_wantoptions.neg_accompression = false;
		_pcb->lcp_wantoptions.neg_pcompression = false;
		_pcb->lcp_wantoptions.neg_asyncmap = false;

		// Start the connection.  The ppp_link_status_cb will be called
		// by the lwIP stack to report the connection success/failure.
		ppp_err rc_con = ppp_connect(_pcb, 0);
		if (rc_con != PPPERR_NONE) {
			ppp_free(_pcb);
			_pcb = nullptr;

			_logger->error("ERROR: %s - connect failure (%s)",
				__class__,
				ppp_errmsg(rc_con).c_str()
			);
		}

		return rc_con == PPPERR_NONE;
	}


	void PPInterface::close(bool nocarrier)
	{
		DEBUG_ENTER(_logger);

		if (_logger->is_debug_enabled())
			stats_display();

		if (!dead()) {

			ppp_err rc = ppp_close(_pcb, nocarrier? 1 : 0);

			if (rc != PPPERR_NONE) {
				_logger->error("ERROR: %s - close failure (%s)",
					__class__,
					ppp_errmsg(rc).c_str()
				);
			}
		}

		return;
	}


	void PPInterface::release()
	{
		DEBUG_ENTER(_logger);

		if (_pcb) {
			if (_pcb->phase != PPP_PHASE_DEAD) {
				_logger->error("ERROR: %s - active interface released", __class__);
			}

			ppp_free(_pcb);
			_pcb = nullptr;
		}
	}


	std::string PPInterface::addr() const
	{
		return std::string(ip4addr_ntoa(netif_ip4_addr(_pcb->netif)));
	}


	int PPInterface::netmask() const
	{
		const ip4_addr_t *mask = netif_ip4_netmask(_pcb->netif);
		int mask_size = 0;
		u32_t mask_test = 0x8000;

		for (int i = 0; i < 32; i++) {
			if (mask->addr & mask_test)
				mask_size++;

			mask_test = mask_test >> 1;
		}
		
		return mask_size;
	}


	std::string PPInterface::gateway() const
	{
		return std::string(ip4addr_ntoa(netif_ip4_gw(_pcb->netif)));
	}


	int PPInterface::mtu() const
	{
		return _pcb->netif->mtu;
	}


	bool PPInterface::send()
	{
		TRACE_ENTER(_logger);
		bool rc;

		if (!_output_queue.is_empty()) {
			const snd_status status{ _output_queue.write(_tunnel) };

			switch (status.code) {
			case snd_status_code::NETCTX_SND_OK:
				rc = true;
				_counters.sent += status.sbytes;
				break;

			case snd_status_code::NETCTX_SND_RETRY:
				rc = true;
				break;

			case snd_status_code::NETCTX_SND_ERROR:
			default:
				rc = false;
				_logger->error("ERROR: %s - tunnel send failure", __class__);
				_logger->error(mbed_errmsg(status.rc).c_str());
				break;
			}
		}
		else {
			rc = true;
		}

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x       %s::%s socket fd=%d rc=%d",
				(uintptr_t)this,
				__class__,
				__func__,
				_tunnel.get_fd(),
				rc
			);

		return rc;
	}


	bool PPInterface::recv()
	{
		TRACE_ENTER(_logger);

		byte buffer[4096];
		bool rc;

		// Read data available in the tunnel.
		const rcv_status status{ _tunnel.recv_data(buffer, sizeof(buffer)) };

		switch (status.code) {
		case rcv_status_code::NETCTX_RCV_OK: {
			rc = true;
			_counters.received += status.rbytes;

			// PPP data available, pass it to the lwIP stack.
			const ppp_err ppp_rc = pppossl_input(_pcb, buffer, status.rbytes);
			if (ppp_rc) {
				_logger->error("ERROR: %s - input failure (%s)",
					__class__,
					ppp_errmsg(ppp_rc).c_str());

				rc = false;
			}
		}
		break;

		case rcv_status_code::NETCTX_RCV_RETRY:
			rc = true;
			break;

		case rcv_status_code::NETCTX_RCV_EOF:
			// the PPPP socket was closed by peer.
			rc = false;
			break;

		case rcv_status_code::NETCTX_RCV_ERROR:
		default:
			rc = false;
			_logger->error("ERROR: %s - tunnel receive failure", __class__);
			_logger->error(mbed_errmsg(status.rc).c_str());
			break;
		}

		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x       %s::%s socket fd=%d rc=%d",
				(uintptr_t)this,
				__class__,
				__func__,
				_tunnel.get_fd(),
				rc);

		return rc;
	}


	void PPInterface::send_keep_alive()
	{
		if (_pcb && (_pcb->lcp_fsm.state == PPP_FSM_OPENED) && (sys_now() - last_xmit() > 60 * 1000)) {
			ppossl_send_ka(_pcb);
		}
	}


	int PPInterface::last_xmit() const
	{
		pppossl_pcb* const pcbssl = reinterpret_cast<pppossl_pcb *>(_pcb->link_ctx_cb);

		return pcbssl->last_xmit;
	}


	u32_t ppp_output_cb(ppp_pcb *pcb, struct pbuf* pbuf, void *ctx)
	{
		LWIP_UNUSED_ARG(pcb);
		net::PPInterface* const pp_interface = reinterpret_cast<net::PPInterface *>(ctx);

		return pp_interface->_output_queue.push(pbuf) ? pbuf->tot_len : 0;
	}


	void ppp_link_status_cb(ppp_pcb *pcb, int err_code, void *ctx)
	{
		LWIP_UNUSED_ARG(pcb);
		net::PPInterface* const pp_interface = reinterpret_cast<net::PPInterface*>(ctx);

		if (err_code) {
			Logger* const logger = pp_interface->_logger;

			if (err_code == PPPERR_USER) {
				// The PPP interface is now down.
				logger->trace(".... %x ppp_link_status_cb interface is down", (uintptr_t)pp_interface);
			} 
			else
			{
				logger->error("ERROR: PPInterface - link error (%s)", ppp_errmsg(err_code).c_str());
			}
		} 

		return;
	}

	const char* PPInterface::__class__ = "PPInterface";

}
