/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Mutex.h"


namespace aux {

	Mutex::Mutex()
	{
		::InitializeCriticalSection(&_cs);
	}


	Mutex::~Mutex()
	{
		::DeleteCriticalSection(&_cs);
	}


	Mutex::Lock::Lock(Mutex &mutex) :
		_mutex(mutex)
	{
		::EnterCriticalSection(&_mutex._cs);
	}


	Mutex::Lock::~Lock()
	{
		::LeaveCriticalSection(&_mutex._cs);
	}

}
