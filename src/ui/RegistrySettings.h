/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include "tools/RegKey.h"
#include "ui/ScreenSize.h"


/**
* This class contains application settings. It is used by FortiRDP to save and
* retrieve default values.
*/
class RegistrySettings
{
public:
	explicit RegistrySettings();
	virtual ~RegistrySettings();

	/* retrieves the last user name successfully logged in the firewall
	*/
	std::wstring get_username(const std::wstring& default_value) const;

	/* saves the last user name successfully logged in the firewall
	*/
	void set_username(const std::wstring& username);

	/* retrieves last firewall address
	*/
	std::wstring get_firewall_address() const;

	/* saves the last firewall address
	*/
	void set_firewall_address(const std::wstring& value);

	/* retrieves the last host address
	*/
	std::wstring get_host_address() const;

	/* saves the last host address
	*/
	void set_host_address(const std::wstring& value);

	/* retrieves the full screen mode flag
	*/
	bool get_full_screen() const;

	/* saves the full scree mode flag
	*/
	void set_full_screen(bool fullscreen);

	/* retrieves the clear RDP user name flag
	*/
	bool get_clear_rdp_username() const;

	/* saves the clear RDP user name flag
	*/
	void set_clear_username(bool clear_username);

	/* retrieves the span mode flag 
	*/
	bool get_span_mode() const;

	/* saves the span mode flag
	*/
	void set_span_mode(bool span_mode);

	/* retrieves the multi monitors mode flag
	*/
	bool get_multimon_mode() const;

	/* saves the multi monitors mode flag
	*/
	void set_multimon_mode(bool multimon_mode);

	/* retrieves the rdp administrator console mode flag
	*/
	bool get_admin_console() const;

	/* saves the rdp administrator console mode flag
	*/
	void set_admin_console(bool admin_console);

	/* retrieves the rdp file mode flag
	*/
	bool get_rdpfile_mode() const;

	/* saves the rdp file mode flag
	*/
	void set_rdpfile_mode(bool rdpfile_mode);

	/* retrieves the rdp filename
	*/
	std::wstring get_rdp_filename() const;

	/* saves the rdp filename
	*/
	void set_rdp_filename(const std::wstring& rdp_filename);

	/* retrieves the screen size
	*/
	ScreenSize get_screen_size() const;

	/* saves the screen size
	*/
	void set_screen_size(const ScreenSize& size);

private:
	//- the registry root key
	tools::RegKey _key;

	//- a convenient method to retrieve a flag value
	bool get_bool(const std::wstring& value_name) const;

	//- a convenient method to save a flag value
	void set_bool(const std::wstring& value_name, const bool value);
	
	// - a convenient method to retrieve an int value
	int get_int(const std::wstring& value_name, const int default_value) const;

	//- a convenient method to save an int value
	void set_int(const std::wstring& value_name, const int value);

	//- registry keys
	static const std::wstring USERNAME_KEYNAME;
	static const std::wstring FIREWALL_KEYNAME;
	static const std::wstring HOST_KEYNAME;
	static const std::wstring FULLSCREEN_KEYNAME;
	static const std::wstring CLEARUSERNAME_KEYNAME;
	static const std::wstring SPANMODE_KEYNAME;
	static const std::wstring MULTIMON_KEYNAME;
	static const std::wstring ADMINCONSOLE_KEYNAME;
	static const std::wstring RDPFILEMODE_KEYNAME;
	static const std::wstring RDPFILENAME_KEYNAME;
	static const std::wstring SCREENSIZE_MODE;
	static const std::wstring SCREEN_WIDTH;
	static const std::wstring SCREEN_HEIGHT;
};