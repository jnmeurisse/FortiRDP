/*!
* This file is part of FortiRDP
*
* Copyright (C) 2022 Jean-Noel Meurisse
* SPDX-License-Identifier: Apache-2.0
*
*/
#pragma once

#include <string>
#include <list>
#include <fstream>
#include "tools/Mutex.h"


#ifdef _DEBUG
#define DEBUG_CTOR(logger, clsname) if ((logger)->is_debug_enabled()) { \
								(logger)->debug("... %x ctor::%s", (uintptr_t)this, clsname); \
								}
#define DEBUG_DTOR(logger, clsname) if ((logger)->is_debug_enabled()) { \
								(logger)->debug("... %x dtor::%s", (uintptr_t)this, clsname); \
								}
#else
#define DEBUG_CTOR(logger, clsname)
#define DEBUG_DTOR(logger, clsname)
#endif

#define DEBUG_ENTER(logger, clsname, funname) if ((logger)->is_debug_enabled()) { \
								(logger)->debug("... %x enter %s::%s", (uintptr_t)this, clsname, funname); \
								}

#define TRACE_ENTER(logger, clsname, funname) if ((logger)->is_trace_enabled()) { \
								(logger)->trace(".... %x enter %s::%s", (uintptr_t)this, clsname, funname); \
								}

namespace tools {

	class LogWriter;

	/**
	* The application Logger.
	*/
	class Logger final
	{
	public:
		typedef enum Level { LL_TRACE = 1, LL_DEBUG = 2, LL_INFO = 3, LL_ERROR = 4 } Level;

		/* Logs a message
		*/
		void log(Level level, const std::string& text);
		void log(Level level, const std::wstring& text);
		void log(Level level, const char* format, ...);
		void log(Level level, const char* format, va_list args);
		void trace(const char* format, ...);
		void debug(const char* format, ...);
		void info(const char* format, ...);
		void error(const char* format, ...);

		/* Sets the threshold to 'level'. Logging message than are less severe than
		*  the specified level are ignored
		*/
		void set_level(Level level);
		
		/* Returns the current level
		*/
		inline Level get_level() const { return _level; }

		/* tests if the specified level is more severe than the current level
		*/
		inline bool is_enabled(Level level) const { return level >= _level; }

		/* tests if the corresponding level is enabled
		*/
		inline bool is_info_enabled() const { return is_enabled(Level::LL_INFO); }
		inline bool is_debug_enabled() const { return is_enabled(Level::LL_DEBUG); }
		inline bool is_trace_enabled() const { return is_enabled(Level::LL_TRACE); }

		/* adds a writer to this logger
		*/
		void add_writer(LogWriter* writer);

		/* removes a writer from this logger
		*/
		void remove_writer(LogWriter* writer);

		/* Creates or returns the instance of the logger. This function 
		 * is not thread safe.
		*/
		static Logger* get_logger();

	private:
		Logger();

		/* Writes a message to the log writers
		*/
		void write(Logger::Level level, const char* text);

		/* Formats an error message. The caller is responsible to free 
		*  the temporary buffer used to store the message.
		*/
		char* fmt(const char* format, va_list args);

	private:
		// a reference to the application logger (singleton)
		static Logger* _logger;

		// a list of writers 
		std::list<LogWriter *> _writers;

		// a mutex to protect access to the list of writers
		tools::Mutex _mutex;

		// current logger level
		Level _level;
	};


	/**
	 * An abstract log writer
	*/
	class LogWriter
	{
	public:
		explicit LogWriter() {}
		virtual ~LogWriter() {}

		virtual void write(Logger::Level level, const char* text) = 0;
		virtual void flush() { return; }
	};


	/**
	 * A file log writer
	*/
	class FileLogWriter final: public LogWriter
	{
	public:
		explicit FileLogWriter();
		virtual ~FileLogWriter();

		bool open(const std::wstring& filename);
		virtual void write(Logger::Level level, const char* text) override;
		virtual void flush() override;

	private:
		std::ofstream _ofs;
	};

}