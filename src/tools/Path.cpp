/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Path.h"

#include <ShlObj.h>
#include <Shlwapi.h>
#include <vector>


namespace tools {

	Path::Path(const std::wstring& path)
	{
		const size_t last_delim = path.find_last_of('\\');

		if (last_delim == std::wstring::npos) {
			_fname = path;
		}
		else {
			_folder = path.substr(0, last_delim + 1);
			_fname = path.substr(last_delim + 1);
		}
	}


	Path::Path(const std::wstring& folder, const std::wstring& filename):
		_folder(folder),
		_fname(filename)
	{
	}

	
	std::wstring Path::to_string() const
	{
		return _folder + _fname;
	}

	
	std::wstring Path::compact(int max_char) const
	{
		std::vector<wchar_t> buffer(max_char + 1);
		if (::PathCompactPathEx(&buffer[0], to_string().c_str(), max_char, 0))
			return std::wstring(&buffer[0]);
		else
			return to_string();
	}

	
	Path Path::get_module_path(HMODULE hModule)
	{
		wchar_t filename[4096]{ 0 };
		::GetModuleFileName(hModule, filename, sizeof(filename) - 1);

		return Path(filename);
	}

	
	Path Path::get_desktop_path()
	{
		wchar_t path[_MAX_PATH + 1]{ 0 };
		::SHGetSpecialFolderPath(NULL, &path[0], CSIDL_DESKTOP, FALSE);
		wcscat_s(path, L"\\");
		
		return Path(path, L"");
	}
}