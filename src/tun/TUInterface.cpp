/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "TUInterface.h"

#include <array>
#include <lwip/stats.h>
#include "util/ErrUtil.h"


namespace tun {
	using namespace utl;


	TUInterface::TUInterface(net::TlsSocket& tunnel, const net::IpAddress& address) :
		InnerInterface(tunnel),
		_ip_address(address)
	{
		DEBUG_CTOR(_logger);
	}


	TUInterface::~TUInterface()
	{
		DEBUG_DTOR(_logger);
	}


	bool TUInterface::open()
	{
		DEBUG_ENTER(_logger);

		if (!_tunnel.is_connected()) {
			_logger->error("ERROR: %s - tunnel not connected");
			return false;
		}

		if (_nif.state) {
			_logger->error("ERROR: %s already initialized");
			return false;
		}

		// initialize lwIP statistics
		::stats_init();

		// Create a TUN over the SSLVPN connection.
		const lwip_err rc = tunif_init(&_nif);
		if (rc) {
			_logger->error("ERROR: tuinif_init - ");
			return false;
		}

		netif_set_addr(&_nif, &_ip_address.get_address());

		// IP traffic is routed through that interface.
		netif_set_default(&_nif);
		netif_set_up(&_nif);

		_nif.state = (void *)1;
	}


	void TUInterface::close()
	{
		DEBUG_ENTER(_logger);

		if (_logger->is_debug_enabled())
			::stats_display();

		if (!is_if_dead()) {
			const ppp_err rc = ::tun_close(_pcb, nocarrier ? 1 : 0);

			if (rc != PPPERR_NONE) {
//				_logger->error("ERROR: %s - close failure",
//					__class__,
//					ppp_errmsg(rc).c_str()
//				);
			}
		}

		return;
	}


	void TUInterface::release()
	{
		DEBUG_ENTER(_logger);
	}


	bool TUInterface::is_if_up() const noexcept
	{
		return netif_is_up(&_nif);
	}


	bool TUInterface::is_if_dead() const noexcept
	{
	}


	bool TUInterface::recv()
	{
		TRACE_ENTER(_logger);

		std::array<unsigned char, 4096> buffer = {};
		bool rc;

		// Read data available in the tunnel.
		const rcv_status status{ _tunnel.recv_data(buffer.data(), buffer.size()) };
		LOG_TRACE(_logger, "code=%d rc=%d rbytes=%zu",
			status.code,
			status.rc,
			status.rbytes
		);

		switch (status.code) {
		case rcv_status_code::NETCTX_RCV_OK: {
			rc = true;
			_counters.rcvd += status.rbytes;

			// TUN data available, pass it to the lwIP stack.
		}
		break;

		case rcv_status_code::NETCTX_RCV_RETRY:
			rc = true;
			break;

		case rcv_status_code::NETCTX_RCV_EOF:
			// the tunnel socket was closed by peer.
			rc = false;
			break;

		case rcv_status_code::NETCTX_RCV_ERROR:
		default:
			rc = false;
			_logger->error("ERROR: %s - tunnel receive failure", __class__);
			_logger->error(mbed_errmsg(status.rc).c_str());
			break;
		}

		LOG_TRACE(_logger, "socket fd=%d rc=%d", _tunnel.get_fd(), rc);

		return rc;
	}


	void TUInterface::send_keep_alive()
	{
	}


	int TUInterface::last_xmit() const
	{
	}


	u32_t tun_output_cb(struct ::tun_pcb_s* pcb, struct pbuf* p, void* ctx)
	{

	}


	const char* TUInterface::__class__ = "TUInterface";
}
