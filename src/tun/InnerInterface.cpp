/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "InnerInterface.h"

#include <array>
#include "util/Logger.h"

namespace tun {
	using namespace utl;


	err_t netif_linkoutput_cb(struct netif* netif, struct pbuf* p)
	{
		InnerInterface* itf = static_cast<InnerInterface*>(netif->state);
		return itf->_output_queue.push(p) ? ERR_OK : ERR_IF;
	}


	InnerInterface::InnerInterface(net::TlsSocket& tunnel):
		_logger(Logger::get_logger()),
		_tunnel(tunnel),
		_counters(),
		_netif(),
		_output_queue(32 * 1024)
	{
		DEBUG_CTOR(_logger);

		_netif.state = this;
		_netif.linkoutput = netif_linkoutput_cb;
	}


	InnerInterface::~InnerInterface()
	{
		DEBUG_DTOR(_logger);
	}


	net::IpAddress InnerInterface::addr() const
	{
		return net::IpAddress(_netif.ip_addr);
	}


	net::IpAddress InnerInterface::netmask() const
	{
		return net::IpAddress(_netif.netmask);
	}


	net::IpAddress InnerInterface::gateway() const
	{
		return net::IpAddress(_netif.gw);
	}


	int InnerInterface::mtu() const
	{
		return _netif.mtu;
	}


	bool InnerInterface::send()
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


	bool InnerInterface::recv()
	{
		TRACE_ENTER(_logger);

		std::array<unsigned char, 4096> buffer = {};
		bool rc;

		// Read data available in the tunnel.
		const net::rcv_status status{ _tunnel.recv_data(buffer.data(), buffer.size()) };
		LOG_TRACE(_logger, "code=%d rc=%d rbytes=%zu",
			status.code,
			status.rc,
			status.rbytes
		);

		switch (status.code) {
		case net::rcv_status_code::NETCTX_RCV_OK: {
			rc = true;
			_counters.rcvd += status.rbytes;

			// data available, pass it to the lwIP stack.
			const lwip_err lwip_rc = input_bytes(buffer.data(), status.rbytes);
			if (lwip_rc) {
				_logger->error("ERROR: %s - input failure (%s)",
					__class__,
					lwip_errmsg(lwip_rc).c_str());

				rc = false;
			}
		}
		break;

		case net::rcv_status_code::NETCTX_RCV_RETRY:
			rc = true;
			break;

		case net::rcv_status_code::NETCTX_RCV_EOF:
			// the tunnel socket was closed by peer.
			rc = false;
			break;

		case net::rcv_status_code::NETCTX_RCV_ERROR:
		default:
			rc = false;
			_logger->error("ERROR: %s - tunnel receive failure", __class__);
			_logger->error(mbed_errmsg(status.rc).c_str());
			break;
		}

		LOG_TRACE(_logger, "socket fd=%d rc=%d", _tunnel.get_fd(), rc);

		return rc;
	}

}
