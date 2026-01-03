/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>


namespace utl {

	// Returns true if a file exists
	bool file_exists(const std::wstring& path);

	// Returns the name of the user associated with this application
	std::wstring get_windows_username();

	// Returns the version of an .exe or .dll
	std::string get_file_ver(const std::wstring& path);

	// Returns platform cpu
	std::string get_plaform();
}
