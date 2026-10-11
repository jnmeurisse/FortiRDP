/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "InfoLogWriter.h"

#include "ui/AsyncMessage.h"


namespace ui {

	InfoLogWriter::InfoLogWriter(HWND hWnd, utl::LogLevel level) :
		LogWriter(level),
		_hWnd(hWnd),
		_logQueue()
	{
	}


	void InfoLogWriter::write(utl::LogLevel level, int indent, const void* object, const std::string& text)
	{
		if (is_enabled(level)) {
			_logQueue.push(text);
			AsyncMessage::OutputInfoEvent->send_message(_hWnd, &_logQueue);
		}
	}


	void InfoLogWriter::flush()
	{
		AsyncMessage::OutputInfoEvent->send_message(_hWnd, &_logQueue);
	}

}
