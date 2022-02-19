/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "net/pppossl.h"
#include "net/TlsSocket.h"
#include "net/WinsOutputQueue.h"

#include "tools/Logger.h"
#include "tools/Counters.h"

namespace net {

	class PPInterface
	{
	public:
		explicit PPInterface(TlsSocket& tunnel, Counters& counters);
		~PPInterface();

		/*
		* Opens a PPP interface.   
		* PPP traffic is ???? into the tunnel. 
		*/
		bool open();

		/*
		* Initiates the end of the PPP over SSL interface.
		* 
		*/
		void close();

		void release();

		inline bool if4_up() const noexcept { return  _pcb && _pcb->if4_up; }

		/* True when data is available in the output queue and must be transmitted
		*  to the peer.
		*/
		inline bool must_transmit() const noexcept { return _output_queue.len() > 0; }

		inline bool dead() const noexcept { return !_pcb || _pcb->phase == PPP_PHASE_DEAD; }

//		inline char* addr() const { return ipaddr_ntoa_r(_pcb->netif->ip_addr); }

		/* Writes PPP data available in the output queue to the tunnel. The counter is updated
		*  with the amount of bytes written to the socket. The function returns false if the socket
		*  was closed or if an error occurred.
		*/
		bool send();

		/* Reads any data from the tunnel and pass it to the ppp stack. The counter is updated
		*  with the amount of bytes read from the socket. The function returns false if the socket
		*  was closed or if an error occurred.
		*/
		bool recv();

		/* Sends a keep alive packet if nothing was sent during the last minute.
		*  A LCP Discard request is sent to the fortigate.
		*/
		void send_keep_alive();


	private:
		friend u32_t ppp_output_cb(ppp_pcb *pcb, struct pbuf* pbuf, void *ctx);
		friend void	 ppp_link_status_cb(ppp_pcb *pcb, int err_code, void *ctx);

		/*
		* Returns the last transmission timeout.
		*/
		int last_xmit() const;

		// a reference to the application logger
		tools::Logger* const _logger;

		// socket connected to the firewall
		TlsSocket&  _tunnel;

		// Counters of bytes sent to / received from the tunnel
		tools::Counters& _counters;

		// The lwip network interface and the control block.  Received 
		// data are passed to that interface
		struct ::netif _nif;
		::ppp_pcb* _pcb;

		// The output queue.  All data in this queue are sent
		// through the tunnel. 
		WinsOutputQueue _output_queue;
	};
}
