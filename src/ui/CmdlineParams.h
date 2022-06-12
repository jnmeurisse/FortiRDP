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
#include <vector>

class CmdlineParams final
{
public:
	explicit CmdlineParams();
	~CmdlineParams();

	/*! Initializes this instance from the command line string
	*/
	bool initialize();

	/*! Initializes this instance from an array of arguments
	*/
	bool initialize(int argc, LPWSTR argv[]);

	/*! prints the correct usage of this program to stdout
	*/
	void print_usage();

	/*! returns the firewall address
	*/
	inline const std::wstring& firewall_address() const { return _fw_address; }
	
	/*! returns the host address
	*/
	inline const std::wstring& host_address() const { return _host_addres; }

	/*! returns the ca filename
	*/
	inline const std::wstring& ca_filename() const { return _ca_filename; }

	/*! returns the user name
	*/
	inline const std::wstring& username() const { return _username; }
	
	/*! returns the app name including parameters
	*/
	inline const std::wstring& appname() const { return _app_name; }

	/*! returns true if appname is mstsc
	*/
	inline bool is_mstsc() const { return _app_name.compare(L"mstsc") == 0; }

	/*! returns the .rdp filename
	*/
	inline const std::wstring& rdp_filename() const { return _rdp_filename; }

	/*! multiple clients are allowed
	*/
	inline bool multi_clients() const { return _multi_clients; }

	/*! full screen (only if app = rdp)
	*/
	inline bool full_screen() const { return _full_screen; }

	/*! span mode (only if app = rdp)
	*/
	inline bool span_mode() const { return _span_mode; }

	/*! multi monitor mode (only if app = rdp)
	*/
	inline bool multimon_mode() const { return _multimon_mode; }

	/*! admin console (only if app = rdp)
	*/
	inline bool admin_console() const { return _admin_console; }

	/*! local port, if not specified or 0, the program use a random port number
	*/
	inline int local_port() const { return _local_port; }

	/*! False if Nagle algorithm must be disabled
	*/
	inline bool tcp_nodelay() const { return _tcp_nodelay; }

	/*! returns true if deletion of last used username from rdp login window 
	 * is requested 
	*/
	inline bool clear_rdp_username() const { return _clear_lastuser; }

	/*! is verbose mode required 
	*/
	inline bool verbose() const { return _verbose; }
	
	/*! is trace mode required 
	*/
	inline bool trace() const { return _trace; }


private:
	// Command line arguments
	std::wstring _username;
	std::wstring _fw_address;
	std::wstring _host_addres;
	std::wstring _ca_filename;
	std::wstring _app_name;
	std::wstring _rdp_filename;
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