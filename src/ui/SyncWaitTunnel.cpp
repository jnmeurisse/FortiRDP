/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SyncWaitTunnel.h"


SyncWaitTunnel::SyncWaitTunnel(HWND hwnd, net::Tunneler* tunneler) :
	SyncProc(hwnd, AsyncMessage::TunnelListeningEvent),
	_tunneler(tunneler)
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

	if (_tunneler) {
		// Launch the listener thread 
		if (_tunneler->start()) {
			// Wait until the listener is in LISTENING state.
			started = _tunneler->wait_listening(7000);
		}
	}

	// Return that tunneler is listening or has failed to start
	return started;
}
