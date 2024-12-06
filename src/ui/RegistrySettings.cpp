/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "RegistrySettings.h"

#include <Windows.h>

namespace ui {
	RegistrySettings::RegistrySettings() :
		_key(HKEY_CURRENT_USER, L"Software\\Fortigate\\fortirdp")
	{
	}

	RegistrySettings::~RegistrySettings()
	{
	}

	std::wstring RegistrySettings::get_username(const std::wstring& default_value) const
	{
		return _key.get_string(USERNAME_KEYNAME, default_value);
	}

	void RegistrySettings::set_username(const std::wstring& username)
	{
		_key.set_string(USERNAME_KEYNAME, username);
	}

	std::wstring RegistrySettings::get_firewall_address() const
	{
		return _key.get_string(FIREWALL_KEYNAME, L"");
	}

	void RegistrySettings::set_firewall_address(const std::wstring& value)
	{
		_key.set_string(FIREWALL_KEYNAME, value);
	}

	std::wstring RegistrySettings::get_host_address() const
	{
		return _key.get_string(HOST_KEYNAME, L"");
	}

	void RegistrySettings::set_host_address(const std::wstring& value)
	{
		_key.set_string(HOST_KEYNAME, value);
	}

	bool RegistrySettings::get_full_screen() const
	{
		return get_bool(FULLSCREEN_KEYNAME);
	}

	void RegistrySettings::set_full_screen(bool fullscreen)
	{
		set_bool(FULLSCREEN_KEYNAME, fullscreen);
	}

	bool RegistrySettings::get_clear_rdp_username() const
	{
		return get_bool(CLEARUSERNAME_KEYNAME);
	}

	void RegistrySettings::set_clear_username(bool clear_username)
	{
		set_bool(CLEARUSERNAME_KEYNAME, clear_username);
	}

	bool RegistrySettings::get_span_mode() const
	{
		return get_bool(SPANMODE_KEYNAME);
	}

	void RegistrySettings::set_span_mode(bool span_mode)
	{
		set_bool(SPANMODE_KEYNAME, span_mode);
	}

	bool RegistrySettings::get_multimon_mode() const
	{
		return get_bool(MULTIMON_KEYNAME);
	}

	void RegistrySettings::set_multimon_mode(bool multimon_mode)
	{
		set_bool(MULTIMON_KEYNAME, multimon_mode);
	}

	bool RegistrySettings::get_admin_console() const
	{
		return get_bool(ADMINCONSOLE_KEYNAME);
	}

	void RegistrySettings::set_admin_console(bool admin_console)
	{
		set_bool(ADMINCONSOLE_KEYNAME, admin_console);
	}

	bool RegistrySettings::get_rdpfile_mode() const
	{
		return get_bool(RDPFILEMODE_KEYNAME);
	}

	void RegistrySettings::set_rdpfile_mode(bool rdpfile_mode)
	{
		set_bool(RDPFILEMODE_KEYNAME, rdpfile_mode);
	}

	std::wstring RegistrySettings::get_rdp_filename() const
	{
		return _key.get_string(RDPFILENAME_KEYNAME, L"");
	}

	void RegistrySettings::set_rdp_filename(const std::wstring& rdp_filename)
	{
		_key.set_string(RDPFILENAME_KEYNAME, rdp_filename);
	}

	ScreenSize RegistrySettings::get_screen_size() const
	{
		ScreenSize size{
			min(ScreenSize::max_height, max(0, get_int(SCREEN_HEIGHT, 0))),
			min(ScreenSize::max_width, max(0, get_int(SCREEN_WIDTH, 0)))
		};

		return size;
	}

	void RegistrySettings::set_screen_size(const ScreenSize& size)
	{
		set_int(SCREEN_HEIGHT, size.height);
		set_int(SCREEN_WIDTH, size.width);
	}

	bool RegistrySettings::get_bool(const std::wstring& value_name) const
	{
		return _key.get_word(value_name, 0) != 0;
	}

	void RegistrySettings::set_bool(const std::wstring& value_name, const bool value)
	{
		_key.set_word(value_name, value ? 1 : 0);
	}

	int RegistrySettings::get_int(const std::wstring& value_name, const int default_value) const
	{
		return _key.get_word(value_name, default_value);
	}

	void RegistrySettings::set_int(const std::wstring& value_name, const int value)
	{
		_key.set_word(value_name, value);
	}


	const std::wstring RegistrySettings::USERNAME_KEYNAME(L"username");
	const std::wstring RegistrySettings::FIREWALL_KEYNAME(L"firewall");
	const std::wstring RegistrySettings::HOST_KEYNAME(L"host");
	const std::wstring RegistrySettings::FULLSCREEN_KEYNAME(L"fullscreen");
	const std::wstring RegistrySettings::CLEARUSERNAME_KEYNAME(L"clearuname");
	const std::wstring RegistrySettings::SPANMODE_KEYNAME(L"span");
	const std::wstring RegistrySettings::MULTIMON_KEYNAME(L"multimon");
	const std::wstring RegistrySettings::ADMINCONSOLE_KEYNAME(L"console");
	const std::wstring RegistrySettings::RDPFILEMODE_KEYNAME(L"rdpfile");
	const std::wstring RegistrySettings::RDPFILENAME_KEYNAME(L"rdpfilename");
	const std::wstring RegistrySettings::SCREENSIZE_MODE(L"screensize");
	const std::wstring RegistrySettings::SCREEN_WIDTH(L"width");
	const std::wstring RegistrySettings::SCREEN_HEIGHT(L"height");

}
