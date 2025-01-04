/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include "ui/SyncProc.h"
#include "fw/PortalClient.h"
#include "net/Tunneler.h"


namespace ui {

	/**
	* A synchronous procedure that disconnects this client from the firewall and terminates the tunnel
	* listeners. The procedure posts a DisconnectedEvent to the window when done.
	*/
	class SyncDisconnect final : public SyncProc
	{
	public:
		explicit SyncDisconnect(HWND hwnd, fw::PortalClient* portal, net::Tunneler* tunnel);
		~SyncDisconnect();

	private:
		//- portal to disconnect from
		fw::PortalClient* const _portal;

		//- tunnel to terminate
		net::Tunneler* const _tunnel;

		virtual bool procedure() override;
	};

}
