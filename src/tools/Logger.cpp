/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Logger.h"

#include <cstdarg>
#include <cstdio>
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
		if (writer) {
			Mutex::Lock lock{ _mutex };

			_writers.push_back(writer);
			_writers.unique();
		}
	}


	void Logger::remove_writer(LogWriter* writer)
	{
		if (writer) {
			Mutex::Lock lock{ _mutex };

			_writers.remove(writer);
		}
	}


	Logger* Logger::get_logger()
	{
		if (_logger == nullptr) {
			_logger = new Logger();
		}

		return _logger;
	}


	void Logger::write(Level level, const std::string& text)
	{
		Mutex::Lock lock{ _mutex };

		for (auto writer : _writers) {
			writer->write(level, text);
			writer->flush();
		}
	}


	void Logger::log(Level level, const std::string& text)
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


	char* Logger::fmt(const char* format, va_list args)
	{
		va_list args_copy;

		// Compute the number of characters.
		va_copy(args_copy, args);
		const size_t size = vsnprintf(nullptr, 0, format, args_copy);
		va_end(args_copy);

		if (size < 0)
			return nullptr;

		// Allocate and format the text.
		char* text = new char[size + 1];
		if (vsnprintf(text, size + 1, format, args) < 0) {
			delete[] text;
			return nullptr;
		}

		// Return the formatted text
		return text;
	}


	void Logger::log(Level level, const char* format, va_list args)
	{
		const char* const text = fmt(format, args);

		if (text) {
			write(level, text);
			delete[] text;
		}
		else {
			write(Level::LL_ERROR, "internal error : fmt returned nullptr");
		}
	}


	static std::string datetime()
	{
		// Get current system time and convert it to local time
		tm local_time;
		const time_t now = time(nullptr);
		localtime_s(&local_time, &now);

		char buffer[128] { 0 };
		strftime(buffer, sizeof(buffer), "%F %T", &local_time);

		return std::string{ buffer };
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


	void FileLogWriter::write(Logger::Level level, const std::string& text)
	{
		(void)level;
		if (_ofs.is_open()) {
			_ofs << datetime() << " > " << text << std::endl;
		}
	}


	void FileLogWriter::flush()
	{
		if (_ofs.is_open()) {
			_ofs.flush();
		}
	}


	LogQueue::LogQueue() :
		_queue(),
		_mutex()
	{
	}


	void LogQueue::push(const std::string& text)
	{
		Mutex::Lock lock(_mutex);
		_queue.push(text);
	}

	
	std::string LogQueue::pop()
	{
		tools::Mutex::Lock lock(_mutex);

		std::string text = _queue.front();
		_queue.pop();
		return text;
	}

}
