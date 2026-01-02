/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include "fw/AuthTypes.h"
#include "tools/RegKey.h"
#include "ui/ScreenSize.h"

namespace ui {

	/**
	* This class contains application settings. It is used by FortiRDP to save and
	* retrieve default values.
	*/
	class RegistrySettings
	{
	public:
		explicit RegistrySettings();
		virtual ~RegistrySettings();

		/**
		 * Retrieves the last user name successfully logged in the firewall.
		*/
		std::wstring get_username(const std::wstring& default_value) const;

		/**
		 * Saves the last user name successfully logged in the firewall.
		*/
		void set_username(const std::wstring& username);

		/** 
		 * Retrieves last firewall address.
		*/
		std::wstring get_firewall_address() const;

		/**
		 * Saves the last firewall address.
		*/
		void set_firewall_address(const std::wstring& value);

		/**
		 * Retrieves the last host address.
		*/
		std::wstring get_host_address() const;

		/**
		 * Saves the last host address.
		*/
		void set_host_address(const std::wstring& value);

		/**
		 * Retrieves the full screen mode flag.
		*/
		bool get_full_screen() const;

		/**
		 * Saves the full screen mode flag.
		*/
		void set_full_screen(bool fullscreen);

		/**
		 * Retrieves the clear RDP user name flag.
		*/
		bool get_clear_rdp_username() const;

		/**
		 * Saves the clear RDP user name flag.
		*/
		void set_clear_username(bool clear_username);

		/**
		 * Retrieves the span mode flag.
		*/
		bool get_span_mode() const;

		/**
		 * Saves the span mode flag.
		*/
		void set_span_mode(bool span_mode);

		/**
		 * Retrieves the multi monitors mode flag.
		*/
		bool get_multimon_mode() const;

		/**
		 * Saves the multi monitors mode flag.
		*/
		void set_multimon_mode(bool multimon_mode);

		/**
		 * Retrieves the RDP administrator console mode flag.
		*/
		bool get_admin_console() const;

		/**
		 * Saves the RDP administrator console mode flag.
		*/
		void set_admin_console(bool admin_console);

		/**
		 * Retrieves the RDP file mode flag.
		*/
		bool get_rdpfile_mode() const;

		/**
		 * Saves the RDP file mode flag.
		*/
		void set_rdpfile_mode(bool rdpfile_mode);

		/**
		 * Retrieves the RDP filename.
		*/
		std::wstring get_rdp_filename() const;

		/**
		 * Saves the RDP filename.
		*/
		void set_rdp_filename(const std::wstring& rdp_filename);

		/**
		 * Retrieves the RDP client screen size.
		*/
		ScreenSize get_screen_size() const;

		/**
		 * Saves the RDP client screen size.
		*/
		void set_screen_size(const ScreenSize& size);

		/**
		 * Retrieves the authentication mode.
		*/
		fw::AuthMethod get_auth_method() const;

		/**
		 * Saves the authentication mode.
		*/
		void set_auth_method(fw::AuthMethod auth_method);

	private:
		//- the registry root key.
		aux::RegKey _key;

		//- a convenient method to retrieve a flag value.
		bool get_bool(const std::wstring& value_name) const;

		//- a convenient method to save a flag value.
		void set_bool(const std::wstring& value_name, const bool value);

		// - a convenient method to retrieve an int value.
		int get_int(const std::wstring& value_name, const int default_value) const;

		//- a convenient method to save an int value.
		void set_int(const std::wstring& value_name, const int value);

		//- registry keys.
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
		static const std::wstring AUTH_METHOD;
	};

}
