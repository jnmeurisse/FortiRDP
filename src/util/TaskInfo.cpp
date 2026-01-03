/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "TaskInfo.h"


namespace aux {

	TaskInfo::TaskInfo(const std::wstring& path, const std::vector<std::wstring>& params) :
		_task_path(path),
		_task_params(params)
	{
	}

	
	TaskInfo::TaskInfo(const std::wstring & path) :
		_task_path(path),
		_task_params(std::vector<std::wstring>())
	{
	}


	TaskInfo::~TaskInfo()
	{
	}

}
