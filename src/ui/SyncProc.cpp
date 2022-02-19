/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SyncProc.h"

SyncProc::SyncProc(HWND hwnd, const AsyncMessage& message):
	_message(message),
	_hwnd(hwnd),
	_logger(tools::Logger::get_logger())
{
	DEBUG_CTOR(_logger, "SyncProc");

}


SyncProc::~SyncProc()
{
	DEBUG_DTOR(_logger, "SyncProc");
}


void SyncProc::run()
{
	DEBUG_ENTER(_logger, "SyncProc", "run");

	bool success = procedure();
	_message.post(_hwnd, (void *)success);
}


