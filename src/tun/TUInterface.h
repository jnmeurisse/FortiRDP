/*!
* This file is part of FortiRDP
*
* Copyright (C) 2026 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include "net/TlsSocket.h"
#include "tun/InnerInterface.h"


namespace tun {

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
		 * Returns the net mask assigned to this interface.
		*/
		//int netmask() const override;

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

		// IP address of ths interface
		net::IpAddress _ip_address;
	};

}
