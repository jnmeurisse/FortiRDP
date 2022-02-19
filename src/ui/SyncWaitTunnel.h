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
#include "net/Tunneler.h"

/**
* A synchronous procedure that waits the listener to be active. The procedure
* sends TunnelListeningEvent when the listener is active.
*/
class SyncWaitTunnel : public SyncProc
{
public:
	explicit SyncWaitTunnel(HWND hwnd, net::Tunneler* tunneler);
	virtual ~SyncWaitTunnel();

private:
	// The tunneler
	net::Tunneler* const _tunneler;

	// The wait procedure
	virtual bool procedure() override;
};