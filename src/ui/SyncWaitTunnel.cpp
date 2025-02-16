/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SyncWaitTunnel.h"


namespace ui {

	SyncWaitTunnel::SyncWaitTunnel(HWND hwnd, fw::FirewallTunnel& tunnel) :
		SyncProc(hwnd, AsyncMessage::TunnelListeningEvent),
		_tunnel(tunnel)
	{
		DEBUG_CTOR(_logger, "SyncWaitTunnel");
	}


	SyncWaitTunnel::~SyncWaitTunnel()
	{
		DEBUG_DTOR(_logger, "SyncWaitTunnel");
	}


	bool SyncWaitTunnel::procedure()
	{
		DEBUG_ENTER(_logger, "SyncWaitTunnel", "procedure");
		bool started = false;

		// connect the socket and launch the listener thread.
		if (_tunnel.start()) {
			// Wait until the listener is in LISTENING state.
			started = _tunnel.wait_listening(7000);
		}

		// Return that tunneler is listening or has failed to start
		return started;
	}

}
