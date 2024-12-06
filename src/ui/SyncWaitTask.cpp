/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SyncWaitTask.h"

#include "ui/AsyncMessage.h"


SyncWaitTask::SyncWaitTask(HWND hwnd, tools::Task* task) :
	SyncProc(hwnd, AsyncMessage::DisconnectFromFirewallRequest),
	_task(task)
{
	DEBUG_CTOR(_logger, "SyncWaitTask");
}


SyncWaitTask::~SyncWaitTask()
{
	DEBUG_DTOR(_logger, "SyncWaitTask");
}


bool SyncWaitTask::procedure()
{
	DEBUG_ENTER(_logger, "SyncWaitTask", "procedure");

	if (_task)
		_task->wait(INFINITE);

	return true;
}
