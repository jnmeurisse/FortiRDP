/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "TUNInterface.h"

#include <array>
#include <lwip/stats.h>
#include "util/ErrUtil.h"


namespace net {
	using namespace utl;

	// lwip callbacks
	u32_t tun_output_cb(struct netif* netif, struct pbuf* p, const ip4_addr_t* ipaddr);


	TUInterface::TUInterface(net::TlsSocket& tunnel, const net::IpAddress& address) :
		InnerInterface(tunnel, address),
		_pcb(nullptr)
	{
		DEBUG_CTOR(_logger);

		_nif.name[0] = 't';
		_nif.name[1] = 'u';
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

		if (_pcb) {
			_logger->error("ERROR: %s already initialized");
			return false;
		}

		// initialize lwIP statistics
		::stats_init();

		// Create a TUN over the SSLVPN connection.
		//_nif = netif_add(_nif, )


		// IP traffic is routed through that interface.
		netif_set_default(&_nif);
		netif_set_up(&_nif);

		// Start the connection.

		
	}


	void TUInterface::close(bool nocarrier)
	{
		DEBUG_ENTER(_logger);

		if (_logger->is_debug_enabled())
			::stats_display();

		if (!is_if_dead()) {
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


	std::string TUInterface::addr() const
	{
	}


	int TUInterface::netmask() const
	{
	}


	std::string TUInterface::gateway() const
	{
	}


	int TUInterface::mtu() const
	{
	}


	bool TUInterface::send()
	{
		TRACE_ENTER(_logger);
		mbed_err rc = 0;

		if (!_output_queue.is_empty()) {
			size_t written = 0;
			rc = _output_queue.write(_tunnel, written);
			LOG_TRACE(_logger, "rc=%d sbytes=%zu", rc, written);

			if (rc == 0) {
				_counters.sent += written;
			}
			else {
				_logger->error("ERROR: %s - tunnel send failure (%d)", __class__, rc);
			}
		}

		LOG_TRACE(_logger, "socket fd=%d rc=%d", _tunnel.get_fd(), rc);

		return rc == 0;
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


	u32_t tun_output_cb(struct netif* netif, struct pbuf* p, const ip4_addr_t* ipaddr)
	{

	}


	const char* TUInterface::__class__ = "TUNInterface";
}
