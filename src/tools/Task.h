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

		/**
		 * Adds a parameter to the task.
		 */
		void add_parameter(const std::wstring& parameter);
	
		
		/**
		 * Starts task.
		 * 
		 * An external process is created and started.
		 * 
		 * @return false if the process can not be stated.
		 */
		bool start();

		/**
		 * Waits 'millis' milliseconds for the task to finish.
		 *
		 * The function returns true if the task has finished and false if
		 * the thread is still running after the specified time.
		*/
		bool wait(unsigned long millis);

		/**
		 * Returns the task handle.
		 * 
		 * Note: the function returns an invalid handle if the task is not
		 * yet started or if the start method failed.
		 * 
		*/
		HANDLE get_handle() const { return _pi.hProcess; }

	private:
		// The class name.
		static const char* __class__;

		// A reference to the application logger.
		Logger* const _logger;

		std::wstring _cmdline;
		PROCESS_INFORMATION _pi;
	};

}
