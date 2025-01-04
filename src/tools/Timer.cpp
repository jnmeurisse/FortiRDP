/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <iostream>

#include "tools/Timer.h"
#include "tools/SysUtil.h"

namespace tools {

	Timer::Timer(uint32_t delay) :
		_logger(Logger::get_logger())
	{
		DEBUG_CTOR(_logger, "Timer");

		start(delay);
	}


	Timer::~Timer()
	{
		DEBUG_DTOR(_logger, "Timer");
	}


	void Timer::start(uint32_t delay) noexcept
	{
		DEBUG_ENTER(_logger, "Timer", "start");

		if (_logger->is_debug_enabled())
			_logger->debug(
				"... %x Timer::start delay=%d",
				std::addressof(this),
				delay);

		_due_time = GetTickCount64() + delay;
		_elapsed = false;
	}


	bool Timer::is_elapsed() noexcept
	{
		if (!_elapsed && GetTickCount64() > _due_time)
			_elapsed = true;

		return _elapsed;
	}


	uint32_t Timer::remaining_delay() const noexcept
	{
		uint64_t remaining_time = max(0, _due_time - GetTickCount64());
		return static_cast<uint32_t>(remaining_time);
	}
}