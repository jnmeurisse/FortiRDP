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
		SyncProc(hwnd, AsyncMessage::TunnelListeningEvent.get()),
		_tunnel(tunnel)
	{
		DEBUG_CTOR(_logger);
	}


	SyncWaitTunnel::~SyncWaitTunnel()
	{
		DEBUG_DTOR(_logger);
	}


	bool SyncWaitTunnel::procedure()
	{
		DEBUG_ENTER(_logger);
		bool started = false;

		if (_tunnel.start()) {
			// Wait until the listener is in LISTENING state.
			started = _tunnel.wait_listening(7000);
		}

		// Return whether the tunneler is listening or has failed to start
		return started;
	}

	const char* SyncWaitTunnel::__class__ = "SyncWaitTunnel";
}
