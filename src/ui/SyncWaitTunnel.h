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
#include "fw/FirewallTunnel.h"

namespace ui {

	/**
	* A synchronous procedure that waits the listener to be active. The procedure
	* sends TunnelListeningEvent when the listener is active.
	*/
	class SyncWaitTunnel final : public SyncProc
	{
	public:
		explicit SyncWaitTunnel(HWND hwnd, fw::FirewallTunnel& tunnel);
		virtual ~SyncWaitTunnel() override;

	private:
		// The class name.
		static const char* __class__;

		// A tunnel that we wait for listening.
		fw::FirewallTunnel& _tunnel;

		// The wait procedure
		virtual bool procedure() override;
	};

}
