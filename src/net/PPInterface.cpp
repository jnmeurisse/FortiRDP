/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "net/PPInterface.h"
#include "lwip/dns.h"

#include "tools/ErrUtil.h"
#include "tools/SysUtil.h"

#include "lwip/timeouts.h"

namespace net {
	using namespace tools;

	PPInterface::PPInterface(TlsSocket& tunnel, Counters& counters) :
		_logger(Logger::get_logger()),
		_pcb(nullptr),
		_tunnel(tunnel),
		_counters(counters),
		_output_queue(32)
	{
		DEBUG_CTOR(_logger, "PPInterface");
	}


	PPInterface::~PPInterface()
	{
		DEBUG_DTOR(_logger, "PPInterface");

		if (_pcb)
			ppp_free(_pcb);
	}


	bool PPInterface::open()
	{
		DEBUG_ENTER(_logger, "PPInterface", "open");

		if (!_tunnel.is_connected()) {
			_logger->error("ERROR: PP Interface - tunnel not connected.");
			return false;
		}

		if (_pcb) {
			_logger->error("ERROR: PP Interface already initialized");
			return false;
		}

		// Create a PPP over SSLVPN connection
		_pcb = pppossl_create(&_nif, ppp_output_cb, ppp_link_status_cb, this);
		if (_pcb == nullptr) {
			_logger->error("ERROR: PP Interface - memory allocation failure.");
			return false;
		}

		// IP traffic is routed to that interface
		ppp_set_default(_pcb);

		// FortiGate does not support these options, disable it 
		_pcb->lcp_wantoptions.neg_accompression = false;
		_pcb->lcp_wantoptions.neg_pcompression = false;
		_pcb->lcp_wantoptions.neg_asyncmap = false;

		// Start the connection.  The ppp_link_status_cb will be called
		// by the lwip stack to report the connection success/failure.
		ppp_err rc_con = ppp_connect(_pcb, 0);
		if (rc_con != PPPERR_NONE) {
			ppp_free(_pcb);
			_pcb = nullptr;

			_logger->error("ERROR: ppp_connect failure - %s.",
				ppp_errmsg(rc_con).c_str());
		}

		return rc_con == PPPERR_NONE;
	}


	void PPInterface::close()
	{
		DEBUG_ENTER(_logger, "PPInterface", "close");

		if (_pcb) {
			ppp_err rc = ppp_close(_pcb, 0);

			if (rc != PPPERR_NONE) {
				_logger->error("ERROR: ppp_close failure - %s.",
					ppp_errmsg(rc).c_str());
			}
		}

		return;
	}


	void PPInterface::release()
	{
		DEBUG_ENTER(_logger, "PPInterface", "release");

		if (_pcb) {
			if (_pcb->phase != PPP_PHASE_DEAD) {
				_logger->error("ERROR: releasing an active ppp interface.");
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
		TRACE_ENTER(_logger, "PPInterface", "send");
		bool rc;

		if (!_output_queue.empty()) {
			const netctx_snd_status snd_status = _output_queue.write(_tunnel);
			switch (snd_status.status_code) {
			case NETCTX_SND_OK:
				rc = true;
				_counters.sent += snd_status.sbytes;
				break;

			case NETCTX_SND_RETRY:
				rc = true;
				break;

			case NETCTX_SND_ERROR:
			default:
				// an error has occurred
				rc = false;
				_logger->error("ERROR: tunnel send failure");
				_logger->error(mbed_errmsg(snd_status.errnum).c_str());
				break;
			}
		}
		else {
			rc = true;
		}

		if (_logger->is_trace_enabled()) {
			_logger->trace(
				".... %x       PPInterface::send socket fd=%d rc=%d",
				std::addressof(this),
				_tunnel.get_fd(),
				rc);
		}

		return rc;
	}


	bool PPInterface::recv()
	{
		TRACE_ENTER(_logger, "PPInterface", "recv");
		byte buffer[4096];
		bool rc;

		// read what is available from the tunnel
		const netctx_rcv_status rcv_status = _tunnel.recv(buffer, sizeof(buffer));

		switch (rcv_status.status_code) {
		case NETCTX_RCV_OK: {
			rc = true;
			_counters.received += rcv_status.rbytes;

			// PPP data available, pass it to the stack.
			const ppp_err ppp_rc = pppossl_input(_pcb, buffer, rcv_status.rbytes);
			if (ppp_rc) {
				_logger->error("ERROR: pppossl_input failure - %s",
					ppp_errmsg(ppp_rc).c_str());

				rc = false;
			}
		}
		break;

		case NETCTX_RCV_RETRY:
			rc = true;
			break;

		case NETCTX_RCV_EOF:
			// socket was closed by peer
			rc = false;
			_logger->error("ERROR: tunnel closed by peer.");
			break;

		case NETCTX_RCV_ERROR:
		default:
			// an error has occurred
			rc = false;
			_logger->error("ERROR: tunnel receive failure");
			_logger->error(mbed_errmsg(rcv_status.errnum).c_str());
			break;
		}

		if (_logger->is_trace_enabled()) {
			_logger->trace(
				".... %x       PPInterface::recv socket fd=%d rc=%d",
				std::addressof(this),
				_tunnel.get_fd(),
				rc);
		}

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
		pppossl_pcb* const pcbssl = (pppossl_pcb *)_pcb->link_ctx_cb;

		return pcbssl->last_xmit;
	}


	static u32_t ppp_output_cb(ppp_pcb *pcb, struct pbuf* pbuf, void *ctx)
	{
		LWIP_UNUSED_ARG(pcb);
		net::PPInterface* const pp_interface = (net::PPInterface *)ctx;

		return pp_interface->_output_queue.append(pbuf);
	}


	static void	ppp_link_status_cb(ppp_pcb *pcb, int err_code, void *ctx)
	{
		LWIP_UNUSED_ARG(pcb);
		net::PPInterface* pp_interface = (net::PPInterface *)ctx;

		if (err_code) {
			Logger* const logger = pp_interface->_logger;

			if (err_code == PPPERR_USER) {
				// The ppp interface is now down.
				logger->trace(".... %x ppp_link_status_cb interface is down", pp_interface);
			} 
			else
			{
				logger->error("ERROR: ppp link error - %s", ppp_errmsg(err_code).c_str());
			}
		} 

		return;
	}
}