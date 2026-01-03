/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "InfoLogWriter.h"

#include "ui/AsyncMessage.h"
#include "util/Mutex.h"

namespace ui {

	InfoLogWriter::InfoLogWriter(HWND hWnd, aux::LogLevel level) :
		LogWriter(level),
		_hWnd(hWnd),
		_logQueue()
	{
	}


	void InfoLogWriter::write(aux::LogLevel level, int indent, const void* object, const std::string& text)
	{
		if (is_enabled(level)) {
			aux::Mutex::Lock lock{ _logQueue.mutex() };
			_logQueue.push(text);
			AsyncMessage::OutputInfoEvent->send_message(_hWnd, &_logQueue);
		}
	}


	void InfoLogWriter::flush()
	{
		aux::Mutex::Lock lock{ _logQueue.mutex() };
		if (_logQueue.size() > 0)
			AsyncMessage::OutputInfoEvent->send_message(_hWnd, &_logQueue);
	}

}
