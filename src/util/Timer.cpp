/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <Windows.h>
#include "Timer.h"

namespace utl {

	Timer::Timer() :
		Timer(0)
	{
	}


	Timer::Timer(uint32_t duration) :
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger);

		start(duration);
	}


	Timer::~Timer()
	{
		DEBUG_DTOR(_logger);
	}


	void Timer::start(uint32_t duration) noexcept
	{
		DEBUG_ENTER_FMT(_logger, "duration=%lu", duration);

		_due_time = ::GetTickCount64() + (uint64_t) duration;
	}


	bool Timer::is_elapsed() const noexcept
	{
		return ::GetTickCount64() > _due_time;
	}


	uint32_t Timer::remaining_time() const noexcept
	{
		const uint64_t now = ::GetTickCount64();
		return now >= _due_time ? 0 : static_cast<uint32_t>(_due_time - now);
	}


	const char* Timer::__class__ = "Timer";
}
