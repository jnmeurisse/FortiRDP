/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "PPPInterface.h"

#include <array>
#include <lwip/stats.h>
#include "util/ErrUtil.h"


namespace tun {
	using namespace utl;

	// lwip callbacks
	void ppp_link_status_cb(ppp_pcb* pcb, int err_code, void* ctx);

	// Max Xmit idle time (in ms) before sending a PPP Keep alive packet
	constexpr int PPP_MAXIDLE = 60 * 1000;


	PPPInterface::PPPInterface(net::TlsSocket& tunnel, const net::IpAddress& address) :
		InnerInterface(tunnel),
		_context(nullptr, &::pppfgt_free)
	{
		DEBUG_CTOR(_logger);
	}


	PPPInterface::~PPPInterface()
	{
		DEBUG_DTOR(_logger);

		if (is_netif_up()) {
			_logger->error("ERROR: %s - active interface deleted", __class__);
		}
	}


	bool PPPInterface::open()
	{
		DEBUG_ENTER(_logger);

		if (is_netif_up()) {
			_logger->error("ERROR %s : already initialized", __class__);
			return false;
		}

		// initialize lwIP statistics
		::stats_init();

		// Create a PPP context.
		_context.reset(::pppfgt_create(&_netif, ppp_link_status_cb, this));
		if (!_context) {
			_logger->error("ERROR: %s pppossl_create - memory allocation failure", 
				__class__
			);
			return false;
		}

		// Start the connection.  The ppp_link_status_cb will be called
		// by the lwIP stack to report the connection success/failure.
		const lwip_err rc = ::pppfgt_connect(_context.get());
		if (rc != ERR_OK) {
			_logger->error("ERROR: %s - pppossl_connect failure (%s)",
				__class__,
				lwip_errmsg(rc).c_str()
			);
		}

		return rc == ERR_OK;
	}


	void PPPInterface::close()
	{
		DEBUG_ENTER(_logger);

		if (_logger->is_debug_enabled())
			::stats_display();

		const lwip_err rc = ::pppfgt_disconnect(_context.get());
		if (rc != ERR_OK) {
			_logger->error("ERROR: %s - close failure (%s)",
				__class__,
				lwip_errmsg(rc).c_str()
			);
		}

		return;
	}


	void PPPInterface::send_keep_alive()
	{
//		if (_pcb && (_pcb->lcp_fsm.state == PPP_FSM_OPENED) && (sys_now() - last_xmit() > PPP_MAXIDLE)) {
//			ppossl_send_ka(_pcb);
//		}
	}


	int PPPInterface::last_xmit() const
	{
		//		auto pcbssl = static_cast<const pppossl_pcb*>(_pcb->link_ctx_cb);

		//return pcbssl->last_xmit;
	}


	utl::lwip_err PPPInterface::input_bytes(uint8_t* data, size_t size)
	{
		return ::pppfgt_input_bytes(_context.get(), data, size);
	}


	void ppp_link_status_cb(ppp_pcb* pcb, int err_code, void* ctx)
	{
	//	LWIP_UNUSED_ARG(pcb);
	//	auto pp_interface = static_cast<PPPInterface*>(ctx);

	//	if (err_code) {
	//		Logger* const logger = pp_interface->_logger;

	//		if (err_code == PPPERR_USER) {
	//			// The PPP interface is now down.
	//			logger->trace("ppp_link_status_cb interface 0x%012Ix is down", PTR_VAL(pp_interface));
	//		}
	//		else
	//		{
	//			logger->error("ERROR: PPInterface - link error (%s)", ppp_errmsg(err_code).c_str());
	//		}
	//	}

		return;
	}

	const char* PPPInterface::__class__ = "PPPInterface";
}
