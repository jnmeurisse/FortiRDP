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
#include <ctime>
#include <iomanip>
#include <memory>
#include <ostream>
#include <thread>
#include "util/Mutex.h"
#include "util/StrUtil.h"


namespace utl {

	static const std::unique_ptr<Logger> LOGGER = std::make_unique<Logger>();
	thread_local int Logger::_indent_level = 0;
	thread_local std::stack<const void*> Logger::_this_stack({ nullptr });


	Logger::Logger() :
		_writers(),
		_mutex(),
		_level(LogLevel::LL_INFO)
	{
	}


	void Logger::log(LogLevel level, const std::string& text)
	{
		if (is_enabled(level)) {
			write(level, text);
		}
	}


	void Logger::log(LogLevel level, const char* format, ...)
	{
		if (is_enabled(level)) {
			va_list args;
			va_start(args, format);
			write(level, format, args);
			va_end(args);
		}
	}


	void Logger::log(LogLevel level, const char* format, va_list args)
	{
		if (is_enabled(level))
			write(level, format, args);
	}


	void Logger::trace(const char* format, ...)
	{
		if (is_trace_enabled()) {
			va_list args;
			va_start(args, format);
			write(LogLevel::LL_TRACE, format, args);
			va_end(args);
		}
	}


	void Logger::debug(const char* format, ...)
	{
		if (is_debug_enabled()) {
			va_list args;
			va_start(args, format);
			write(LogLevel::LL_DEBUG, format, args);
			va_end(args);
		}
	}


	void Logger::info(const char* format, ...)
	{
		if (is_info_enabled()) {
			va_list args;
			va_start(args, format);
			write(LogLevel::LL_INFO, format, args);
			va_end(args);
		}
	}


	void Logger::error(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		write(LogLevel::LL_ERROR, format, args);
		va_end(args);
	}


	void Logger::set_level(LogLevel level)
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
		return LOGGER.get();
	}


	void Logger::write(LogLevel level, const std::string& text)
	{
		Mutex::Lock lock{ _mutex };

		const int indent = _indent_level;
		const void* object = _this_stack.top();
		for (auto& writer : _writers) {
			writer->write(level, indent, object, text);
			writer->flush();
		}
	}


	void Logger::write(LogLevel level, const char* format, va_list args)
	{
		write(level, utl::string_format(format, args));
	}


	LogScope::LogScope(Logger* logger, LogLevel level,
		const void* this_address, const char* class_name,
		const char* func_name, const char* format, ...) :
		_logger(logger),
		_level(level),
		_class_name(class_name),
		_func_name(func_name)
	{
		if (_logger->is_enabled(_level)) {
			if (format) {
				va_list args;
				va_start(args, format);
				const std::string message = utl::string_format(format, args);
				va_end(args);
				_logger->log(_level, "> %s::%s - %s", _class_name, _func_name, message.c_str());
			}
			else {
				_logger->log(_level, "> %s::%s", _class_name, _func_name);
			}
		}

		Logger::_indent_level++;
		Logger::_this_stack.push(this_address);
	}


	LogScope::LogScope(Logger* logger, LogLevel level,
		const void* this_address, const char* class_name, const char* func_name) :
		LogScope(logger, level, this_address, class_name, func_name, nullptr)
	{
	}

	LogScope::~LogScope()
	{
		Logger::_indent_level--;
		Logger::_this_stack.pop();

		if (_logger->is_enabled(_level))
			_logger->log(_level, "< %s::%s", _class_name, _func_name);
	}



	LogWriter::LogWriter(LogLevel level) :
		_level(level)
	{
	}


	char LogWriter::get_level_char(LogLevel level) const noexcept
	{
		switch (level) {
		case LogLevel::LL_DEBUG: return 'D';
		case LogLevel::LL_ERROR: return 'E'; 
		case LogLevel::LL_INFO:  return 'I';
		case LogLevel::LL_TRACE: return 'T';
		default: return 'U'; // Unknown
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


	FileLogWriter::FileLogWriter(LogLevel level) :
		LogWriter(level),
		_ofs()
	{
	}


	bool FileLogWriter::open(const std::wstring& filename)
	{
		_ofs.open(filename, std::ostream::out);

		return _ofs.is_open();
	}


	void FileLogWriter::write(LogLevel level, int indent, const void* object, const std::string& text)
	{
		if (_ofs.is_open() && is_enabled(level)) {
			_ofs 
				<< '[' << datetime() << "] "
				<< '[' << get_level_char(level) << "] "
				<< '[' << std::setw(5) << std::setfill('0') << std::this_thread::get_id() << "] "
				<< "0x" << std::setw(12) << std::setfill('0') << std::hex << PTR_VAL(object)
				<< " "
				<< std::string(indent, '.')
				<< text
				<< std::endl;
		}
	}


	void FileLogWriter::flush()
	{
		if (_ofs.is_open()) {
			_ofs.flush();
		}
	}


	void LogQueue::push(const std::string& text)
	{
		Mutex::Lock lock(_mutex);
		_queue.push(text);
	}

	
	std::string LogQueue::pop()
	{
		utl::Mutex::Lock lock(_mutex);

		std::string text = _queue.front();
		_queue.pop();
		return text;
	}

}
