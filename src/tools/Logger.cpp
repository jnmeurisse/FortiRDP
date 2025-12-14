/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Logger.h"

#include <cstdarg>
#include <ctime>
#include "tools/Mutex.h"


namespace tools {

	Logger* Logger::_logger = nullptr;

	Logger::Logger() :
		_writers(),
		_level(LL_INFO),
		_mutex()
	{
	}


	void Logger::set_level(Level level)
	{
		_level = level;
	}


	void Logger::add_writer(LogWriter* writer)
	{
		Mutex::Lock lock{ _mutex };

		_writers.push_back(writer);
		_writers.unique();
	}


	void Logger::remove_writer(LogWriter* writer)
	{
		Mutex::Lock lock{ _mutex };

		_writers.remove(writer);
	}


	Logger* Logger::get_logger()
	{
		if (_logger == nullptr) {
			_logger = new Logger();
		}

		return _logger;
	}


	void Logger::write(Level level, const std::u8string& text)
	{
		Mutex::Lock lock{ _mutex };

		for (auto writer : _writers) {
			writer->write(level, text);
			writer->flush();
		}
	}


	void Logger::log(Level level, const std::u8string& text)
	{
		if (is_enabled(level)) {
			write(level, text);
		}
	}

	
	void Logger::log(Level level, const char* format, ...)
	{
		if (is_enabled(level)) {
			va_list args;
			va_start(args, format);
			log(level, format, args);
			va_end(args);
		}
	}
	

	void Logger::trace(const char* format, ...)
	{
		if (is_trace_enabled()) {
			va_list args;
			va_start(args, format);
			log(LL_TRACE, format, args);
			va_end(args);
		}
	}


	void Logger::debug(const char* format, ...)
	{
		if (is_debug_enabled()) {
			va_list args;
			va_start(args, format);
			log(LL_DEBUG, format, args);
			va_end(args);
		}
	}


	void Logger::info(const char* format, ...)
	{
		if (is_info_enabled()) {
			va_list args;
			va_start(args, format);
			log(LL_INFO, format, args);
			va_end(args);
		}
	}


	void Logger::error(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		log(LL_ERROR, format, args);
		va_end(args);
	}


	char8_t* Logger::fmt(const char* format, va_list args)
	{
		va_list args_copy;

		// Compute the number of characters.
		va_copy(args_copy, args);
		const int size = vsnprintf(nullptr, 0, format, args_copy);
		va_end(args_copy);

		if (size < 0)
			return nullptr;

		// Allocate and format the text.
		char8_t* text = new char8_t[size + 1];
		if (vsnprintf((char *)text, size + 1, format, args) < 0) {
			delete[] text;
			return nullptr;
		}

		// Return the formatted text
		return text;
	}


	void Logger::log(Level level, const char* format, va_list args)
	{

		const char8_t* const text = fmt(format, args);
		if (text) {
			write(level, std::u8string(text));
			delete[] text;
		}
		else {
			write(Level::LL_ERROR, u8"internal error : fmt returned nullptr");
		}
	}


	static std::u8string datetime()
	{
		// Get current system time and convert it to local time
		tm local_time;
		const time_t now = time(nullptr);
		localtime_s(&local_time, &now);

		char8_t buffer[128] { 0 };
		strftime((char *)buffer, sizeof(buffer), "%F %T", &local_time);

		return std::u8string{ buffer };
	}


	FileLogWriter::FileLogWriter() :
		_ofs()
	{
	}


	FileLogWriter::~FileLogWriter()
	{
		if (_ofs.is_open()) {
			_ofs.close();
		}
	}


	bool FileLogWriter::open(const std::wstring& filename)
	{
		_ofs.open(filename, std::ostream::out);

		return _ofs.failbit == std::ofstream::goodbit;
	}


	void FileLogWriter::write(Logger::Level level, const std::u8string& text)
	{
		(void)level;
		if (_ofs.is_open()) {
			write_utf8(_ofs, datetime());
			write_utf8(_ofs, u8" > ");
			write_utf8(_ofs, text);
			write_utf8(_ofs, u8"\n");
		}
	}


	void FileLogWriter::flush()
	{
		if (_ofs.is_open()) {
			_ofs.flush();
		}
	}


	void FileLogWriter::write_utf8(std::ofstream& os, const std::u8string& text)
	{
		os.write(reinterpret_cast<const char*>(text.data()), text.size());
	}


	LogQueue::LogQueue() :
		_queue(),
		_mutex()
	{
	}


	void LogQueue::push(const std::u8string& text)
	{
		Mutex::Lock lock(_mutex);
		_queue.push(text);
	}

	
	std::u8string LogQueue::pop()
	{
		tools::Mutex::Lock lock(_mutex);

		std::u8string text = _queue.front();
		_queue.pop();
		return text;
	}

}
