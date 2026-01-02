/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <cstdarg>
#include <fstream>
#include <list>
#include <queue>
#include <string>
#include <stack>
#include "tools/Mutex.h"

#define PTR_VAL(ptr) (reinterpret_cast<std::uintptr_t>(ptr))


#define DEBUG_CTOR(logger)	(logger)->debug("* ctor::%s", __class__)
#define DEBUG_DTOR(logger)	(logger)->debug("* dtor::%s", __class__)


#define DEBUG_ENTER_FMT(logger, fmt, ...) \
			aux::LogScope __scope(logger, aux::LogLevel::LL_DEBUG, this, __class__, __func__, fmt, __VA_ARGS__)
#define TRACE_ENTER_FMT(logger, fmt, ...) \
			aux::LogScope __scope(logger, aux::LogLevel::LL_TRACE, this, __class__, __func__, fmt, __VA_ARGS__)

#define DEBUG_ENTER(logger) \
			 aux::LogScope __scope(logger, aux::LogLevel::LL_DEBUG, this, __class__, __func__)
#define TRACE_ENTER(logger) \
			aux::LogScope __scope(logger, aux::LogLevel::LL_TRACE, this, __class__, __func__)

#define LOG_DEBUG(_logger, fmt, ...) \
			if ((_logger)->is_debug_enabled()) \
				(_logger)->log(aux::LogLevel::LL_DEBUG, "= %s::%s - "##fmt, __class__, __func__, __VA_ARGS__)

#define LOG_TRACE(_logger, fmt, ...) \
			if ((_logger)->is_trace_enabled()) \
				(_logger)->log(aux::LogLevel::LL_TRACE, "= %s::%s - "##fmt, __class__, __func__, __VA_ARGS__)



namespace aux {

	class LogWriter;

	 enum class LogLevel { LL_TRACE = 1, LL_DEBUG = 2, LL_INFO = 3, LL_ERROR = 4 };

	/**
	* The application Logger.
	*/
	class Logger final
	{
	public:
		Logger();

		/**
		 * Logs a message.
		*/
		void log(LogLevel level, const std::string& text);
		void log(LogLevel level, const char* format, ...);
		void log(LogLevel level, const char* format, va_list args);
		void trace(const char* format, ...);
		void debug(const char* format, ...);
		void info(const char* format, ...);
		void error(const char* format, ...);

		/**
		 * Sets the threshold to 'level'.
		 *
		 * Logging message than are less severe than the specified level are ignored.
		*/
		void set_level(LogLevel level);
		
		/**
		 * Returns the current level.
		*/
		inline LogLevel get_level() const noexcept { return _level; }

		/** 
		 * Checks if the specified level is more severe than the current level
		*/
		inline bool is_enabled(LogLevel level) const noexcept { return level >= _level; }

		/**
		 * Checks if the corresponding level is enabled.
		*/
		inline bool is_info_enabled() const noexcept { return is_enabled(LogLevel::LL_INFO); }
		inline bool is_debug_enabled() const noexcept { return is_enabled(LogLevel::LL_DEBUG); }
		inline bool is_trace_enabled() const noexcept { return is_enabled(LogLevel::LL_TRACE); }

		/**
		 * Adds a writer to this logger.
		*/
		void add_writer(LogWriter* writer);

		/**
		 * Removes a writer from this logger.
		*/
		void remove_writer(LogWriter* writer);

		/**
		 * Returns the instance of the logger.
		*/
		static Logger* get_logger();

	private:
		// A reference to the application logger (singleton).
		static Logger* _logger;

		// A list of writers.
		std::list<LogWriter *> _writers;

		// A mutex to protect access to the list of writers.
		aux::Mutex _mutex;

		// The current logger level.
		LogLevel _level;

		friend class LogScope;
		// Current log message indentation
		static thread_local int _indent_level;

		// Stack of this pointers
		static thread_local std::stack<const void*> _this_stack;

		/**
		 * Writes a message to the log writers.
		*/
		void write(LogLevel level, const std::string& text);
		void write(LogLevel level, const char* format, va_list args);
	};


	class LogScope final
	{
	public:
		explicit LogScope(Logger *logger, LogLevel level,
			const void *this_address, const char* class_name, const char* func_name, const char* fmt, ...);
		explicit LogScope(Logger* logger, LogLevel level,
			const void* this_address, const char* class_name, const char* func_name);
		~LogScope();

	private:
		Logger* _logger;
		const LogLevel _level;
		const char* _class_name;
		const char* _func_name;
	};


	/**
	 * An abstract log writer.
	*/
	class LogWriter
	{
	public:
		explicit LogWriter(LogLevel level);
		virtual ~LogWriter() = default;

		virtual void write(LogLevel level, int indent, const void* object, const std::string& text) = 0;
		virtual void flush() { return; }

		/**
		 * Checks if the specified level is more severe than the current level
		*/
		inline bool is_enabled(LogLevel level) const noexcept { return level >= _level; }

	protected:
		char get_level_char(LogLevel level) const noexcept;

	private:
		// The current writer log level.
		const LogLevel _level;
	};


	/**
	 * A file log writer.
	*/
	class FileLogWriter final: public LogWriter
	{
	public:
		explicit FileLogWriter(LogLevel level);
		~FileLogWriter() override = default;

		bool open(const std::wstring& filename);
		void write(LogLevel level, int indent, const void* object,  const std::string& text) override;
		void flush() override;

	private:
		std::ofstream _ofs;
	};


	class LogQueue final
	{
	public:
		inline size_t size() const { return _queue.size(); }
		void push(const std::string& text);
		std::string pop();

		inline aux::Mutex& mutex() { return _mutex; }

	private:
		std::queue<std::string> _queue;
		aux::Mutex _mutex;
	};

}
