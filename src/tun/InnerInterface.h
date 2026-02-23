/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include <lwip/netif.h>
#include "net/IpAddress.h"
#include "net/TlsSocket.h"
#include "tun/OutputQueue.h"
#include "util/Logger.h"
#include "util/Counters.h"


namespace tun {

	class InnerInterface
	{
	public:
		explicit InnerInterface(net::TlsSocket& tunnel);
		virtual ~InnerInterface();

		/**
		 * Opens the interface.
		*/
		virtual bool open() = 0;

		/**
		 * Closes the interface.
		*/
		virtual void close(bool nocarrier) = 0;

		/**
		 * Releases all resources.
		*/
		virtual void release() = 0;

		/**
		* Returns true if the interface is up.
		*/
		virtual bool is_if_up() const noexcept = 0;

		/**
		 * Returns true if the interface is dead.
		*/
		virtual bool is_if_dead() const noexcept = 0;

		/**
		 * Returns true when data is available in the output queue and must be transmitted
		 * to the peer.
		*/
		inline bool must_transmit() const noexcept { return !_output_queue.is_empty(); }

		/**
		 * Returns the IP address assigned to this interface.
		*/
		net::IpAddress addr() const;

		/**
		 * Returns the net mask assigned to this interface.
		*/
		net::IpAddress netmask() const;

		/**
		 * Returns the gateway IP address assigned to this interface.
		*/
		net::IpAddress gateway() const;

		/**
		 * Returns the network MTU.
		*/
		int mtu() const;

		/**
		 * Writes data available in the output queue into the tunnel.
		 *
		 * The internal counters are updated with the amount of bytes written
		 * to the socket. The function returns false if the socket was closed
		 * or if an error occurred.
		*/
		virtual bool send();

		/**
		 * Reads any data from the tunnel and pass it to the IP stack.
		 *
		 * The internal counters are updated  with the amount of bytes read
		 * from the socket. The function returns false if the socket was closed
		 * or if an error occurred.
		*/
		virtual bool recv() = 0;

		/**
		 * Sends a keep alive packet.
		 *
		 * Note: the keep alive packet is sent only if if nothing was sent
		 * during the last minute.
		*/
		virtual void send_keep_alive() = 0;

		/**
		 * Returns the transmitted/received bytes counters.
		*/
		inline const utl::Counters& counters() const noexcept { return _counters; }

	private:
		// The class name
		static const char* __class__;

	protected:
		// A reference to the application logger.
		utl::Logger* const _logger;

		// The lwIP internal network interface.
		// Received data are passed to that interface.
		struct ::netif _nif;

		// socket connected to the firewall.
		net::TlsSocket& _tunnel;

		// Counters of bytes sent to / received from the tunnel.
		utl::Counters _counters;

		// The output queue.
		// All data in this queue are sent through the tunnel. 
		tun::OutputQueue _output_queue;
	};

}