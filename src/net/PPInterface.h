/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include <lwip/arch.h>
#include <lwip/pbuf.h>
#include <lwip/netif.h>
#include "net/pppossl.h"
#include "net/TlsSocket.h"
#include "net/OutputQueue.h"
#include "util/Logger.h"
#include "util/Counters.h"


namespace net {

	class PPInterface final
	{
	public:
		explicit PPInterface(TlsSocket& tunnel, Counters& counters);
		~PPInterface();

		/**
		 * Opens a PPP interface.
		*/
		bool open();

		/**
		 * Initiates the end of the PPP over SSL interface.
		*/
		void close(bool nocarrier);

		/**
		 * Releases all resources.
		*/
		void release();

		/**
		 * Returns true if the PPP interface is up.
		*/
		inline bool if4_up() const noexcept { return  _pcb && _pcb->if4_up; }

		/**
		 * Returns true if the PPP interface is dead.
		*/
		inline bool dead() const noexcept { return !_pcb || _pcb->phase == PPP_PHASE_DEAD; }

		/**
		 * Returns true when data is available in the output queue and must be transmitted
		 * to the peer.
		*/
		inline bool must_transmit() const noexcept { return _output_queue.size() > 0; }

		/**
		 * Returns the IP address assigned to this interface.
		*/
		std::string addr() const;

		/**
		 * Returns the net mask assigned to this interface.
		*/
		int netmask() const;

		/**
		 * Returns the gateway IP address assigned to this interface.
		*/
		std::string gateway() const;

		/**
		 * Returns the network MTU.
		*/
		int mtu() const;

		/**
		 * Writes PPP data available in the output queue to the tunnel.
		 *
		 * The internal counters are updated with the amount of bytes written
		 * to the socket. The function returns false if the socket was closed
		 * or if an error occurred.
		*/
		bool send();

		/**
		 * Reads any data from the tunnel and pass it to the PPP stack.
		 *
		 * The internal counters are updated  with the amount of bytes read
		 * from the socket. The function returns false if the socket was closed
		 * or if an error occurred.
		*/
		bool recv();

		/**
		 * Sends a keep alive packet.
		 * 
		 * Note: the keep alive packet is sent only if if nothing was sent
		 * during the last minute.  The keep alive packet is a LCP Discard
		 * sent to the FortiGate PPP interface.
		*/
		void send_keep_alive();

	private:
		friend u32_t ppp_output_cb(ppp_pcb *pcb, struct pbuf* pbuf, void *ctx);
		friend void ppp_link_status_cb(ppp_pcb *pcb, int err_code, void *ctx);

		/**
		 * @return the last transmission timeout.
		*/
		int last_xmit() const;

		// The class name
		static const char* __class__;

		// A reference to the application logger.
		utl::Logger* const _logger;

		// socket connected to the firewall.
		TlsSocket&  _tunnel;

		// Counters of bytes sent to / received from the tunnel.
		utl::Counters& _counters;

		// The lwIP network interface and the control block.  Received 
		// data are passed to that interface.
		struct ::netif _nif;
		::ppp_pcb* _pcb;

		// The output queue.  All data in this queue are sent
		// through the tunnel. 
		OutputQueue _output_queue;
	};

}
