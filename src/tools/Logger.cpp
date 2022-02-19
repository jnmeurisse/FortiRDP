/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#include <ctime>
#include "tools/Logger.h"
#include "tools/StrUtil.h"

namespace tools {

	Logger* Logger::_logger = nullptr;

	Logger::Logger() :
		_writers(),
		_level(LL_INFO),
		_mutex()
	{
	}


	Logger::~Logger()
	{
	}


	void Logger::set_level(Level level)
	{
		_level = level;
	}


	void Logger::add_writer(LogWriter* writer)
	{
		Mutex::Lock lock(_mutex);

		_writers.push_back(writer);
		_writers.unique();
	}


	void Logger::remove_writer(LogWriter* writer)
	{
		Mutex::Lock lock(_mutex);

		_writers.remove(writer);
	}


	Logger* Logger::get_logger()
	{
		if (_logger == nullptr) {
			_logger = new Logger();
		}

		return _logger;
	}


	void Logger::write(Logger::Level level, const char* text)
	{
		Mutex::Lock lock(_mutex);

		for (auto writer : _writers) {
			writer->write(level, text);
			writer->flush();
		}
	}


	void Logger::log(Level level, const std::string& text)
	{
		if (is_enabled(level)) {
			write(level, text.c_str());
		}
	}


	void Logger::log(Level level, const std::wstring& text)
	{
		if (is_enabled(level)) {
			write(level, tools::wstr2str(text).c_str());
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
		char* text = 0;
		int size = 0;

		//  compute the number of characters in the string referenced by the list of arguments.
		size = _vscprintf(format, args) + 1;
		text = new char[size];
		vsprintf_s(text, size, format, args);

		return text;
	}


	void Logger::log(Level level, const char* format, va_list args)
	{
		char* text = fmt(format, args);
		write(level, text);
		delete[] text;
	}


	static std::string datetime()
	{
		// get current system time
		time_t now = time(nullptr);

		// convert to local time
		tm local_time;
		localtime_s(&local_time, &now);

		// output as a string
		char buffer[128] = { 0 };
		strftime(buffer, sizeof(buffer), "%F %T", &local_time);

		return std::string(buffer);
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


	void FileLogWriter::write(Logger::Level level, const char * text)
	{
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

}
