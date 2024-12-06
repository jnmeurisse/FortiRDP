/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Timer.h"

#include "tools/SysUtil.h"


namespace tools {

	Timer::Timer(int ms) :
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger, "Timer");

		_handle = ::CreateWaitableTimer(NULL, TRUE, NULL);
		if (_handle == NULL)
			throw_winapi_error(::GetLastError(), "CreateWaitableTimer error");

		if (_logger->is_debug_enabled())
			_logger->debug( "... %x create Timer handle=%x", (uintptr_t)this, _handle);

		start(ms);
	}


	Timer::Timer(const Timer& timer) : 
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger, "Timer");

		if (!::DuplicateHandle(GetCurrentProcess(),
			timer._handle,
			GetCurrentProcess(),
			&_handle,
			0,
			FALSE,
			DUPLICATE_SAME_ACCESS))
			throw_winapi_error(::GetLastError(), "Timer DuplicateHandle error");

		if (_logger->is_debug_enabled())
			_logger->debug("... %x created Timer handle=%x", (uintptr_t)this, _handle);
	}


	Timer::~Timer()
	{
		DEBUG_DTOR(_logger, "Timer");

		if (_handle != NULL) {
			_logger->debug("... %x destroyed Timer handle=%x", (uintptr_t)this, _handle);
			::CloseHandle(_handle);
		}
	}


	void Timer::start(int ms)
	{
		DEBUG_ENTER(_logger, "Timer", "start");

		LARGE_INTEGER due_time;
		due_time.QuadPart = ms * -10000;
		BOOL rc = ::SetWaitableTimer(_handle, &due_time, 0, nullptr, nullptr, false);
		if (_logger->is_debug_enabled())
			_logger->debug("... %x Timer::start ms=%d rc=%d", (uintptr_t)this, ms, rc);

		if (!rc)
			throw_winapi_error(::GetLastError(), "SetWaitableTimer error");
	}


	bool Timer::elapsed() const
	{
		bool done = ::WaitForSingleObject(_handle, 0) == WAIT_OBJECT_0;
		if (done) {
			_logger->debug("... %x Timer::elapsed", (uintptr_t)this);
		}

		return done;
	}

}
