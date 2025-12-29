/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Task.h"

#include <vector>
#include "tools/StrUtil.h"


namespace tools {

	Task::Task(const std::wstring& path) :
		_cmdline(tools::quote(path)),
		_logger(Logger::get_logger())
	{
		memset(&_pi, 0, sizeof(_pi));
		_pi.hProcess = INVALID_HANDLE_VALUE;
		_pi.hThread = INVALID_HANDLE_VALUE;
	}


	Task::~Task()
	{
		if (_pi.hThread != INVALID_HANDLE_VALUE)
			CloseHandle(_pi.hThread);

		if (_pi.hProcess != INVALID_HANDLE_VALUE)
			CloseHandle(_pi.hProcess);
	}


	void Task::add_parameter(const std::wstring& parameter)
	{
		_cmdline
			.append(L" ")
			.append(tools::quote(parameter));
	}


	bool Task::start()
	{
		bool rc = false;

		if (_pi.hProcess == INVALID_HANDLE_VALUE) {
			STARTUPINFO si = { 0 };
			si.cb = sizeof(si);

			_logger->debug(">> start task cmd=%s", tools::wstr2str(_cmdline).c_str());

			std::vector<wchar_t> cmdline_buffer(_cmdline.begin(), _cmdline.end());
			cmdline_buffer.push_back(L'\0');

			if (!::CreateProcess(
				nullptr,
				cmdline_buffer.data(),
				nullptr,
				nullptr,
				FALSE,
				0,
				nullptr,
				nullptr,
				&si,
				&_pi)) {

				_logger->error("ERROR: unable to create process (error=%x)", GetLastError());
				rc = false;
			}
			else {
				_logger->debug("... task pid=%d started", _pi.dwProcessId);
				rc = true;
			}
		}

		return rc;
	}


	bool Task::wait(unsigned long millis)
	{
		switch (::WaitForSingleObject(_pi.hProcess, millis)) {
		case WAIT_OBJECT_0:
			_logger->debug("... task pid=%d is stopped", _pi.dwProcessId);
			return true;

		case WAIT_TIMEOUT: // the thread is still running
			return false;

		default:
			_logger->error("ERROR: error waiting for end of task pid=%d (code:%x)", _pi.dwProcessId, GetLastError());
			return false;
		}
	}

}
