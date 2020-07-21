#pragma once
#ifndef _profiler_HPP_
#define _profiler_HPP_

#if defined(_MSC_VER) && _MSC_VER <= 1800
#include <Windows.h>
#else
#include <chrono>
#endif  // _MSC_VER

#include <fstream>
#include <vector>
#include <string>
#include <memory>

#include "logger.hpp"

namespace glasssix
{
	static void profiler_write_event(std::ofstream &file,
		const char *name, const char *ph, uint64_t ts)
	{
		file << "    {" << std::endl;
		file << "      \"name\": \"" << name << "\"," << std::endl;
		file << "      \"cat\": \"category\"," << std::endl;
		file << "      \"ph\": \"" << ph << "\"," << std::endl;
		file << "      \"ts\": " << ts << "," << std::endl;
		file << "      \"pid\": 0," << std::endl;
		file << "      \"tid\": 0" << std::endl;
		file << "    }";
	}

	/*!
	 * \brief profiler for glasssix, don't enable profiler in Multi-thread Env
	 *  This class is used to profile a range of source code as a scope.
	 *  The basic usage is like below.
	 *
	 * ```
	 * profiler *profiler = profiler::get();
	 * profiler->scope_start("scope1");
	 * ...
	 * ...
	 * profiler->scope_end();
	 * ```
	 *
	 * scope represents a range of source code. Nested scope is also supported.
	 * Dump profile into a json file, then we can view the data from google chrome
	 * in chrome://tracing/
	 */
	class profiler
	{
	public:
		/*! \brief get global instance */
		static profiler *get()
		{
			static profiler inst;
			return &inst;
		}
		/*!
		 * \brief start a scope
		 * \param name scope name
		 */
		void scope_start(const char *name)
		{
			if (state_ == not_running) return;
			scope_ptr new_scope = std::make_shared<scope>();
			if (!scope_stack_.empty())
			{
				new_scope->name = scope_stack_.back()->name + ":" + name;
			}
			else
			{
				new_scope->name = name;
			}
			new_scope->start_microsec = now() - init_;
			scope_stack_.push_back(new_scope);
		}
		/*!
		 * \brief end a scope
		 */
		void scope_end()
		{
			if (state_ == not_running) return;
			CHECK(!scope_stack_.empty());
			scope_ptr current_scope = scope_stack_.back();
			current_scope->end_microsec = now() - init_;
			scopes_.push_back(current_scope);
			// pop stack
			scope_stack_.pop_back();
		}
		/*!
		 * \brief dump profile data
		 * \param fn file name
		 */
		void DumpProfile(const char *fn) const
		{
			CHECK(scope_stack_.empty());
			CHECK_EQ(state_, not_running);

			std::ofstream file;
			file.open(fn);
			file << "{" << std::endl;
			file << "  \"traceEvents\": [";

			bool is_first = true;
			for (auto scope : scopes_)
			{
				if (is_first)
				{
					file << std::endl;
					is_first = false;
				}
				else
				{
					file << "," << std::endl;
				}
				profiler_write_event(file, scope->name.c_str(), "B", scope->start_microsec);
				file << "," << std::endl;
				profiler_write_event(file, scope->name.c_str(), "E", scope->end_microsec);
			}

			file << "  ]," << std::endl;
			file << "  \"displayTimeUnit\": \"ms\"" << std::endl;
			file << "}" << std::endl;
		}
		/*! \brief turn on profiler */
		void turn_on()
		{
			CHECK_EQ(state_, not_running) << "Profile is already running.";
			state_ = running;
		}
		/*! \brief turn off profiler */
		void turn_off()
		{
			CHECK_EQ(state_, running) << "Profile is not running.";
			CHECK(scope_stack_.empty()) << "Profile scope stack is not empty, with size = "
				<< scope_stack_.size();
			state_ = not_running;
		}
		/*! \brief timestamp, return in microseconds */
		uint64_t now() const
		{
#if defined(_MSC_VER) && _MSC_VER <= 1800
			LARGE_INTEGER frequency, counter;
			QueryPerformanceFrequency(&frequency);
			QueryPerformanceCounter(&counter);
			return counter.QuadPart * 1000000 / frequency.QuadPart;
#else
			return std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif  // _MSC_VER
		}

	private:
		profiler() : init_(now()), state_(not_running)
		{
			scope_stack_.reserve(10);
			scopes_.reserve(1024);
		}

		DISABLE_COPY_AND_ASSIGN(profiler);

	private:
		enum state
		{
			running,
			not_running,
		};
		struct scope
		{
			std::string name;
			uint64_t start_microsec = 0;
			uint64_t end_microsec = 0;
		};
		typedef std::shared_ptr<scope> scope_ptr;
		/*! \brief scope stack for nested scope */
		std::vector<scope_ptr> scope_stack_;
		/*! \brief all scopes used in profile, not including scopes in stack */
		std::vector<scope_ptr> scopes_;
		/*! \brief init timestamp */
		uint64_t init_;
		/*! \brief profile state */
		state state_;
	};  // class profiler

	class timer
	{
	public:
		timer() : start_(), end_()
		{
		}

		void start()
		{
			start_ = std::chrono::system_clock::now();
		}

		void stop()
		{
			end_ = std::chrono::system_clock::now();
		}

		double get_elapsed_seconds() const
		{
			return (double)std::chrono::duration_cast<std::chrono::seconds>(end_ - start_).count();
		}

		double get_elapsed_milli_seconds() const
		{
			return (double)std::chrono::duration_cast<std::chrono::milliseconds>(end_ - start_).count();
		}

		double get_elapsed_micro_seconds() const
		{
			return (double)std::chrono::duration_cast<std::chrono::microseconds>(end_ - start_).count();
		}

		double get_elapsed_nano_seconds() const
		{
			return (double)std::chrono::duration_cast<std::chrono::nanoseconds>(end_ - start_).count();
		}

	private:
		std::chrono::time_point<std::chrono::system_clock> start_;
		std::chrono::time_point<std::chrono::system_clock> end_;
	};
}  // namespace
#endif // !_profiler_HPP_