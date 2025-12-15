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
		_logQueue(),
		_hWnd(hWnd)
	{
	}


	InfoLogWriter::~InfoLogWriter()
	{
	}


	void InfoLogWriter::write(tools::Logger::Level level, const std::string& text)
	{
		if (level >= tools::Logger::LL_INFO) {
			tools::Mutex::Lock lock{ _logQueue.mutex() };
			_logQueue.push(text);
			AsyncMessage::OutputInfoEvent->send_message(_hWnd, &_logQueue);
		}
	}


	void InfoLogWriter::flush()
	{
		tools::Mutex::Lock lock{ _logQueue.mutex() };
		if (_logQueue.size() > 0)
			AsyncMessage::OutputInfoEvent->send_message(_hWnd, &_logQueue);
	}

}
