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
#include <system_error>
#include "tools/StrUtil.h"


namespace tools {

	bool file_exists(const std::wstring& path)
	{
		DWORD dwAttrib = ::GetFileAttributes(path.c_str());

		return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
			!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
	}


	bool file_exists(const tools::Path& path)
	{
		return file_exists(path.to_string());
	}


	std::wstring get_windows_username()
	{
		wchar_t username[UNLEN + 1]{ 0 };
		DWORD username_len = UNLEN + 1;

		if (::GetUserName(username, &username_len)) {
			return username;
		}
		else {
			return L"";
		}
	}

	
	std::string get_file_ver(const std::wstring& path)
	{
		DWORD size = 0;
		BYTE* pVerInfo = nullptr;
		VS_FIXEDFILEINFO* pFileInfo = nullptr;
		UINT fileInfoLen = 0;
		
		size = ::GetFileVersionInfoSize(path.c_str(), nullptr);
		if (size == 0)
			goto terminate;

		pVerInfo = new BYTE[size];
		if (!::GetFileVersionInfo(path.c_str(), 0, size, pVerInfo))
			goto terminate;

		if (!::VerQueryValue(pVerInfo, L"\\", (LPVOID *)&pFileInfo, &fileInfoLen))
			goto terminate;

		return string_format("%d.%d.%d",
			HIWORD(pFileInfo->dwFileVersionMS),
			LOWORD(pFileInfo->dwFileVersionMS),
			HIWORD(pFileInfo->dwFileVersionLS));

	terminate:
		if (pVerInfo)
			delete [] pVerInfo;

		return "?";
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
