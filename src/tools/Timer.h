/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <cstdint>
#include "tools/Logger.h"

namespace tools {

	class Timer final
	{
	public:
		/* Constructs a timer.
		 * The timer must be started by calling start.
		*/
		Timer();

		/* Constructs and starts the timer for the specified duration (ms).
		*/
		explicit Timer(uint32_t duration);

		/* Destructor.
		*/
		~Timer();

		/* Restarts the timer for the specified duration (ms).
		*/
		void start(uint32_t duration) noexcept;

		/* Returns true if the timer has elapsed.
		*/
		bool is_elapsed() const noexcept;

		/* Returns the remaining time (ms) before the timer elapses.
		*/
		uint32_t remaining_time() const noexcept;

	private:
		// A reference to the application logger
		Logger* const _logger;
		
		// End time of the timer
		uint64_t _due_time;
	};

}
