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

	class InfoLogWriter final : public aux::LogWriter
	{
	public:
		explicit InfoLogWriter(HWND hWnd, aux::LogLevel level);

		void write(aux::LogLevel level, int indend, const void* object, const std::string& text) override;
		void flush() override;


	private:
		const HWND _hWnd;
		aux::LogQueue _logQueue;
	};

}
