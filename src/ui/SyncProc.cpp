/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SyncProc.h"


namespace ui {

	SyncProc::SyncProc(HWND hwnd, const AsyncMessage* message) :
		_logger(aux::Logger::get_logger()),
		_hwnd(hwnd),
		_message(message)
	{
		DEBUG_CTOR(_logger);

	}


	SyncProc::~SyncProc()
	{
		DEBUG_DTOR(_logger);
	}


	void SyncProc::run()
	{
		DEBUG_ENTER(_logger);

		bool success = procedure();
		_message->send_message(_hwnd, reinterpret_cast<void *>(success));
	}


	const char* SyncProc::__class__ = "SyncProc";
}
