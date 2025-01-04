/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>


namespace tools {

	/**
	* A mutex provides a synchronization mechanism.
	*/
	class Mutex final
	{
	public:
		Mutex();
		~Mutex();

		/* Forbid mutex copy
		*/
		Mutex(const Mutex& mutex) = delete;
		Mutex& operator=(const Mutex& mutex) = delete;

		/**
		 * A Lock object provides a scoped lock on the mutex. The lock is
		 * automatically released when the Lock object is discarded from the stack.
		*/
		class Lock
		{
		public:
			explicit Lock(Mutex &mutex);
			~Lock();

		private:
			Mutex &_mutex;
		};

	private:
		CRITICAL_SECTION _cs;
	};

}
