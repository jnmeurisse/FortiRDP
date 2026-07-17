/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "TaskInfo.h"


namespace utl {

	TaskInfo::TaskInfo(const std::wstring& path, const std::vector<std::wstring>& params) noexcept :
		_task_path(path),
		_task_params(params)
	{
	}

	
	TaskInfo::TaskInfo(const std::wstring & path) noexcept :
		_task_path(path),
		_task_params(std::vector<std::wstring>())
	{
	}

}
