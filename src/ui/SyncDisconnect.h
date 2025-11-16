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
#include "fw/FirewallClient.h"
#include "fw/FirewallTunnel.h"


namespace ui {

	/**
	* A synchronous procedure that disconnects this client from the firewall and terminates the tunnel
	* listeners. The procedure posts a DisconnectedEvent to the main window when done.
	*/
	class SyncDisconnect final : public SyncProc
	{
	public:
		explicit SyncDisconnect(HWND hwnd, fw::FirewallClient& portal_client, fw::FirewallTunnel* tunnel);
		~SyncDisconnect();

	private:
		// The class name.
		static const char* __class__;

		//- portal to disconnect from.
		fw::FirewallClient& _portal_client;

		//- tunnel to terminate if not null.
		fw::FirewallTunnel* const _tunnel;

		// disconnect procedure.
		virtual bool procedure() override;
	};

}
