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
#include "net/TlsSocket.h"
#include "tun/InnerInterface.h"


namespace tun {

	class PPInterface final : public tun::InnerInterface
	{
	public:
		explicit PPInterface(net::TlsSocket& tunnel, const net::IpAddress& address);
		~PPInterface();

		/**
		 * Opens a PPP interface.
		*/
		bool open() override;

		/**
		 * Initiates the end of the PPP over SSL interface.
		*/
		void close(bool nocarrier) override;

		/**
		 * Releases all resources.
		*/
		void release() override;

		/**
		 * Returns true if the PPP interface is up.
		*/
		bool is_if_up() const noexcept override;

		/**
		 * Returns true if the PPP interface is dead.
		*/
		bool is_if_dead() const noexcept override;

		/**
		 * Returns the net mask assigned to this interface.
		*/
		//int netmask() const override;

		/**
		 * Reads any data from the tunnel and pass it to the PPP stack.
		 *
		 * The internal counters are updated  with the amount of bytes read
		 * from the socket. The function returns false if the socket was closed
		 * or if an error occurred.
		*/
		bool recv() override;

		/**
		 * Sends a keep alive packet.
		 * 
		 * Note: the keep alive packet is sent only if if nothing was sent
		 * during the last minute.  The keep alive packet is a LCP Discard
		 * sent to the FortiGate PPP interface.
		*/
		void send_keep_alive() override;

	private:
		/**
		 * @return the last transmission timeout.
		*/
		int last_xmit() const;

		// The class name
		static const char* __class__;
	};

}
