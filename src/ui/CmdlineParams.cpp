/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "ui/CmdlineParams.h"

#include <iostream>

#include "tools/Path.h"
#include "tools/SysUtil.h"
#include "tools/StrUtil.h"
#include "tools/XGetopt.h"

CmdlineParams::CmdlineParams()
{
}

	
CmdlineParams::~CmdlineParams()
{
}


bool CmdlineParams::initialize()
{
	bool rc = true;
	int argc = 0;
	LPWSTR* const argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
	
	if (argv) {
		// skip first argument which is the program name
		rc = initialize(argc-1, &argv[1]);

		// free storage used by the argv array
		LocalFree(argv);
	}
	
	return rc;
}


bool CmdlineParams::initialize(int argc, LPWSTR argv[])
{
	_full_screen = false;
	_admin_console = false;
	_multi_clients = false;
	_span_mode = false;
	_multimon_mode = false;
	_clear_lastuser = false;
	_keep_alive = false;

	_verbose = false;
	_trace = false;

	_ca_filename = L"";
	_app_name = L"mstsc";
	_local_port = 0;
	_rdp_filename = L"";

	int c;
	while ((c = getopt(argc, argv, L"?u:famvc:tx:p:sr:lCMnk")) != EOF) {
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
			if (!tools::str2i(optarg, _local_port))
				_local_port = -1;
			break;

		case L'c':
			_ca_filename = tools::trim(optarg);
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

		case L'k':
			_keep_alive = true;
			break;

		default:
			break;

		}
	}

	// trace allowed only if verbose
	if (_trace && !_verbose)
		return false;

	// console mode, full screen, span, multi options and rdp file allowed only if app is rdp
	if ((_admin_console || _full_screen || _span_mode || _multimon_mode || !_rdp_filename.empty()) 
		&& (!is_mstsc()))
		return false;

	// local port is not valid
	if (_local_port < 0 || _local_port > 65535)
		return false;

	// get the last two arguments
	int i = 0;
	for (; optind < argc; optind++) {
		if (i == 0) {
			_fw_address = tools::trim(argv[optind]);
		}
		else if (i == 1) {
			_host_addres = tools::trim(argv[optind]);
		}

		i++;
	}

	return true;
}


void CmdlineParams::print_usage()
{
	// Retrieve major/minor version from .exe
	const tools::Path app_path = tools::Path::get_module_path();
	const std::string version = tools::get_file_ver(app_path.to_string());

	// show program parameters
	std::cout << tools::string_format("fortirdp %s (jn.meurisse@gmail.com)\n\n", version.c_str());
	std::cout << "fortirdp [-v [-t]] [-u username] [-c cacert_file] [-x app] [-f] [-a] [-s] [-p port]\n";
	std::cout << "         [-r rdp_file] [-m] [-l] [-C] [-M] [-n] [-k] firewall-ip[:port1] remote-ip[:port2]\n";
	std::cout << "\n";
	std::cout << "Options :\n";
	std::cout << "\t-v             Verbose mode (use -t to trace tls conversation, high verbosity !)\n";
	std::cout << "\t-u username    Specifies a user name for login to the firewall.\n";
	std::cout << "\t-c cacert_file Defines the Certificate Authority.\n";
	std::cout << "\t-x app         Specifies the application to launch instead of mstsc.\n";
	std::cout << "\t               The application must be specified with the syntax path{;parameter;parameter...}.\n";
	std::cout << "\t               Where path is the path to the executable file.\n";
	std::cout << "\t               and parameters are passed to the launched application.\n";
	std::cout << "\t               Parameters should contains ${host} and ${port} variables that are replaced by\n";
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
	std::cout << "\t-k             Specifies to send keep alive message to the sslvpn portal.\n";
	std::cout << "\tfirewall-ip    Specifies the hostname or IP address of the firewall to connect to.\n";
	std::cout << "\t               By default, the connection is done on port 10443. The 'port1' parameter\n";
	std::cout << "\t               allows to specify another port number on the firewall.\n";
	std::cout << "\tremote-ip      Specifies the IP address of the computer to connect to.\n";
	std::cout << "\t               By default, the RDP connection is done on port 3389. The 'port2' parameter\n";
	std::cout << "\t               allows to specify another port number on the terminal server.\n";
	std::cout << "\n";
}


