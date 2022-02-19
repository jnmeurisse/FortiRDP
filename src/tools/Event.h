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

	/**
	* An event is a synchronization object that enables one thread to notify another
	* thread that an event has occurred. A event stays in the state set by set() or
	* reset() until the other function is called.
	*/
	class Event
	{
	public:
		explicit Event(bool manual_reset=true);
		explicit Event(const Event& event);
		~Event();

		/* Sets the event to a non-signaled state. The method
		 * returns false if the function failed.
		*/
		bool reset() noexcept;

		/* Sets the event to a signaled state. The method
		 * returns false if the function failed. 
		*/
		bool set() noexcept;

		/* Returns true if this event is in a signaled state, false
		 * if not signaled or if the function failed.
		*/
		bool is_set() const noexcept;

		/* Waits until the event is in a signaled state. The method
		 * returns false if the function failed.
		*/
		bool wait(DWORD timeout = INFINITE) const;

		/* Returns the event handle
		*/
		inline HANDLE get_handle() const noexcept{ return _handle; }

	private:
		// A reference to the application logger
		Logger* const _logger;

		// The event handle
		HANDLE _handle = INVALID_HANDLE_VALUE;
	};

}