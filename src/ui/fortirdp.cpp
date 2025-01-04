/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
// fortirdp.cpp�: application entry point.
//

#include <iostream>
#include <io.h>
#include <fcntl.h>

#include "net\SslConfig.h"
#include "ui\fortirdp.h"
#include "ui\ConnectDialog.h"
#include "ui\CmdlineParams.h"
#include "tools\Logger.h"
#include "tools\Path.h"

#include "lwip\init.h"
#include "lwip\dns.h"

#include "mbedccl\init.h"
#include "mbedccl\debug.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
	name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
	processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Pr�-d�clarations des fonctions incluses dans ce module de code�:
INT_PTR CALLBACK MainDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
static void RedirectStdioToConsole();
static bool is_wow64();
static void lwip_log_cb(void *ctx, int level, const char* fmt, va_list args);
static void show_fatal_error();
static void debug_fn(int level, const char* buffer, void* data);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	tools::Logger* const logger = tools::Logger::get_logger();
	MSG msg;
	CmdlineParams cmdline_params;
	tools::FileLogWriter writer;

	// Stop running if a 32 Bits application is running in the wow64. The 
	// Task launcher can not wait for the child application which could be
	// a 64 bits application
#ifndef _WIN64
	if (is_wow64()) {
		MessageBox(0, L"This version does not run on a 64bit windows.", L"FortiRDP", MB_ICONSTOP);
		return 1;
	}
#endif

	// load command line arguments 
	bool invalid_args = !cmdline_params.initialize();
	
	if (invalid_args) {
		// display command line syntax if an invalid argument was specified.
		RedirectStdioToConsole();
		std::cout << "Error: invalid command line" << std::endl;
		cmdline_params.print_usage();
		std::cout.flush();

		while (GetMessage(&msg, nullptr, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return 1;
	}

	// Get the application path
	tools::Path desktop_path{ tools::Path::get_desktop_path() };

	// Should we write to a log file ?
	if (cmdline_params.verbose()) {
		// the log file is created on the desktop of the current user
		writer.open(tools::Path(desktop_path.folder(), L"fortirpd.log").to_string());
		logger->add_writer(&writer);

		logger->set_level(tools::Logger::LL_DEBUG);
		if (cmdline_params.trace()) 
			logger->set_level(tools::Logger::LL_TRACE);
	}


	// Initialize lwip stack
	lwip_init();
	//TODO: remove this call, was called in lwip_init : dns_init();
	sys_set_logger(lwip_log_cb, logger);

	// Initialize the mbedtls c++ compatibility layer
	mbed_errnum errnum = ::mbedccl_initialize();
	if (errnum != 0) {
		show_fatal_error();
	} 
	else {
		// Configure a debug callback
		::mbedccl_set_debug_cb(debug_fn, logger);
		//TODO: mbedccl_set_debug_treshod(logger->get_level());
		::mbedccl_set_debug_treshod(2);

		// Create the main dialog in a reduced scope.  This forces the
		// compiler to destroy the ConnectDialog when the application loop
		// ends (and before the destruction of the logger).
		ConnectDialog main_dialog(hInstance, cmdline_params);
		main_dialog.show_window(nCmdShow);

		// Main application loop
		while (GetMessage(&msg, nullptr, 0, 0)) {
			if (IsDialogMessage(main_dialog.window_handle(), &msg))
				continue;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	mbedccl_terminate();

	if (cmdline_params.verbose()) {
		logger->debug("End.");
		writer.flush();
		logger->remove_writer(&writer);
	}

	return 0;
}


static void RedirectStdioToConsole()
{
	if (AllocConsole()) {

		HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		COORD size = { 120, 1000 };
		SMALL_RECT position = { 0, 0, 119, 50 };

		// set the screen buffer large enough
		SetConsoleScreenBufferSize(hStdOut, size);

		// set text color 
		SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);

		// and font
		CONSOLE_FONT_INFOEX cfi;
		cfi.cbSize = sizeof cfi;
		cfi.nFont = 0;
		cfi.dwFontSize.X = 0;
		cfi.dwFontSize.Y = 12;
		cfi.FontFamily = FF_DONTCARE;
		cfi.FontWeight = FW_NORMAL;
		wcscpy_s(cfi.FaceName, L"Lucida Console");
		SetCurrentConsoleFontEx(hStdOut, FALSE, &cfi);

		// set position and size
		SetConsoleWindowInfo(hStdOut, TRUE, &position);

		// reopen stdout and stderr 
		FILE* new_stdout = nullptr;
		FILE* new_stderr = nullptr;

		freopen_s(&new_stdout, "CONOUT$", "w", stdout);
		freopen_s(&new_stderr, "CONOUT$", "w", stderr);

		std::cout.clear();
		std::wcout.clear();

		std::cerr.clear();
		std::wcerr.clear();
	}

	return;
}


static bool is_wow64()
{
	BOOL bIsWow64 = FALSE;
	typedef BOOL(APIENTRY *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process;
	HMODULE module = GetModuleHandle(L"kernel32");
	const char funcName[] = "IsWow64Process";

	if (!module)
		return false;

	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(module, funcName);
	if (fnIsWow64Process != NULL) {
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
			throw std::exception("IsWow64Process error");
	}

	return bIsWow64 != FALSE;
}


static void lwip_log_cb(void *ctx, int level, const char* fmt, va_list args)
{
	tools::Logger* const logger = static_cast<tools::Logger*>(ctx);
	
	switch (level)
	{
	case tools::Logger::Level::LL_ERROR:
		logger->log(tools::Logger::Level::LL_ERROR, fmt, args);
		break;

	case tools::Logger::Level::LL_DEBUG:
		logger->log(tools::Logger::Level::LL_DEBUG, fmt, args);
		break;

	default:
		logger->log(tools::Logger::Level::LL_TRACE, fmt, args);
		break;
	}
}

static void show_fatal_error()
{
	::MessageBox(0, L"mbedssl library initialization failure.", L"FortiRDP error", MB_SYSTEMMODAL);
}

static void debug_fn(int level, const char* buffer, void* data)
{
	((void)level);
	tools::Logger* const logger = (tools::Logger *)data;

	if (logger && level != 0 && strlen(buffer) > 0) {
		logger->trace(buffer);
	}
}

