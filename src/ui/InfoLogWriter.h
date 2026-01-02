/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <Windows.h>
#include "tools/Logger.h"


namespace ui {

	class InfoLogWriter final : public tools::LogWriter
	{
	public:
		explicit InfoLogWriter(HWND hWnd, tools::LogLevel level);

		void write(tools::LogLevel level, int indend, const void* object, const std::string& text) override;
		void flush() override;


	private:
		const HWND _hWnd;
		tools::LogQueue _logQueue;
	};

}
