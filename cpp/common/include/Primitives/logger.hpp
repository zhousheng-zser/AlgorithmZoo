#pragma once
#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include "compiler.hpp"
#include "fmt/format.h"
#include "dllexport.hpp"

#include <mutex>
#include <ctime>
#include <chrono>
#include <sstream>
#include <iostream>
#include <exception>
#include <string_view>
#include <unordered_map>

#ifdef _WIN32
#define localtime_r(a, b) localtime_s(b, a)
#endif

namespace glasssix
{
	extern "C" std::uint32_t EXPORT_EXCALIBUR_PRIMITIVES get_current_thread_id() noexcept;

	enum class log_level
	{ 
		INFO,
		WARNING,
		ERROR,
		FATAL
	};

	namespace
	{
		std::mutex log_mutex;
		const std::unordered_map<enum log_level, const char*> log_level_string_map =
		{
			{ log_level::INFO, "INFO" },
			{ log_level::WARNING, "WARNING" },
			{ log_level::ERROR, "ERROR" },
			{ log_level::FATAL, "FATAL" },
		};
	}

	class logger_base
	{
	public:
		class log_stream : public std::ostringstream
		{
		public:
			log_stream(logger_base& logger, const char* file, int line, log_level level) : line_{ line }, level_{ level }, logger_{ logger }, file_{ file }
			{
			};

			log_stream(logger_base& logger, log_level level) : log_stream{ logger, nullptr, 0, level }
			{
			};

			log_stream(const log_stream& other) : line_{ other.line_ }, level_{ other.level_ }, logger_{ other.logger_ }, file_{ other.file_ }
			{
			};

			~log_stream()
			{
				logger_.endline(file_, line_, level_, str().c_str());
			}
		private:
			int line_;
			log_level level_;
			logger_base& logger_;
			const char* file_;
		};

		virtual ~logger_base()
		{
		}

		auto operator()(const char* file, int line, enum log_level level = log_level::INFO)
		{
			return log_stream{ *this, file, line, level };
		}

		auto operator()(enum log_level level = log_level::INFO)
		{
			return log_stream{ *this, level };
		}
	protected:
		virtual void output(const tm& time, const char* level, const char* message) = 0;
		virtual void output(const tm& time, const char* level, const char* file, int line, const char* message) = 0;
	private:
		const tm& get_local_time()
		{
			thread_local std::tm local_time;
			auto now = std::chrono::system_clock::now();
			auto timestamp = std::chrono::system_clock::to_time_t(now);

			return (localtime_r(&timestamp, &local_time), local_time);
		}

		void endline(const char* file, int line, log_level level, const char* message)
		{
			std::lock_guard<std::mutex> lock{ log_mutex };
			
			if (file)
			{
				output(get_local_time(), log_level_string_map.find(level)->second, file, line, message);
			}
			else
			{
				output(get_local_time(), log_level_string_map.find(level)->second, message);
			}

			if (level == log_level::FATAL)
			{
				std::terminate();
			}
		}
	};

	class standard_output_logger : public logger_base
	{
		using logger_base::logger_base;

		virtual void output(const tm& time, const char* level, const char* message) override
		{
			std::cout << fmt::format(FMT_STRING("[{:04}-{:02}-{:02} {:02}:{:02}:{:02} {:5}][{}] {}"),
				1900 + time.tm_year,
				time.tm_mon + 1,
				time.tm_mday,
				time.tm_hour,
				time.tm_min,
				time.tm_sec,
				get_current_thread_id(),
				level,
				message
			) << std::endl;
		}

		virtual void output(const tm& time, const char* level, const char* file, int line, const char* message) override
		{
			std::cout << fmt::format(FMT_STRING("[{:04}-{:02}-{:02} {:02}:{:02}:{:02} {:5} {}:{}][{}] {}"),
				1900 + time.tm_year,
				time.tm_mon + 1,
				time.tm_mday,
				time.tm_hour,
				time.tm_min,
				time.tm_sec,
				get_current_thread_id(),
				file,
				line,
				level,
				message
			) << std::endl;
		}
	};

#define LOG_ND(level) glasssix::standard_output_logger{}(glasssix::log_level::level)
#define LOG(level) glasssix::standard_output_logger{}(__FILE__, __LINE__, glasssix::log_level::level)
#define LOG_IF(level, condition) if(condition) LOG(level)

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
#define DLOG(level) while (false) LOG(level) 
#else
#define DCHECK(x) CHECK(x)
#define DCHECK_LT(x, y) CHECK_LT(x, y)
#define DCHECK_GT(x, y) CHECK_GT(x, y)
#define DCHECK_LE(x, y) CHECK_LE(x, y)
#define DCHECK_GE(x, y) CHECK_GE(x, y)
#define DCHECK_EQ(x, y) CHECK_EQ(x, y)
#define DCHECK_NE(x, y) CHECK_NE(x, y)
#define DLOG(level) LOG(level) 
#endif  // NDEBUG

	// A simple macro to mark codes that are not implemented, so that when the code
// is executed we will see a fatal log.
#define NOT_IMPLEMENTED LOG(FATAL) << "Not Implemented Yet."
#define NO_GPU LOG(FATAL) << "Cannot use GPU in CPU-only Mode: check mode."
#define DEPRECATED LOG(FATAL) << "Module has already been deprecated. Transfering to new module is required."
// Disable the copy and assignment operator for a class.
#define DISABLE_COPY_AND_ASSIGN(classname)\
private:\
  classname(const classname&) = delete;\
  classname(classname&&) = delete;\
  classname& operator=(const classname&) = delete;\
  classname& operator=(classname&&) = delete
}

#endif
