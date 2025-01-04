/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include "tools/Logger.h"
#include "ui/AsyncMessage.h"


namespace ui {

	/**
	* A synchronous procedure executes the "procedure" defined in a subclass. At the
	* end of the execution, a message is posted to a recipient window.
	*/
	class SyncProc
	{
	public:
		explicit SyncProc(HWND hwnd, const AsyncMessage& message);
		virtual ~SyncProc();

		/* Runs the procedure and then send the message
		*/
		void run();

	protected:
		// - the logger
		tools::Logger* const _logger;

		// - the recipient window of the user even message
		const HWND _hwnd;

	private:
		// - event message sent asynchronously when procedure execution is finished
		const AsyncMessage _message;

		// - procedure to execute
		virtual bool procedure() = 0;
	};

}
