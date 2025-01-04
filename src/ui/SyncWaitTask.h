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
		explicit SyncWaitTask(HWND hwnd, tools::Task* task);
		~SyncWaitTask();

	private:
		//- the task to wait to complete
		tools::Task* const _task;

		//- the wait procedure
		virtual bool procedure() override;
	};

}
