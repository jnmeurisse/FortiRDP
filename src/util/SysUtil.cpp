/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "SysUtil.h"

#include <lmcons.h>
#include <string>
#include <vector>
#include <system_error>
#include "util/StrUtil.h"


namespace utl {

	bool file_exists(const std::wstring& path)
	{
		DWORD dwAttrib = ::GetFileAttributes(path.c_str());

		return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
			!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
	}


	bool file_exists(const utl::Path& path)
	{
		return file_exists(path.to_string());
	}


	std::wstring get_windows_username()
	{
		DWORD buffer_size = UNLEN + 1;

		do {
			std::vector<wchar_t> buffer(buffer_size);

			if (!::GetUserName(buffer.data(), &buffer_size)) {
				if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
					buffer_size += 64;
				else
					return L"";
			}
			else {
				return std::wstring(buffer.data(), buffer_size);
			}
		} while (true);
	}

	
	std::string get_file_ver(const std::wstring& path)
	{
		std::string file_version = "?";
		
		const DWORD size = ::GetFileVersionInfoSize(path.c_str(), nullptr);
		if (size > 0) {
			std::vector<BYTE> version_info(size);
			if (!::GetFileVersionInfo(path.c_str(), 0, size, version_info.data()))
				goto terminate;

			UINT file_info_len = 0;
			LPVOID file_info_ptr;
			if (!::VerQueryValue(version_info.data(), L"\\", &file_info_ptr, &file_info_len))
				goto terminate;

			const VS_FIXEDFILEINFO* pFileInfo = static_cast<VS_FIXEDFILEINFO*>(file_info_ptr);
			file_version = string_format("%d.%d.%d",
				HIWORD(pFileInfo->dwFileVersionMS),
				LOWORD(pFileInfo->dwFileVersionMS),
				HIWORD(pFileInfo->dwFileVersionLS));
		}

	terminate:
		return file_version;
	}

	
	std::string get_plaform()
	{
#ifdef _WIN64
		return "64 bits";
#else
		return "32 bits"
#endif
	}


	void throw_winapi_error(DWORD error_code, const std::string& func_name)
	{
		throw std::system_error{ (int) error_code, std::system_category(), func_name };
	}

}
