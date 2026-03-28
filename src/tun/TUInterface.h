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
		void close() override;


		/**
		 * Returns the net mask assigned to this interface.
		*/
		//int netmask() const override;

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
