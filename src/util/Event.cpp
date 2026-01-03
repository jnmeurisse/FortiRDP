/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Event.h"

#include "util/SysUtil.h"


namespace aux {

	Event::Event() :
		Event(true)
	{
	}


	Event::Event(bool manual_reset) :
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger);

		_handle = ::CreateEvent(nullptr, manual_reset, false, nullptr);
		LOG_DEBUG(_logger, "handle=%x", _handle);

		if (_handle == NULL)
			throw_winapi_error(::GetLastError(), "CreateEvent error");
	}


	Event::Event(const Event& event):
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger);

		BOOL rc = ::DuplicateHandle(::GetCurrentProcess(),
			event.get_handle(),
			::GetCurrentProcess(),
			&_handle,
			0,
			FALSE,
			DUPLICATE_SAME_ACCESS);
		LOG_DEBUG(_logger, "rc=%d, handle=%x", rc, _handle);

		if (!rc)
			throw_winapi_error(::GetLastError(), "DuplicateHandle error");

	}


	Event::~Event()
	{
		DEBUG_DTOR(_logger);

		if (_handle != NULL) {
			LOG_DEBUG(_logger, "handle=%x", _handle);
			::CloseHandle(_handle);
		}
	}


	bool Event::set() noexcept
	{
		DEBUG_ENTER_FMT(_logger, "handle=%x", _handle);
		return ::SetEvent(_handle) != 0;
	}


	bool Event::reset() noexcept
	{
		DEBUG_ENTER_FMT(_logger, "handle=%x", _handle);
		return ::ResetEvent(_handle) != 0;
	}


	bool Event::is_set() const
	{
		return wait(0);
	}


	bool Event::wait(DWORD timeout) const
	{
		DEBUG_ENTER_FMT(_logger, "handle=%x", _handle);

		switch (::WaitForSingleObject(_handle, timeout)) {
		case WAIT_OBJECT_0: // The event is set.
			return true;

		case WAIT_TIMEOUT: // The event is not yet signaled.
			return false;

		case WAIT_FAILED:
			throw_winapi_error(::GetLastError(), "Event::wait");

		default:
			return false;
		}
	}

	const char* Event::__class__ = "Event";

}
