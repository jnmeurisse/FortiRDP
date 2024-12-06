/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include <string>
#include "tools/Logger.h"


namespace tools {

	class Task final
	{
	public:
		explicit Task(const std::wstring& path);
		~Task();

		void add_parameter(const std::wstring& parameter);
		bool start();
		bool wait(unsigned long millis);

		/* Returns the task handle
		*/
		HANDLE get_handle() const { return _pi.hProcess; }

	private:
		// A reference to the application logger
		Logger* const _logger;

		std::wstring _cmdline;
		PROCESS_INFORMATION _pi;
	};

}