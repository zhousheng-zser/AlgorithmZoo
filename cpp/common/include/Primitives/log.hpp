#pragma once

#include "dllexport.hpp"
#include "abi/consumer.hpp"
#include "abi/g6_attributes.hpp"

#include <utility>
#include <type_traits>

namespace glasssix
{
	/// <summary>
	/// Available log levels.
	/// </summary>
	enum class log_level : std::int32_t
	{
		/// <summary>
		/// None will be output.
		/// </summary>
		none,

		/// <summary>
		/// A message that helps debug the program and find bugs exactly.
		/// </summary>
		debug,

		/// <summary>
		/// A message that informs the consumer of some suggestive tips.
		/// </summary>
		info,

		/// <summary>
		/// A warning that is presented to the consumer.
		/// </summary>
		warning,

		/// <summary>
		/// A serious logic error occurs now and must be resolved immediately.
		/// </summary>
		error,

		/// <summary>
		/// A fatal error occurs unexpectedly and the program must be terminated.
		/// </summary>
		fatal
	};
}

namespace glasssix::logging
{
	struct log;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<logging::log>
	{
		using identity_type = type_identity_interface;
		static constexpr guid id{ "BAA262FF-5AF5-4217-853E-83AD5FBEC6C8" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL debug(abi_in_t<param_string> message) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL warning(abi_in_t<param_string> message) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL info(abi_in_t<param_string> level) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL error(abi_in_t<param_string> message) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL fatal(abi_in_t<param_string> message) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_log_level(abi_in_t<log_level> message) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, logging::log> : interface_vtable_base<Derived, logging::log>
	{
		virtual std::int32_t G6_ABI_CALL debug(abi_in_t<param_string> message) noexcept override
		{
			return abi_safe_call([&] { this->self().debug(create_from_abi<param_string>(message)); });
		}

		virtual std::int32_t G6_ABI_CALL info(abi_in_t<param_string> message) noexcept override
		{
			return abi_safe_call([&] { this->self().info(create_from_abi<param_string>(message)); });
		}

		virtual std::int32_t G6_ABI_CALL warning(abi_in_t<param_string> message) noexcept override
		{
			return abi_safe_call([&] { this->self().warning(create_from_abi<param_string>(message)); });
		}

		virtual std::int32_t G6_ABI_CALL error(abi_in_t<param_string> message) noexcept override
		{
			return abi_safe_call([&] { this->self().error(create_from_abi<param_string>(message)); });
		}

		virtual std::int32_t G6_ABI_CALL fatal(abi_in_t<param_string> message) noexcept override
		{
			return abi_safe_call([&] { this->self().fatal(create_from_abi<param_string>(message)); });
		}

		virtual std::int32_t G6_ABI_CALL set_log_level(abi_in_t<log_level> level) noexcept override
		{
			return abi_safe_call([&] { this->self().set_log_level(create_from_abi<log_level>(level)); });
		}
	};

	template<> struct abi_adapter<logging::log>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, logging::log>
		{
			void debug(const param_string& message) const
			{
				check_abi_result(this->self_abi().debug(get_abi(message)));
			}

			void info(const param_string& message) const
			{
				check_abi_result(this->self_abi().info(get_abi(message)));
			}

			void warning(const param_string& message) const
			{
				check_abi_result(this->self_abi().warning(get_abi(message)));
			}

			void error(const param_string& message) const
			{
				check_abi_result(this->self_abi().error(get_abi(message)));
			}

			void fatal(const param_string& message) const
			{
				check_abi_result(this->self_abi().fatal(get_abi(message)));
			}

			void set_log_level(log_level level) const
			{
				check_abi_result(this->self_abi().set_log_level(get_abi(level)));
			}
		};
	};
}

namespace glasssix::logging
{
	struct log : exposing::inherits<log>
	{
		using inherits::inherits;
	};
}

namespace glasssix
{
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void* glasssix_add_ref_get_logger_abi();

	/// <summary>
	/// Gets the application-wise logger.
	/// </summary>
	/// <returns>The logger</returns>
	inline logging::log get_logger() noexcept
	{
		return logging::log{ exposing::take_over_abi_from_void_ptr{ glasssix_add_ref_get_logger_abi() } };
	}

	/// <summary>
	/// A convenient facility for logging.
	/// </summary>
	struct log
	{
		/// <summary>
		/// Sets the current log level.
		/// </summary>
		/// <param name="level">The log level</param>
		static void set_log_level(log_level level)
		{
			get_logger().set_log_level(level);
		}

		/// <summary>
		/// Prints debugging information.
		/// </summary>
		/// <param name="message">The message</param>
		static void d(exposing::utf8_string_view message)
		{
			get_logger().debug(message);
		}

		/// <summary>
		/// Prints ordinary information.
		/// </summary>
		/// <param name="message">The message</param>
		static void i(exposing::utf8_string_view message)
		{
			get_logger().info(message);
		}

		/// <summary>
		/// Prints a warning.
		/// </summary>
		/// <param name="message">The message</param>
		static void w(exposing::utf8_string_view message)
		{
			get_logger().warning(message);
		}

		/// <summary>
		/// Prints an error.
		/// </summary>
		/// <param name="message">The message</param>
		static void e(exposing::utf8_string_view message)
		{
			get_logger().error(message);
		}

		/// <summary>
		/// Prints a fatal error.
		/// </summary>
		/// <param name="message">The message</param>
		static void f(exposing::utf8_string_view message)
		{
			get_logger().fatal(message);
		}

		/// <summary>
		/// Prints ordinary information.
		/// </summary>
		/// <param name="format">The format string</param>
		/// <param name="...args">The arguments</param>
		template<typename FormatString, typename... Args, typename = std::enable_if_t<sizeof...(Args) != 0>>
		static void i(FormatString&& format, Args&&... args)
		{
			i(exposing::format(std::forward<FormatString>(format), std::forward<Args>(args)...));
		}

		/// <summary>
		/// Prints a warning.
		/// </summary>
		/// <param name="format">The format string</param>
		/// <param name="...args">The arguments</param>
		template<typename FormatString, typename... Args, typename = std::enable_if_t<sizeof...(Args) != 0>>
		static void w(FormatString&& format, Args&&... args)
		{
			w(exposing::format(std::forward<FormatString>(format), std::forward<Args>(args)...));
		}

		/// <summary>
		/// Prints an error.
		/// </summary>
		/// <param name="format">The format string</param>
		/// <param name="...args">The arguments</param>
		template<typename FormatString, typename... Args, typename = std::enable_if_t<sizeof...(Args) != 0>>
		static void e(FormatString&& format, Args&&... args)
		{
			e(exposing::format(std::forward<FormatString>(format), std::forward<Args>(args)...));
		}

		/// <summary>
		/// Prints a fatal error.
		/// </summary>
		/// <param name="format">The format string</param>
		/// <param name="...args">The arguments</param>
		template<typename FormatString, typename... Args, typename = std::enable_if_t<sizeof...(Args) != 0>>
		static void f(FormatString&& format, Args&&... args)
		{
			f(exposing::format(std::forward<FormatString>(format), std::forward<Args>(args)...));
		}
	};
}
