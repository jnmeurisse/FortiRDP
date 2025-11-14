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
#include "fw/AuthTypes.h"
#include "ScreenSize.h"


namespace ui {

	class CmdlineParams final
	{
	public:
		explicit CmdlineParams();
		~CmdlineParams();

		/**
		* Initializes this instance from the command line string.
		*/
		bool initialize();

		/**
		 * Initializes this instance from an array of arguments.
		*/
		bool initialize(int argc, LPWSTR argv[]);

		/**
		 * Prints the correct usage of this program to stdout.
		*/
		void print_usage();

		/**
		 * Returns the firewall address option.
		*/
		inline const std::wstring& firewall_address() const { return _fw_address; }

		/**
		 * Returns the host address option.
		*/
		inline const std::wstring& host_address() const { return _host_addres; }

		/**
		 * Returns the CA certificate filename option.
		*/
		inline const std::wstring& ca_cert_filename() const { return _ca_cert_filename; }

		/**
		 * Returns the authentication method option.
		*/
		inline fw::AuthMethod auth_method() const { return _auth_method; }

		/**
		 * Returns the user certificate filename option.
		*/
		inline const std::wstring& us_cert_filename() const { return _us_cert_filename; }

		/**
		 * Returns the user name option.
		*/
		inline const std::wstring& username() const { return _username; }

		/**
		 * Returns the application name including parameters option.
		*/
		inline const std::wstring& appname() const { return _app_name; }

		/**
		 * Returns true if the application name option is mstsc (remote desktop client).
		*/
		inline bool is_mstsc() const { return _app_name.compare(L"mstsc") == 0; }

		/**
		 * Returns the remote desktop client configuration file name option.
		*/
		inline const std::wstring& rdp_filename() const { return _rdp_filename; }

		/**
		 * Returns true if multiple clients option is specified.
		*/
		inline bool multi_clients() const { return _multi_clients; }

		/**
		 * Returns true if remote desktop client full screen mode option is enabled.
		 * (only if app = mstsc)
		*/
		inline bool full_screen() const { return _full_screen; }

		/**
		 * Returns true if remote desktop client span mode option is enabled.
		 * (only if app = mstsc)
		*/
		inline bool span_mode() const { return _span_mode; }

		/**
		 * Returns true if remote desktop client multi monitor mode option is enabled.
		 * (only if app = mstsc)
		*/
		inline bool multimon_mode() const { return _multimon_mode; }

		/**
		 * Returns the screen size specified on the command line.
		 * (only if app = mstsc)
		 */
		ScreenSize screen_size() const { return _screen_size; }

		/**
		 * Returns true if the remote desktop client admin console mode is enabled.
		 * (only if app = mstsc)
		*/
		inline bool admin_console() const { return _admin_console; }

		/**
		 * Returns a local port to listen to.
		 * If not specified or 0, the listener uses a random port number.
		*/
		inline int local_port() const { return _local_port; }

		/**
		 * Returns false if Nagle algorithm must be disabled.
		*/
		inline bool tcp_nodelay() const { return _tcp_nodelay; }

		/**
		 * Returns true if deletion of last used username from mstsc login window
		 * option is enabled.
		 * (only if app = mstsc)
		*/
		inline bool clear_rdp_username() const { return _clear_lastuser; }

		/**
		 * Returns if debug logs mode is enabled.
		*/
		inline bool verbose() const { return _verbose; }

		/**
		 * Returns if trace logs is enabled.
		*/
		inline bool trace() const { return _trace; }

	private:
		// Command line arguments.
		fw::AuthMethod _auth_method = fw::AuthMethod::BASIC;
		std::wstring _username;
		std::wstring _fw_address;
		std::wstring _host_addres;
		std::wstring _ca_cert_filename;
		std::wstring _us_cert_filename;
		std::wstring _app_name;
		std::wstring _rdp_filename;
		ScreenSize _screen_size{ 0,0 };
		int _local_port = 0;

		// Command line options
		bool _full_screen = false;
		bool _admin_console = false;
		bool _multi_clients = false;
		bool _span_mode = false;
		bool _multimon_mode = false;
		bool _clear_lastuser = false;
		bool _tcp_nodelay = false;

		bool _verbose = false;
		bool _trace = false;
	};

}
