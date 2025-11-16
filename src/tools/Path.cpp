/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Path.h"

#include <wil/com.h>
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

	
	std::wstring Path::compact(unsigned int max_char) const
	{
		std::vector<wchar_t> buffer(max_char + 1);
		if (max_char > 0 && ::PathCompactPathEx(buffer.data(), to_string().c_str(), max_char, 0))
			return buffer.data();
		else
			return to_string();
	}

	
	Path Path::get_module_path(HMODULE hModule)
	{
		size_t buffer_size = MAX_PATH;

		do {
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
		return get_known_folder_path(FOLDERID_Desktop);
	}


	Path Path::get_appdata_path()
	{
		return get_known_folder_path(FOLDERID_LocalAppData);
	}


	Path Path::get_known_folder_path(REFKNOWNFOLDERID rfid)
	{
		wil::unique_cotaskmem_string buffer;

		// Get the path to the folder for the current user.
		const HRESULT hr = ::SHGetKnownFolderPath(rfid, KF_FLAG_DEFAULT, NULL, &buffer);
		if (FAILED(hr))
			throw tools::win_errmsg(::GetLastError());

		// The returned path does not include a trailing backslash, add one.
		std::wstring path{ buffer.get() };
		path.append(L"\\");

		return Path(path, L"");
	}

}
