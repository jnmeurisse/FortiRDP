/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "CmdlineParams.h"

#include <iostream>
#include <limits>
#include <Windows.h>
#include "tools/Path.h"
#include "tools/SysUtil.h"
#include "tools/StrUtil.h"
#include "tools/XGetopt.h"

namespace ui {


	bool CmdlineParams::initialize()
	{
		bool rc = true;
		int argc = 0;
		wchar_t **argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

		if (argv) {
			// Skip first argument which is the program name.
			rc = initialize(argc - 1, &argv[1]);

			// Free storage used by the argv array
			::LocalFree(argv);
		}

		return rc;
	}


	bool CmdlineParams::initialize(int argc, const wchar_t * const *argv)
	{
		_full_screen = false;
		_admin_console = false;
		_multi_clients = false;
		_span_mode = false;
		_multimon_mode = false;
		_screen_size = { 0, 0 };
		_clear_lastuser = false;

		_verbose = false;
		_trace = false;

		_auth_method = fw::AuthMethod::DEFAULT;
		_username.clear();
		_fw_address.clear();
		_host_addres.clear();
		_us_cert_filename = L"";
		_ca_cert_filename = L"";
		_app_name = L"mstsc";
		_local_port = 0;
		_rdp_filename = L"";
		_tcp_nodelay = false;

		int c;
		while ((c = getopt(argc, argv, L"?u:famvc:tx:p:sr:lCMnw:h:U:A:")) != EOF) {
			switch (c) {
			case L'?':
				return false;

			case L'u':
				_username = optarg;
				break;

			case L'f':
				_full_screen = true;
				break;

			case L'a':
				_admin_console = true;
				break;

			case L'p':
				int port;
				if (tools::str2i(optarg, port) && port > 0 && (port <= std::numeric_limits<uint16_t>::max()))
					_local_port = static_cast<uint16_t>(port);
				break;

			case L'c':
				_ca_cert_filename = tools::trim(optarg);
				break;

			case L'v':
				_verbose = true;
				break;

			case L't':
				_trace = true;
				break;

			case L'x':
				_app_name = tools::trim(optarg);
				break;

			case L's':
				_span_mode = true;
				break;

			case L'm':
				_multimon_mode = true;
				break;

			case L'r':
				_rdp_filename = tools::trim(optarg);
				break;

			case L'C':
				_clear_lastuser = true;
				break;

			case L'M':
				_multi_clients = true;
				break;

			case L'n':
				_tcp_nodelay = true;
				break;

			case L'w':
				if (!tools::str2i(optarg, _screen_size.width))
					_screen_size.width = -1;
				break;

			case L'h':
				if (!tools::str2i(optarg, _screen_size.height))
					_screen_size.height = -1;
				break;

			case L'U':
				_us_cert_filename = tools::trim(optarg);
				break;

			case L'A':
				if (std::wstring(optarg).compare(L"basic") == 0)
					_auth_method = fw::AuthMethod::BASIC;
				else if (std::wstring(optarg).compare(L"cert") == 0)
					_auth_method = fw::AuthMethod::CERTIFICATE;
				else if (std::wstring(optarg).compare(L"saml") == 0)
					_auth_method = fw::AuthMethod::SAML;
				else
					return false;
				break;

			default:
				break;
			}
		}

		// Trace is allowed only if verbose is enabled.
		if (_trace && !_verbose)
			return false;

		// Console mode, full screen, span, multi options and RDP file allowed only if app is mstsc.
		if ((_admin_console || _full_screen || _span_mode || _multimon_mode || !_rdp_filename.empty())
			&& (!is_mstsc()))
			return false;

		// Screen size configuration is allowed only if app is mstsc.
		if ((_screen_size.height != 0 || _screen_size.width != 0) && (!is_mstsc()))
			return false;

		// Check if the screen size is valid.
		if (!_screen_size.is_valid())
			return false;

		// Check if the local TCP port is valid.
		if (_local_port > 0)
			return false;

		// Validate the authentication method.
		bool auth_method_valid = false;
		switch (_auth_method) {
			case fw::AuthMethod::DEFAULT:
				auth_method_valid = _username.size() > 0 && _us_cert_filename.size() == 0;
				break;

			case fw::AuthMethod::BASIC:
				auth_method_valid = _username.size() > 0 && _us_cert_filename.size() == 0;
				break;

			case fw::AuthMethod::SAML:
				auth_method_valid = _username.size() == 0 && _us_cert_filename.size() == 0;
				break;

			case fw::AuthMethod::CERTIFICATE:
				auth_method_valid = _username.size() == 0 && _us_cert_filename.size() > 0;
				break;
		}

		if (!auth_method_valid)
			return false;

		// Get the last two arguments on the command line.
		int i = 0;
		for (; optind < argc; optind++) {
			if (i == 0) {
				_fw_address = tools::trim(argv[optind]);
			}
			else if (i == 1) {
				_host_addres = tools::trim(argv[optind]);
			}
			else
				return false;

			i++;
		}

		// TCP no delay is always set for a RDP connection.
		if (is_mstsc())
			_tcp_nodelay = true;

		return true;
	}


	void CmdlineParams::print_usage() const
	{
		// Retrieve major/minor version from .exe
		const tools::Path app_path{ tools::Path::get_module_path() };
		const std::string version{ tools::get_file_ver(app_path.to_string()) };

		// Show program parameters.
		std::cout << tools::string_format("fortirdp %s (jn.meurisse@gmail.com)\n\n", version.c_str());
		std::cout << "fortirdp [-v [-t]] [-A auth] [-u username] [-c cacert_file] [-x app] [-f] [-a] [-s] [-p port]\n";
		std::cout << "         [-r rdp_file] [-m] [-l] [-C] [-M] [-n] firewall-ip[:port1] remote-ip[:port2]\n";
		std::cout << "\n";
		std::cout << "Options :\n";
		std::cout << "\t-v             Verbose mode (use -t to trace tls conversation, high verbosity !)\n";
		std::cout << "\t-A auth        Specifies the authentication mode (basic, cert, saml)\n";
		std::cout << "\t-u username    Specifies a user name for login basic to the firewall.\n";
		std::cout << "\t-c cacert_file Defines the Certificate Authority.\n";
		std::cout << "\t-x app         Specifies the application to launch instead of mstsc.\n";
		std::cout << "\t               The application must be specified with the syntax path{;parameter;parameter...}.\n";
		std::cout << "\t               Where path is the path to the executable file and {;parameter;parameter...} are.\n";
		std::cout << "\t               optional parameters passed to the launched application.\n";
		std::cout << "\t               Parameters may contains ${host} and ${port} variables that are replaced by\n";
		std::cout << "\t               their effective values assigned dynamically when building the tunnel.\n";
		std::cout << "\t               If this parameter is not specified, app is c:\\windows\\system32\\mstsc.exe;/v:${host}:${port}\n";
		std::cout << "\t               Note that app can be empty (using \"\" syntax), it that case the application must be started\n";
		std::cout << "\t               manually.\n";
		std::cout << "\t-f             Starts Remote Desktop Connection in full-screen mode.\n";
		std::cout << "\t-a             Is used for administration of a Remote Desktop Session Host server.\n";
		std::cout << "\t-s             Enables Remote Desktop Span mode.\n";
		std::cout << "\t-p port        Specifies a static local port. The ${port} parameter in the app command line\n";
		std::cout << "\t               is replaced by the port instead of using a dynamic value\n";
		std::cout << "\t-r rdp_file    Defines the .rdp file passed to mstsc.\n";
		std::cout << "\t-m             Enables Remote Desktop Multimonitor mode.\n";
		std::cout << "\t-C             Specifies to clear the last rdp session username.\n";
		std::cout << "\t-M             Specifies that the tunnel can accept multiple client connections.\n";
		std::cout << "\t-n             Disables the Nagle algorithm.\n";
		std::cout << "\tfirewall-ip    Specifies the hostname or IP address of the firewall to connect to.\n";
		std::cout << "\t               By default, the connection is done on port 10443. The 'port1' parameter\n";
		std::cout << "\t               allows to specify another port number on the firewall.\n";
		std::cout << "\tremote-ip      Specifies the IP address of the computer to connect to.\n";
		std::cout << "\t               By default, the RDP connection is done on port 3389. The 'port2' parameter\n";
		std::cout << "\t               allows to specify another port number on the terminal server.\n";
		std::cout << "\n";
	}

}
