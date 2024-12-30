/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Path.h"

#include <wil/com.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <vector>
#include "tools/ErrUtil.h"


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
		size_t buffer_size = MAX_PATH;

		do {
			size_t written = 0;
			std::vector<wchar_t> buffer(buffer_size);

			int rc = ::GetModuleFileName(hModule, buffer.data(), static_cast<DWORD>(buffer.size()));
			if (rc == buffer.size() && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
				buffer_size += 1024;
			else if (rc == 0)
				throw tools::win_errmsg(::GetLastError());
			else
				return Path(buffer.data());
		} while (true);
	}

	
	Path Path::get_desktop_path()
	{
		wil::unique_cotaskmem_string buffer;

		// Get the path to the desktop folder for the current user.
		HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_Desktop, KF_FLAG_DEFAULT, NULL, &buffer);
		if (FAILED(hr))
			throw tools::win_errmsg(::GetLastError());

		// The returned path does not include a trailing backslash, add one.
		std::wstring path{ buffer.get() };
		path.append(L"\\");
		
		return Path(path, L"");
	}

}
