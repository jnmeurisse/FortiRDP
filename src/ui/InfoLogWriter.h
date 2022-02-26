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


class InfoLogWriter final : public tools::LogWriter
{
public:
	explicit InfoLogWriter(HWND hWnd);
	virtual ~InfoLogWriter();

	virtual void write(tools::Logger::Level level, const char* text) override;

private:
	const HWND _hWnd;
};

