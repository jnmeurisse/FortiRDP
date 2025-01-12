/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <Windows.h>
#include "Timer.h"

namespace tools {

	Timer::Timer(uint32_t duration) :
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger, "Timer");

		start(duration);
	}


	Timer::~Timer()
	{
		DEBUG_DTOR(_logger, "Timer");
	}


	void Timer::start(uint32_t durtion) noexcept
	{
		DEBUG_ENTER(_logger, "Timer", "start");

		if (_logger->is_debug_enabled())
			_logger->debug(
				"... %x Timer::start duration=%d",
				(uintptr_t)this,
				durtion);

		_due_time = ::GetTickCount64() + durtion;
	}


	bool Timer::is_elapsed() const noexcept
	{
		return ::GetTickCount64() > _due_time;
	}


	uint32_t Timer::remaining_delay() const noexcept
	{
		static ULONGLONG now = ::GetTickCount64();
		return now >= _due_time ? 0 : static_cast<uint32_t>(_due_time - now);
	}

}
