#pragma once
#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <map>
#include <ctime>
#include <chrono>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>

#include "compiler.hpp"

#ifdef G6_CXX11
// For C++/CLI, we use a workaround.
#if defined(_MSC_VER) && defined(__cplusplus_cli)
#include "mutex_wrapper.hpp"
#else // C++/CLI for .Net
#include <mutex>
#endif // C++/Native
#else
#error "C++11 (or higher) support not detected! (Is `-std=c++11' or '/std:14' missing?)"
#endif // !G6_CXX11


#ifdef _MSC_VER
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#undef ERROR
#define GetTID GetCurrentThreadId
#define localtime_r(a, b) localtime_s(b, a)
#elif defined(__linux__)
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#define GetTID pthread_self 
#endif

namespace glasssix
{
#if defined(_MSC_VER) && defined(__cplusplus_cli)
	static mutex_wrapper log_mutex;
#else
	static std::mutex log_mutex;
#endif

	static const char* const_basename(const char* filepath)
	{
		const char* base = strrchr(filepath, '/');
#ifdef _MSC_VER
		if (!base)
			base = strrchr(filepath, '\\');
#endif
		return base ? (base + 1) : filepath;
	}

	enum LogLevel { GLASSSIX_LOG_INFO, GLASSSIX_LOG_WARNING, GLASSSIX_LOG_ERROR, GLASSSIX_LOG_FATAL };
	static const std::map<enum LogLevel, const char *> LevelStr =
	{
		{ LogLevel::GLASSSIX_LOG_INFO, "INFO" },
		{ LogLevel::GLASSSIX_LOG_WARNING, "WARNING" },
		{ LogLevel::GLASSSIX_LOG_ERROR, "ERROR" },
		{ LogLevel::GLASSSIX_LOG_FATAL, "FATAL" },
	};


	class BaseLogger
	{
		class LogStream : public std::ostringstream
		{
			BaseLogger& m_oLogger;
			enum LogLevel m_nLevel;
			const char *m_file;
			int m_line;
		public:
			LogStream(BaseLogger& oLogger, const char* file, int line, enum LogLevel nLevel)
				: m_oLogger(oLogger), m_nLevel(nLevel), m_file{ file }, m_line(line) {};
			LogStream(const LogStream& ls)
				: m_oLogger(ls.m_oLogger), m_nLevel(ls.m_nLevel), m_file(ls.m_file), m_line(ls.m_line) {};
			~LogStream()
			{
				m_oLogger.endline(m_file, m_line, m_nLevel, std::move(str()));
			}
		};
	public:
		BaseLogger() = default;
		virtual ~BaseLogger() = default;

		virtual LogStream operator()(const char* file, int line, enum LogLevel nLevel = LogLevel::GLASSSIX_LOG_INFO)
		{
			return LogStream(*this, file, line, nLevel);
		}
	private:
		const tm* getLocalTime()
		{
			auto now = std::chrono::system_clock::now();
			auto in_time_t = std::chrono::system_clock::to_time_t(now);
			localtime_r(&in_time_t, &_localTime);
			return &_localTime;
		}
		void endline(const char *file, int line, enum LogLevel nLevel, std::string&& oMessage)
		{
#if defined(_MSC_VER) && defined(__cplusplus_cli)
			auto lock = log_mutex.guard();
#else
			std::lock_guard<std::mutex> lock{ log_mutex };
#endif
			output(getLocalTime(), LevelStr.find(nLevel)->second, file, line, oMessage.c_str());
			if (nLevel == LogLevel::GLASSSIX_LOG_FATAL)
				abort();
		}
		virtual void output(const tm *p_tm,
			const char *str_level,
			const char* file, int line,
			const char *str_message) = 0;
	private:
		tm _localTime;
	};

	class ConsoleLogger : public BaseLogger
	{
		using BaseLogger::BaseLogger;
		virtual void output(const tm *p_tm,
			const char *str_level,
			const char* file, int line,
			const char *str_message)
		{
			std::cout << '[' << 1900 + p_tm->tm_year << '-'
				<< std::setfill('0') << std::setw(2) << p_tm->tm_mon + 1 << '-'
				<< std::setfill('0') << std::setw(2) << p_tm->tm_mday << ' '
				<< std::setfill('0') << std::setw(2) << p_tm->tm_hour << ':'
				<< std::setfill('0') << std::setw(2) << p_tm->tm_min << ':'
				<< std::setfill('0') << std::setw(2) << p_tm->tm_sec
				<< ' ' << std::setw(5) << GetTID() << std::setfill('0') << ' '
				<< const_basename(file) << ':' << line << ']'
				<< '[' << str_level << "]"
				<< ' ' << str_message << std::endl;
			std::cout.flush();
		}
	};

#define LOG(Level) glasssix::ConsoleLogger{}(__FILE__, __LINE__, glasssix::LogLevel::GLASSSIX_LOG_##Level)

	//#define LOG(Level) std::cout << "[" << glasssix::LevelStr.find(glasssix::GLASSSIX_LOG_##Level)->second << "] "

#define LOG_IF(Level, Condition) if(Condition) LOG(Level)

#define CHECK(a) \
if(!(a)) \
	LOG(FATAL) << "CHECK FAILED(" << #a << " = " << (a) << ") "

#define CHECK_BINARY_OP(name, op, a, b)  \
if(!((a) op (b))) \
LOG(FATAL) << "CHECK" << #name << " FAILED(" << #a << " " << #op << " " << #b << " vs. " << (a) << " " << #op << " " << (b) << ") "

#define CHECK_LT(x, y) CHECK_BINARY_OP(_LT, <, x, y)
#define CHECK_GT(x, y) CHECK_BINARY_OP(_GT, >, x, y)
#define CHECK_LE(x, y) CHECK_BINARY_OP(_LE, <=, x, y)
#define CHECK_GE(x, y) CHECK_BINARY_OP(_GE, >=, x, y)
#define CHECK_EQ(x, y) CHECK_BINARY_OP(_EQ, ==, x, y)
#define CHECK_NE(x, y) CHECK_BINARY_OP(_NE, !=, x, y)
#define CHECK_NOTNULL(x) \
 ((x) == NULL ? LOG(FATAL) << "Check  notnull: "  #x << ' ', (x) : (x)) 

// Debug-only checking.
#ifdef NDEBUG
#define DCHECK(x)  while (false) CHECK(x)
#define DCHECK_LT(x, y)  while (false) CHECK((x) < (y))
#define DCHECK_GT(x, y)  while (false) CHECK((x) > (y))
#define DCHECK_LE(x, y)  while (false) CHECK((x) <= (y))
#define DCHECK_GE(x, y)  while (false) CHECK((x) >= (y))
#define DCHECK_EQ(x, y)  while (false) CHECK((x) == (y))
#define DCHECK_NE(x, y)  while (false) CHECK((x) != (y))
#else
#define DCHECK(x) CHECK(x)
#define DCHECK_LT(x, y) CHECK_LT(x, y)
#define DCHECK_GT(x, y) CHECK_GT(x, y)
#define DCHECK_LE(x, y) CHECK_LE(x, y)
#define DCHECK_GE(x, y) CHECK_GE(x, y)
#define DCHECK_EQ(x, y) CHECK_EQ(x, y)
#define DCHECK_NE(x, y) CHECK_NE(x, y)
#endif  // NDEBUG

	// A simple macro to mark codes that are not implemented, so that when the code
// is executed we will see a fatal log.
#define NOT_IMPLEMENTED LOG(FATAL) << "Not Implemented Yet."
#define NO_GPU LOG(FATAL) << "Cannot use GPU in CPU-only Mode: check mode."
#define DEPRECATED LOG(FATAL) << "Module has already deprecated. Transfer to new module is required."
// Disable the copy and assignment operator for a class.
#define DISABLE_COPY_AND_ASSIGN(classname)\
private:\
  classname(const classname&) = delete;\
  classname(classname&&) = delete;\
  classname& operator=(const classname&) = delete;\
  classname& operator=(classname&&) = delete
}

#endif