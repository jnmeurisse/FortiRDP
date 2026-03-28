/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <memory>
#include <string>
#include <lwip/arch.h>
#include <lwip/pbuf.h>
#include "net/TlsSocket.h"
#include "tun/InnerInterface.h"
#include "tun/pppfgt.h"


namespace tun {
	using PPPContext = std::unique_ptr<struct ::pppfgt_context_s, decltype(&::pppfgt_free)>;

	class PPPInterface final : public tun::InnerInterface
	{
	public:
		explicit PPPInterface(net::TlsSocket& tunnel, const net::IpAddress& address);
		~PPPInterface();

		/**
		 * Opens a PPP interface.
		*/
		bool open() override;

		/**
		 * Initiates the end of the PPP over SSL interface.
		*/
		void close() override;

		/**
		 * Sends a keep alive packet.
		 * 
		 * Note: the keep alive packet is sent only if if nothing was sent
		 * during the last minute.  The keep alive packet is a LCP Discard
		 * sent to the FortiGate PPP interface.
		*/
		void send_keep_alive() override;

	protected:
		virtual utl::lwip_err input_bytes(uint8_t* data, size_t size);

	private:
		/**
		 * @return the last transmission timeout.
		*/
		int last_xmit() const;

		// The class name
		static const char* __class__;

		// The PPP context
		PPPContext _context;
	};

}
