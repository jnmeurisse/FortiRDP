/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "InfoLogWriter.h"

#include <cstring>
#include "ui/AsyncMessage.h"

namespace ui {

	InfoLogWriter::InfoLogWriter(HWND hWnd) :
		_hWnd(hWnd)
	{
	}


	InfoLogWriter::~InfoLogWriter()
	{
	}


	void InfoLogWriter::write(tools::Logger::Level level, const char* text)
	{
		if (level >= tools::Logger::LL_INFO) {
			AsyncMessage::OutputInfoMessageRequest.post(_hWnd, (void*)_strdup(text));
		}
	}

}
