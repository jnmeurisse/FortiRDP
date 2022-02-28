/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "net/PPInterface.h"

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

		if (!_tunnel.connected()) {
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



	bool PPInterface::send()
	{
		TRACE_ENTER(_logger, "PPInterface", "send");

		if (!_output_queue.empty()) {
			size_t written;
			const int rc = _output_queue.write(_tunnel, written);

			if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE)
				return true;

			if (rc < 0) {
				// an error has occurred
				_logger->error("ERROR: tunnel send failure");
				_logger->error(mbed_errmsg(rc).c_str());

				return false;
			}

			_counters.sent += written;
			if (_logger->is_trace_enabled())
				_logger->trace(
					".... %x       PPInterface::send socket fd=%d rc=%d",
					this,
					_tunnel.get_fd(),
					rc);

//			if (written > 0)
//				_tunnel.flush();
		}

		return true;
	}


	bool PPInterface::recv()
	{
		TRACE_ENTER(_logger, "PPInterface", "recv");
		byte buffer[4096];

		// read what is available from the tunnel
		const int rc = _tunnel.recv(buffer, sizeof(buffer));

		if (rc == MBEDTLS_ERR_SSL_WANT_READ || rc == MBEDTLS_ERR_SSL_WANT_WRITE)
			return true;

		if (rc == 0) {
			// socket was closed by peer
			_logger->error("ERROR: tunnel closed by peer.");
			return false;
		}

		if (rc < 0) {
			// an error has occurred
			_logger->error("ERROR: tunnel receive failure");
			_logger->error(mbed_errmsg(rc).c_str());

			return false;
		}

		_counters.received += rc;
		if (_logger->is_trace_enabled())
			_logger->trace(
				".... %x       PPInterface::recv socket fd=%d rc=%d",
				this,
				_tunnel.get_fd(),
				rc);

		// PPP data available, pass it to the stack.
		const ppp_err ppp_rc = pppossl_input(_pcb, buffer, rc);
		if (ppp_rc) {
			_logger->error("ERROR: pppossl_input failure - %s",
				ppp_errmsg(ppp_rc).c_str());

			return false;
		}

		return true;
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