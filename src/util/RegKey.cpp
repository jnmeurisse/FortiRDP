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
#include "util/ErrUtil.h"


namespace utl {

	RegKey::RegKey(HKEY root_key, const std::wstring& key_name) :
		_root_key(root_key),
		_key_name(key_name),
		_key()
	{
		LSTATUS rc = ::RegCreateKeyEx(
			_root_key,
			_key_name.c_str(),
			0,
			nullptr,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			nullptr,
			&_key,
			nullptr);

		
		if (rc != ERROR_SUCCESS)
			throw win_error(rc, "RegCreateKey");
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
			throw win_error(rc, "RegGetValue");

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
			throw win_error(rc, "RegSetValue");
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
			throw win_error(rc, "RegGetValue");

		// Allocate the required space.
		// Note: 'len' is in bytes and includes space for the null terminator.
		std::vector<wchar_t> buffer(len / sizeof(wchar_t));

		// Get the string into the allocated buffer.
		rc = ::RegGetValue(
			_root_key,
			_key_name.c_str(),
			value_name.c_str(),
			RRF_RT_REG_SZ,
			nullptr,
			buffer.data(),
			&len);
		if (rc != ERROR_SUCCESS)
			throw win_error(rc, "RegGetValue");

		// Returns the string not including the null char
		return buffer.data();
	}


	std::wstring RegKey::get_string(const std::wstring& value_name, const std::wstring& default_value) const
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
			throw win_error(rc, "RegSetValueEx");
	}


	void RegKey::del(const std::wstring& key_name)
	{
		LSTATUS rc = ::RegDeleteKey(
			_key,
			key_name.c_str());
		if (rc != ERROR_SUCCESS)
			throw win_error(rc, "RegDeleteKey");
	}


	void RegKey::del_value(const std::wstring& value_name)
	{
		LSTATUS rc = ::RegDeleteValue(
			_key,
			value_name.c_str());
		if (rc != ERROR_SUCCESS)
			throw win_error(rc, "RegDeleteValue");
	}

}
