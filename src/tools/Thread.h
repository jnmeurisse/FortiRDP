/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once
#include <crtdefs.h>
#include <Windows.h>

#include "tools/Logger.h"

namespace tools {

	/**
	 * A Thread is an abstract class that enables creation of separate threads 
	 * of execution. Each new instance of a Thread descendant is a new 
	 * thread of execution.
	*/
	class Thread
	{
	public:
		/* Allocates a new execution thread. The thread is created in suspended state.
		 * The caller must call start to resume the execution.
		*/
		explicit Thread(bool auto_delete=false);

		/* Deletes this thread.
		*/
		virtual ~Thread();

		/* Starts the execution of the thread. The method returns true
		 * if the function succeed.
		*/
		bool start();

		/* Waits 'timeout' milliseconds for the thread to finish.
		 * The function returns true if the thread has finished and false if
		 * the thread is still running after the specified time.
		*/
		bool wait(DWORD timeout);

		/* Returns the thread identifier
		*/
		inline unsigned int get_id() const { return _id; }

		/* Returns the thread handle
		*/
		inline HANDLE get_handle() const { return _handle; }

		/* Specifies whether this object is deleted when thread terminates
		*/
		inline bool get_auto_delete() const { return _auto_delete; }

	protected:
		virtual unsigned int run() = 0;

	private:
		// A reference to the application logger
		Logger* const _logger;

		// Handle to the windows thread
		HANDLE _handle;

		// Thread id
		unsigned int _id;

		// Should we delete this object when the thread stops
		const bool _auto_delete;

		friend unsigned __stdcall thread_entry_point(void *data);
	};

}