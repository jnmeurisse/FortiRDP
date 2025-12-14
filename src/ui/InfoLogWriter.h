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
#include "tools/Logger.h"


namespace ui {

	class InfoLogWriter final : public tools::LogWriter
	{
	public:
		explicit InfoLogWriter(HWND hWnd);
		virtual ~InfoLogWriter() override;

		virtual void write(tools::Logger::Level level, const std::u8string& text) override;
		virtual void flush() override;


	private:
		const HWND _hWnd;

		tools::LogQueue _logQueue;
	};

}
