/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include <string>


namespace tools {

	/**
	 * A RegKey object represents a registry key in the windows registry
	*/
	class RegKey final
	{
	public:
		explicit RegKey(HKEY root_key, const std::wstring& key_name);
		~RegKey();

		/* Returns the value as a dword. The method raises a system_error exception
		 * if the key does not exist or is not a dword.
		*/
		DWORD get_word(const std::wstring& value_name) const;

		/* Returns the value as a dword. If the key name does not exist, the function
		 * returns the default value. The method raises a system_error exception
		* if the key is not a dword
		*/
		DWORD get_word(const std::wstring& value_name, DWORD default_value) const;

		void set_word(const std::wstring& value_name, DWORD value);

		/* Returns the value as a string. The method raises a system_error exception
		* if the key does not exist or is not a string.
		*/
		std::wstring get_string(const std::wstring& value_name) const;

		/* Returns the value as a string. If the key name does not exist, the function
		* returns the default value. The method raises a system_error exception
		* if the key is not a string
		*/
		std::wstring get_string(const std::wstring& value_name, const std::wstring& default_value) const;

		void set_string(const std::wstring& value_name, const std::wstring& value);

		/* Deletes a named key from this registry key
		*/
		void del(const std::wstring& key_name);

		/* Deletes a named value from this registry key
		*/
		void del_value(const std::wstring& value_name);

	private:
		const HKEY _root_key;
		const std::wstring _key_name;
		HKEY _key;
	};

}
