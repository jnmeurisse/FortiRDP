/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include "Logger.h"

#include <array>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <vector>
#include "tools/Mutex.h"


namespace tools {

	Logger* Logger::_logger = nullptr;

	Logger::Logger() :
		_writers(),
		_mutex(),
		_level(LL_INFO)
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

		for (auto& writer : _writers) {
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


	std::string Logger::fmt(const char* format, va_list args)
	{
		size_t buffer_size = 132;

		while (true)
		{
			va_list args_copy;
			std::vector<char> buffer(buffer_size);

			va_copy(args_copy, args);
			const int n = std::vsnprintf(buffer.data(), buffer_size, format, args_copy);
			va_end(args_copy);

			if (n < 0)
				return "";

			// The string has been completely written only when n is non negative 
			// and less than size.  The buffer contains a null terminated string.
			if (n > 0 && n < buffer_size)
				return buffer.data();
			else
				buffer_size *= 2;
		}
	}


	void Logger::log(Level level, const char* format, va_list args)
	{
		std::string formatted_text = fmt(format, args);

		if (!formatted_text.empty()) {
			write(level, formatted_text);
		}
		else {
			write(Level::LL_ERROR, "internal error : fmt returned empty string");
		}
	}


	static std::string datetime()
	{
		// Get current system time and convert it to local time
		tm local_time;
		const time_t now = time(nullptr);
		localtime_s(&local_time, &now);

		std::array<char, 128> buffer = { 0 };
		const size_t output_size = std::strftime(buffer.data(), buffer.size(), "%F %T", &local_time);

		return output_size == 0 ? "" : buffer.data();
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
