/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Event.h"

#include "tools/SysUtil.h"


namespace tools {

	Event::Event() :
		Event(true)
	{
	}


	Event::Event(bool manual_reset) :
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger, "Event");

		_handle = ::CreateEvent(nullptr, manual_reset, false, nullptr);
		if (_handle == NULL)
			throw_winapi_error(::GetLastError(), "CreateEvent error");

		if (_logger->is_debug_enabled())
			_logger->debug("... %x create Event handle=%x", (uintptr_t)this, _handle);
	}


	Event::Event(const Event& event):
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger, "Event");

		BOOL rc = ::DuplicateHandle(::GetCurrentProcess(),
			event.get_handle(),
			::GetCurrentProcess(),
			&_handle,
			0,
			FALSE,
			DUPLICATE_SAME_ACCESS);

		if (!rc)
			throw_winapi_error(::GetLastError(), "DuplicateHandle error");

		if (_logger->is_debug_enabled())
			_logger->debug("... %x create Event handle=%x", (uintptr_t)this, _handle);
	}


	Event::~Event()
	{
		DEBUG_DTOR(_logger, "Event");

		if (_handle != NULL)
			::CloseHandle(_handle);
	}


	bool Event::set() noexcept
	{
		return ::SetEvent(_handle) != 0;
	}


	bool Event::reset() noexcept
	{
		return ::ResetEvent(_handle) != 0;
	}


	bool Event::is_set() const
	{
		return wait(0);
	}


	bool Event::wait(DWORD timeout) const
	{
		switch (::WaitForSingleObject(_handle, timeout)) {
		case WAIT_OBJECT_0:
			// the event is set
			return true;

		case WAIT_TIMEOUT:
			// the event is not yet signaled
			return false;

		case WAIT_FAILED:
			throw_winapi_error(::GetLastError(), "Event::wait");

		default:
			return false;
		}
	}

}