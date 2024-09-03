/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <Windows.h>

#include "tools/Logger.h"

namespace tools {
	class Timer final
	{
	public:
		/* Constructs and starts the timer for the specified number of milli-seconds.
		*/
		explicit Timer(int ms);

		/* Copy constructor.
		*/
		explicit Timer(const Timer& timer);

		/* Destructor.
		*/
		~Timer();

		/* Starts the timer for the specified number of milli-seconds
		*/
		void start(int ms);

		/* Returns true if the timer has elapsed
		*/
		bool elapsed() const;

	private:
		// A reference to the application logger
		Logger* const _logger;
		
		// Handle to the windows timer
		HANDLE _handle;
	};

}