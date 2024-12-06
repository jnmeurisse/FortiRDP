/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include <vector>


namespace tools {
	class TaskInfo final
	{
	public:
		explicit TaskInfo(const std::wstring& path, const std::vector<std::wstring>& params);
		explicit TaskInfo(const std::wstring& path);
		~TaskInfo();

		/* Returns a reference to the task path
		*/
		inline const std::wstring& path() const { return _task_path; }

		/* Returns a reference to the task parameters
		*/
		inline const std::vector<std::wstring>& params() const { return _task_params; }

	private:
		const std::wstring _task_path;
		const std::vector<std::wstring> _task_params;
	};

}