/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
// fortirdp.cpp : application entry point.
//

#include "fortirdp.h"

#include <iostream>
#include <lwip/arch.h>
#include <lwip/init.h>
#include <lwip/dns.h>
#include "ui/ConnectDialog.h"
#include "ui/CmdlineParams.h"
#include "util/Logger.h"
#include "util/Path.h"


#pragma comment(linker,"\"/manifestdependency:type='win32' \
	name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
	processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Pré-déclarations des fonctions incluses dans ce module de code :
INT_PTR CALLBACK MainDialogProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
static void RedirectStdioToConsole();
static void lwip_log_cb(void *ctx, int level, const char* fmt, va_list args);
#ifndef _WIN64
static bool is_wow64();
#endif


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int    nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	aux::Logger* const logger = aux::Logger::get_logger();
	MSG msg;
	ui::CmdlineParams cmdline_params;
	aux::FileLogWriter writer(aux::LogLevel::LL_TRACE);

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
		// Display command line syntax if an invalid argument was specified.
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

	// Get the application path.
	aux::Path desktop_path{ aux::Path::get_desktop_path() };

	// Should we write to a log file ?
	if (cmdline_params.verbose()) {
		// The log file is created on the desktop of the current user.
		writer.open(aux::Path(desktop_path.folder(), L"fortirpd.log").to_string());
		logger->add_writer(&writer);

		logger->set_level(aux::LogLevel::LL_DEBUG);
		if (cmdline_params.trace()) 
			logger->set_level(aux::LogLevel::LL_TRACE);
	}

	// Initialize lwIP stack
	lwip_init();
	dns_init();
	sys_set_logger(lwip_log_cb, logger);

	// Create the main dialog in a reduced scope.  This forces the
	// compiler to destroy the ConnectDialog when the application loop
	// ends (and before the destruction of the logger).
	{
		ui::ConnectDialog main_dialog(hInstance, cmdline_params);
		main_dialog.show_window(nCmdShow);

		// Main application loop.
		while (GetMessage(&msg, nullptr, 0, 0)) {
			if (IsDialogMessage(main_dialog.window_handle(), &msg))
				continue;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

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

		// Set the screen buffer large enough
		SetConsoleScreenBufferSize(hStdOut, size);

		// Set text color
		SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);

		// and font.
		CONSOLE_FONT_INFOEX cfi;
		cfi.cbSize = sizeof cfi;
		cfi.nFont = 0;
		cfi.dwFontSize.X = 0;
		cfi.dwFontSize.Y = 12;
		cfi.FontFamily = FF_DONTCARE;
		cfi.FontWeight = FW_NORMAL;
		wcscpy_s(cfi.FaceName, L"Lucida Console");
		SetCurrentConsoleFontEx(hStdOut, FALSE, &cfi);

		// Set position and size
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

static void lwip_log_cb(void *ctx, int level, const char* fmt, va_list args)
{
	aux::Logger* const logger = static_cast<aux::Logger*>(ctx);
	
	if (level == LWIP_ERROR_MESSAGE)
		logger->log(aux::LogLevel::LL_ERROR, fmt, args);
	else if (level == LWIP_DIAG_MESSAGE)
		logger->log(aux::LogLevel::LL_DEBUG, fmt, args);
	else
		logger->log(aux::LogLevel::LL_TRACE, fmt, args);
}

#ifndef _WIN64
static bool is_wow64()
{
	BOOL bIsWow64 = FALSE;
	typedef BOOL(APIENTRY* LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
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
#endif
