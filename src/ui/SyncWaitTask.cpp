/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SyncWaitTask.h"

#include "ui/AsyncMessage.h"

namespace ui {

	SyncWaitTask::SyncWaitTask(HWND hwnd, tools::Task* task) :
		SyncProc(hwnd, AsyncMessage::DisconnectFromFirewallRequest.get()),
		_task(task)
	{
		DEBUG_CTOR(_logger);
	}


	SyncWaitTask::~SyncWaitTask()
	{
		DEBUG_DTOR(_logger);
	}


	bool SyncWaitTask::procedure()
	{
		DEBUG_ENTER(_logger);

		if (_task)
			_task->wait(INFINITE);

		return true;
	}

	const char* SyncWaitTask::__class__ = "SyncWaitTask";
}
