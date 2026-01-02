/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include "SyncProc.h"
#include "tools/Task.h"


namespace ui {

	/**
	* A synchronous procedure that waits the end of a task (a task is running in a
	* separate Windows process). The procedure posts a DisconnectFromFirewallRequest
	* to the window when done.
	*/
	class SyncWaitTask final : public SyncProc
	{
	public:
		explicit SyncWaitTask(HWND hwnd, aux::Task* task);
		virtual ~SyncWaitTask() override;

	private:
		// The class name.
		static const char* __class__;

		// A task that we wait for to finish.
		aux::Task* const _task;

		// The wait procedure.
		virtual bool procedure() override;
	};

}
