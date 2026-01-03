/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include "util/Logger.h"


namespace ui {

	class InfoLogWriter final : public utl::LogWriter
	{
	public:
		explicit InfoLogWriter(HWND hWnd, utl::LogLevel level);

		void write(utl::LogLevel level, int indend, const void* object, const std::string& text) override;
		void flush() override;


	private:
		const HWND _hWnd;
		utl::LogQueue _logQueue;
	};

}
