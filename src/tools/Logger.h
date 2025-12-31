/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <cstdint>
#include <fstream>
#include <list>
#include <queue>
#include <string>
#include "tools/Mutex.h"


#ifdef _DEBUG
#define DEBUG_CTOR(logger) if ((logger)->is_debug_enabled()) { \
								(logger)->debug("... %x ctor::%s", (uintptr_t)this, __class__); \
								}
#define DEBUG_DTOR(logger) if ((logger)->is_debug_enabled()) { \
								(logger)->debug("... %x dtor::%s", (uintptr_t)this, __class__); \
								}
#else
#define DEBUG_CTOR(logger)
#define DEBUG_DTOR(logger)
#endif


#define DEBUG_ENTER(logger) if ((logger)->is_debug_enabled()) { \
								(logger)->debug("... %x enter %s::%s", (uintptr_t)this, __class__, __func__); \
								}

#define TRACE_ENTER(logger) if ((logger)->is_trace_enabled()) { \
								(logger)->trace(".... %x enter %s::%s", (uintptr_t)this, __class__, __func__); \
								}


#define THISADDR() (static_cast<void *>(this))

namespace tools {

	class LogWriter;

	/**
	* The application Logger.
	*/
	class Logger final
	{
	public:
		using Level = enum { LL_TRACE = 1, LL_DEBUG = 2, LL_INFO = 3, LL_ERROR = 4 } ;

		/**
		 * Logs a message.
		*/
		void log(Level level, const std::string& text);
		void log(Level level, const char* format, ...);
		void log(Level level, const char* format, va_list args);
		void trace(const char* format, ...);
		void debug(const char* format, ...);
		void info(const char* format, ...);
		void error(const char* format, ...);

		/**
		 * Sets the threshold to 'level'.
		 *
		 * Logging message than are less severe than the specified level are ignored.
		*/
		void set_level(Level level);
		
		/**
		 * Returns the current level.
		*/
		inline Level get_level() const { return _level; }

		/** 
		 * Checks if the specified level is more severe than the current level
		*/
		inline bool is_enabled(Level level) const { return level >= _level; }

		/**
		 * Checks if the corresponding level is enabled.
		*/
		inline bool is_info_enabled() const { return is_enabled(Level::LL_INFO); }
		inline bool is_debug_enabled() const { return is_enabled(Level::LL_DEBUG); }
		inline bool is_trace_enabled() const { return is_enabled(Level::LL_TRACE); }

		/**
		 * Adds a writer to this logger.
		*/
		void add_writer(LogWriter* writer);

		/**
		 * Removes a writer from this logger.
		*/
		void remove_writer(LogWriter* writer);

		/**
		 * Creates or returns the instance of the logger.
		 * 
		 * Warning: this function is not thread safe.
		*/
		static Logger* get_logger();

	private:
		Logger();

		/**
		 * Writes a message to the log writers.
		*/
		void write(Logger::Level level, const std::string& text);

	private:
		// A reference to the application logger (singleton).
		static Logger* _logger;

		// A list of writers.
		std::list<LogWriter *> _writers;

		// A mutex to protect access to the list of writers.
		tools::Mutex _mutex;

		// The current logger level.
		Level _level;
	};


	/**
	 * An abstract log writer.
	*/
	class LogWriter
	{
	public:
		virtual ~LogWriter() = default;

		virtual void write(Logger::Level level, const std::string& text) = 0;
		virtual void flush() { return; }
	};


	/**
	 * A file log writer.
	*/
	class FileLogWriter final: public LogWriter
	{
	public:
		explicit FileLogWriter();
		~FileLogWriter() override;

		bool open(const std::wstring& filename);
		void write(Logger::Level level, const std::string& text) override;
		void flush() override;

	private:
		std::ofstream _ofs;
	};


	class LogQueue final
	{
	public:
		explicit LogQueue();

		inline size_t size() const { return _queue.size(); }
		void push(const std::string& text);
		std::string pop();

		inline tools::Mutex& mutex() { return _mutex; }

	private:
		std::queue<std::string> _queue;
		tools::Mutex _mutex;

	};

}
