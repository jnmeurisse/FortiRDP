/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "RegKey.h"

#include <vector>
#include <system_error>
#include "tools/SysUtil.h"


namespace tools {

	RegKey::RegKey(HKEY root_key, const std::wstring& key_name) :
		_key(),
		_root_key(root_key),
		_key_name(key_name)
	{
		LSTATUS rc = ::RegCreateKeyEx(
			_root_key,
			_key_name.c_str(),
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&_key,
			NULL);

		
		if (rc != ERROR_SUCCESS)
			throw_winapi_error(rc, "RegCreateKey error");
	}


	RegKey::~RegKey()
	{
		::RegCloseKey(_key);
	}

	
	DWORD RegKey::get_word(const std::wstring& value_name) const
	{
		DWORD data = 0;
		DWORD size = sizeof(data);

		LSTATUS rc = ::RegGetValue(
			_root_key,
			_key_name.c_str(),
			value_name.c_str(),
			RRF_RT_DWORD,
			nullptr,
			&data,
			&size);

		if (rc != ERROR_SUCCESS)
			throw_winapi_error(rc, "RegGetValue error");

		return data;
	}


	DWORD RegKey::get_word(const std::wstring& value_name, DWORD default_value) const
	{
		try {
			return get_word(value_name);
		} catch (const std::system_error&) {
			return default_value;
		}
	}


	void RegKey::set_word(const std::wstring & value_name, DWORD value)
	{
		LSTATUS rc = ::RegSetValueEx(
			_key,
			value_name.c_str(),
			0,
			REG_DWORD,
			reinterpret_cast<const BYTE*>(&value),
			sizeof(DWORD));

		if (rc != ERROR_SUCCESS)
			throw_winapi_error(rc, "RegSetValue error");
	}


	std::wstring RegKey::get_string(const std::wstring& value_name) const
	{
		DWORD len = 0;

		// Get the size of the string
		LSTATUS rc = ::RegGetValue(
			_root_key,
			_key_name.c_str(),
			value_name.c_str(),
			RRF_RT_REG_SZ,
			nullptr,
			nullptr,
			&len);

		if (rc != ERROR_SUCCESS)
			throw_winapi_error(rc, "RegGetValue error");

		// Allocate the needed space.
		// Note: len is expressed in bytes and includes the space to store the null char 
		std::vector<wchar_t> buffer(len / sizeof(wchar_t));

		// Get the string into the allocated buffer
		rc = ::RegGetValue(
			_root_key,
			_key_name.c_str(),
			value_name.c_str(),
			RRF_RT_REG_SZ,
			nullptr,
			&buffer[0],
			&len);
		if (rc != ERROR_SUCCESS)
			throw_winapi_error(rc, "RegGetValue error");

		// Returns the number of wchar not including the null char
		return std::wstring(&buffer[0], (len / sizeof(wchar_t)) - 1);
	}


	std::wstring RegKey::get_string(const std::wstring & value_name, const std::wstring& default_value) const
	{
		try {
			return get_string(value_name);
		} catch (const std::system_error&) {
			return default_value;
		}
	}

	
	void RegKey::set_string(const std::wstring& value_name, const std::wstring& value)
	{
		size_t size = (value.length() + 1) * sizeof(wchar_t);
		LSTATUS rc = ::RegSetValueEx(
			_key,
			value_name.c_str(),
			0,
			REG_SZ,
			reinterpret_cast<const BYTE*>(value.c_str()),
			(DWORD) size);

		if (rc != ERROR_SUCCESS)
			throw_winapi_error(rc, "RegSetValueEx error");
	}


	void RegKey::del(const std::wstring& key_name)
	{
		LSTATUS rc = ::RegDeleteKey(
			_key,
			key_name.c_str());
		if (rc != ERROR_SUCCESS)
			throw_winapi_error(rc, "RegDeleteKey error");
	}


	void RegKey::del_value(const std::wstring& value_name)
	{
		LSTATUS rc = ::RegDeleteValue(
			_key,
			value_name.c_str());
		if (rc != ERROR_SUCCESS)
			throw_winapi_error(rc, "RegDeleteValue error");
	}

}
