/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Thread.h"

#include <process.h>
#include "tools/SysUtil.h"


namespace tools {

	/* Internal functions */
	unsigned __stdcall thread_entry_point(void *);

	Thread::Thread(bool auto_delete) : 
		_logger(Logger::get_logger()),
		_auto_delete(auto_delete)
	{
		DEBUG_CTOR(_logger);

		_handle = (HANDLE)::_beginthreadex(nullptr, 0, thread_entry_point, this, CREATE_SUSPENDED, &_id);
		_logger->debug("... %x created Thread handle=%x", (uintptr_t)this, _handle);

		if (_handle == NULL)
			throw_winapi_error(::GetLastError(), "_beginthreadex error");
	}


	Thread::~Thread()
	{
		DEBUG_DTOR(_logger);
		if (_handle != NULL)
		{
			_logger->debug("... %x destroyed Thread handle=%x", (uintptr_t)this, _handle);
			CloseHandle(_handle);
		}
	}


	bool Thread::start()
	{
		DEBUG_ENTER(_logger);
		DWORD status = ::ResumeThread(_handle);
		
		return status != (DWORD)-1;
	}


	bool Thread::wait(DWORD timeout)
	{
		DEBUG_ENTER(_logger);

		switch (::WaitForSingleObject(_handle, timeout)) {
		case WAIT_OBJECT_0: // the thread has ended.
			return true;

		case WAIT_TIMEOUT: // the thread is still running.
			return false;

		case WAIT_FAILED:
			throw_winapi_error(::GetLastError(), "Thread::wait");

		default:
			return false;
		}
	}


	/**
	* Thread entry point.
	*/
	static unsigned __stdcall thread_entry_point(void* data)
	{
		Thread* const thread = reinterpret_cast<Thread*>(data);
		unsigned int rc = thread->run();

		if (thread->_auto_delete) {
			delete thread;
		}
		
		::_endthreadex(rc);
		return 0;
	}

	const char* Thread::__class__ = "Thread";

}
