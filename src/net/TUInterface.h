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
#include "net/pppossl.h"
#include "net/InnerInterface.h"
#include "net/TlsSocket.h"


namespace net {

	class TUInterface final : public InnerInterface
	{
	public:
		explicit TUInterface(net::TlsSocket& tunnel, const net::IpAddress& address);
		~TUInterface();

		/**
		 * Opens a TUN interface.
		*/
		bool open() override;

		/**
		 * Initiates the end of the TUN over SSL interface.
		*/
		void close(bool nocarrier) override;

		/**
		 * Releases all resources.
		*/
		void release() override;

		/**
		 * Returns true if the TUN interface is up.
		*/
		bool is_if_up() const noexcept override;

		/**
		 * Returns true if the TUN interface is dead.
		*/
		bool is_if_dead() const noexcept override;

		/**
		 * Returns the IP address assigned to this interface.
		*/
		std::string addr() const override;

		/**
		 * Returns the net mask assigned to this interface.
		*/
		int netmask() const override;

		/**
		 * Returns the gateway IP address assigned to this interface.
		*/
		std::string gateway() const override;

		/**
		 * Returns the network MTU.
		*/
		int mtu() const override;

		/**
		 * Writes TUN data available in the output queue to the tunnel.
		 *
		 * The internal counters are updated with the amount of bytes written
		 * to the socket. The function returns false if the socket was closed
		 * or if an error occurred.
		*/
		bool send() override;

		/**
		 * Reads any data from the tunnel and pass it to the TUN stack.
		 *
		 * The internal counters are updated  with the amount of bytes read
		 * from the socket. The function returns false if the socket was closed
		 * or if an error occurred.
		*/
		bool recv() override;

		/**
		 * Sends a keep alive packet.
		 * 
		*/
		void send_keep_alive() override;

	private:
		/**
		 * @return the last transmission timeout.
		*/
		int last_xmit() const;

		// The class name
		static const char* __class__;

		::ppp_pcb* _pcb;
	};

}
