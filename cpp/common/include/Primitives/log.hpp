#pragma once

#include "dllexport.hpp"
#include "log_level.hpp"
#include "source_location.hpp"
#include "char8_t_remediation.hpp"

#include "abi/consumer.hpp"
#include "abi/g6_attributes.hpp"

#include <utility>
#include <cstdint>
#include <exception>
#include <type_traits>

namespace glasssix::logging
{
	struct log;

	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void* G6_ABI_CALL glasssix_add_ref_get_logger_abi();
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void G6_ABI_CALL glasssix_set_log_debugging_info(const char* file, std::int32_t line);
}

namespace glasssix::exposing::impl
{
	template<> struct abi<logging::log>
	{
		using identity_type = type_identity_interface;
		static constexpr guid id{ "BAA262FF-5AF5-4217-853E-83AD5FBEC6C8" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> config_path) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_log_level(abi_in_t<log_level> message) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL debug(abi_in_t<param_string> message, abi_in_t<bool> including_debugging_info) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL warning(abi_in_t<param_string> message, abi_in_t<bool> including_debugging_info) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL info(abi_in_t<param_string> level, abi_in_t<bool> including_debugging_info) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL error(abi_in_t<param_string> message, abi_in_t<bool> including_debugging_info) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL fatal(abi_in_t<param_string> message, abi_in_t<bool> including_debugging_info) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, logging::log> : interface_vtable_base<Derived, logging::log>
	{
		virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> config_path) noexcept override
		{
			return abi_safe_call([&] { this->self().init(create_from_abi<param_string>(config_path)); });
		}

		virtual std::int32_t G6_ABI_CALL set_log_level(abi_in_t<log_level> level) noexcept override
		{
			return abi_safe_call([&] { this->self().set_log_level(create_from_abi<log_level>(level)); });
		}

		virtual std::int32_t G6_ABI_CALL debug(abi_in_t<param_string> message, abi_in_t<bool> including_debugging_info) noexcept override
		{
			return abi_safe_call([&] { this->self().debug(create_from_abi<param_string>(message), including_debugging_info); });
		}

		virtual std::int32_t G6_ABI_CALL info(abi_in_t<param_string> message, abi_in_t<bool> including_debugging_info) noexcept override
		{
			return abi_safe_call([&] { this->self().info(create_from_abi<param_string>(message), including_debugging_info); });
		}

		virtual std::int32_t G6_ABI_CALL warning(abi_in_t<param_string> message, abi_in_t<bool> including_debugging_info) noexcept override
		{
			return abi_safe_call([&] { this->self().warning(create_from_abi<param_string>(message), including_debugging_info); });
		}

		virtual std::int32_t G6_ABI_CALL error(abi_in_t<param_string> message, abi_in_t<bool> including_debugging_info) noexcept override
		{
			return abi_safe_call([&] { this->self().error(create_from_abi<param_string>(message), including_debugging_info); });
		}

		virtual std::int32_t G6_ABI_CALL fatal(abi_in_t<param_string> message, abi_in_t<bool> including_debugging_info) noexcept override
		{
			return abi_safe_call([&] { this->self().fatal(create_from_abi<param_string>(message), including_debugging_info); });
		}
	};

	template<> struct abi_adapter<logging::log>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, logging::log>
		{
			void init(const param_string& config_path) const
			{
				check_abi_result(this->self_abi().init(get_abi(config_path)));
			}

			void set_log_level(log_level level) const
			{
				check_abi_result(this->self_abi().set_log_level(get_abi(level)));
			}

			void debug(const param_string& message, bool including_debugging_info) const
			{
				check_abi_result(this->self_abi().debug(get_abi(message), get_abi(including_debugging_info)));
			}

			void info(const param_string& message, bool including_debugging_info) const
			{
				check_abi_result(this->self_abi().info(get_abi(message), get_abi(including_debugging_info)));
			}

			void warning(const param_string& message, bool including_debugging_info) const
			{
				check_abi_result(this->self_abi().warning(get_abi(message), get_abi(including_debugging_info)));
			}

			void error(const param_string& message, bool including_debugging_info) const
			{
				check_abi_result(this->self_abi().error(get_abi(message), get_abi(including_debugging_info)));
			}

			void fatal(const param_string& message, bool including_debugging_info) const
			{
				check_abi_result(this->self_abi().fatal(get_abi(message), get_abi(including_debugging_info)));
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

namespace glasssix::details
{
	struct string_view_with_source_location
	{
		exposing::utf8_string_view str;
		source_location location;

		template<typename String, typename std::enable_if_t<std::is_convertible_v<String, exposing::utf8_string_view>>* = nullptr>
		constexpr string_view_with_source_location(String&& str, const source_location& location = source_location::current()) noexcept
			: str{ std::forward<String>(str) }, location{ location }
		{
		}
	};

	/// <summary>
	/// Gets the application-wise logger.
	/// </summary>
	/// <returns>The logger</returns>
	inline logging::log get_logger()
	{
		return logging::log{ exposing::take_over_abi_from_void_ptr{ logging::glasssix_add_ref_get_logger_abi() } };
	}

	/// <summary>
	/// A convenient facility for logging.
	/// </summary>
	template<bool IncludingDebuggingInfo>
	class log
	{
	public:
		/// <summary>
		/// Sets the current log config file.
		/// </summary>
		/// <param name="level">The log level</param>
		static void init(exposing::utf8_string_view config_path)
		{
			get_logger().init(config_path);
		}

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
		/// <param name="format_or_message">Message or format string</param>
		/// <param name="...args">The arguments</param>
		template<typename... Args>
		static void d(const string_view_with_source_location& format_or_message, Args&&... args)
		{
			if constexpr (sizeof...(Args) == 0)
			{
				invoke_impl(format_or_message.location, [&] { get_logger().debug(format_or_message.str, IncludingDebuggingInfo); });
			}
			else
			{
				invoke_impl(format_or_message.location, [&] { get_logger().debug(exposing::format(format_or_message.str, std::forward<Args>(args)...), IncludingDebuggingInfo); });
			}
		}

		/// <summary>
		/// Prints ordinary information.
		/// </summary>
		/// <param name="format_or_message">Message or format string</param>
		/// <param name="...args">The arguments</param>
		template<typename... Args>
		static void i(const string_view_with_source_location& format_or_message, Args&&... args)
		{
			if constexpr (sizeof...(Args) == 0)
			{
				invoke_impl(format_or_message.location, [&] { get_logger().info(format_or_message.str, IncludingDebuggingInfo); });
			}
			else
			{
				invoke_impl(format_or_message.location, [&] { get_logger().info(exposing::format(format_or_message.str, std::forward<Args>(args)...), IncludingDebuggingInfo); });
			}
		}

		/// <summary>
		/// Prints a warning.
		/// </summary>
		/// <param name="format_or_message">Message or format string</param>
		/// <param name="...args">The arguments</param>
		template<typename... Args>
		static void w(const string_view_with_source_location& format_or_message, Args&&... args)
		{
			if constexpr (sizeof...(Args) == 0)
			{
				invoke_impl(format_or_message.location, [&] { get_logger().warning(format_or_message.str, IncludingDebuggingInfo); });
			}
			else
			{
				invoke_impl(format_or_message.location, [&] { get_logger().warning(exposing::format(format_or_message.str, std::forward<Args>(args)...), IncludingDebuggingInfo); });
			}
		}

		/// <summary>
		/// Prints an error.
		/// </summary>
		/// <param name="format_or_message">Message or format string</param>
		/// <param name="...args">The arguments</param>
		template<typename... Args>
		static void e(const string_view_with_source_location& format_or_message, Args&&... args)
		{
			if constexpr (sizeof...(Args) == 0)
			{
				invoke_impl(format_or_message.location, [&] { get_logger().error(format_or_message.str, IncludingDebuggingInfo); });
			}
			else
			{
				invoke_impl(format_or_message.location, [&] { get_logger().error(exposing::format(format_or_message.str, std::forward<Args>(args)...), IncludingDebuggingInfo); });
			}
		}

		/// <summary>
		/// Prints a fatal error.
		/// </summary>
		/// <param name="format_or_message">Message or format string</param>
		/// <param name="...args">The arguments</param>
		template<typename... Args>
		static void f(const string_view_with_source_location& format_or_message, Args&&... args)
		{
			if constexpr (sizeof...(Args) == 0)
			{
				invoke_impl(format_or_message.location, [&] { get_logger().fatal(format_or_message.str, IncludingDebuggingInfo); });
			}
			else
			{
				invoke_impl(format_or_message.location, [&] { get_logger().fatal(exposing::format(format_or_message.str, std::forward<Args>(args)...), IncludingDebuggingInfo); });
			}
		}
	private:
		template<typename Callable>
		static void invoke_impl(const source_location& location, Callable&& callable)
		{
			if constexpr (IncludingDebuggingInfo)
			{
				logging::glasssix_set_log_debugging_info(location.file.data(), location.line);
			}

			callable();
		}
	};
}

namespace glasssix
{
	/// <summary>
	/// A convenient facility for message or formattable logging.
	/// </summary>
	struct log : details::log<false> {};

	/// <summary>
	/// A convenient facility for message or formattable logging with debugging information.
	/// </summary>
	struct logd : details::log<true> {};

	/// <summary>
	/// Defines an assertion operation.
	/// std::terminate will be called if failed.
	/// </summary>
	template<typename T>
	struct assertion : T
	{
		template<typename... Args>
		void operator()(Args&&... args) const
		{
			if (!static_cast<const T&>(*this)(std::forward<Args>(args)...))
			{
				log::f(u8"Assertion failed: {}", exposing::name_of_v<T>);
			}
		}
	};

	template<typename T>
	assertion(T)->assertion<T>;

	struct is_true
	{
		template<typename T>
		bool operator()(T&& value) const
		{
			return static_cast<bool>(std::forward<decltype(value)>(value));
		}
	};

	struct is_false
	{
		template<typename T>
		bool operator()(T&& value) const
		{
			return !static_cast<bool>(std::forward<decltype(value)>(value));
		}
	};

	struct equal_to
	{
		template<typename T, typename U>
		bool operator()(T&& left, U&& right) const
		{
			if constexpr (std::is_convertible_v<T, exposing::utf8_string_view> && std::is_convertible_v<U, exposing::utf8_string_view>)
			{
				return exposing::utf8_string_view{ std::forward<T>(left) } == exposing::utf8_string_view{ std::forward<U>(right) };
			}
			else
			{
				return std::forward<T>(left) == std::forward<U>(right);
			}
		}
	};

	struct unequal_to
	{
		template<typename T, typename U>
		bool operator()(T&& left, U&& right) const
		{
			if constexpr (std::is_convertible_v<T, exposing::utf8_string_view> && std::is_convertible_v<U, exposing::utf8_string_view>)
			{
				return exposing::utf8_string_view{ std::forward<T>(left) } != exposing::utf8_string_view{ std::forward<U>(right) };
			}
			else
			{
				return std::forward<T>(left) != std::forward<U>(right);
			}
		}
	};

	struct is_nullptr
	{
		template<typename T>
		bool operator()(T&& value) const
		{
			return std::forward<T>(value) == nullptr;
		}
	};

	struct is_non_nullptr
	{
		template<typename T>
		bool operator()(T&& value) const
		{
			return std::forward<T>(value) != nullptr;
		}
	};

	struct less_than
	{
		template<typename T, typename U>
		bool operator()(T&& left, U&& right) const
		{
			return std::forward<T>(left) < std::forward<U>(right);
		}
	};

	struct greater_than
	{
		template<typename T, typename U>
		bool operator()(T&& left, U&& right) const
		{
			return std::forward<T>(left) > std::forward<U>(right);
		}
	};

	struct less_than_or_equal_to
	{
		template<typename T, typename U>
		bool operator()(T&& left, U&& right) const
		{
			return std::forward<T>(left) <= std::forward<U>(right);
		}
	};

	struct greater_than_or_equal_to
	{
		template<typename T, typename U>
		bool operator()(T&& left, U&& right) const
		{
			return std::forward<T>(left) >= std::forward<U>(right);
		}
	};
	
	/// <summary>
	/// Built-in assertion functions.
	/// </summary>
	struct assert
	{
		/// <summary>
		/// Asserts that a == true.
		/// </summary>
		inline static constexpr assertion<is_true> is_true{};

		/// <summary>
		/// Asserts that a == false.
		/// </summary>
		inline static constexpr assertion<is_false> is_false{};

		/// <summary>
		/// Asserts that a == nullptr;
		/// </summary>
		inline static constexpr assertion<is_nullptr> is_nullptr{};

		/// <summary>
		/// Asserts that a != nullptr;
		/// </summary>
		inline static constexpr assertion<is_non_nullptr> is_non_nullptr{};

		/// <summary>
		/// Asserts that a == b.
		/// </summary>
		inline static constexpr assertion<equal_to> eq{};

		/// <summary>
		/// Asserts that a != b.
		/// </summary>
		inline static constexpr assertion<unequal_to> ne{};

		/// <summary>
		/// Asserts that a &lt; b.
		/// </summary>
		inline static constexpr assertion<less_than> lt{};

		/// <summary>
		/// Asserts that a &gt; b.
		/// </summary>
		inline static constexpr assertion<greater_than> gt{};

		/// <summary>
		/// Asserts that a &lt;= b.
		/// </summary>
		inline static constexpr assertion<less_than_or_equal_to> le{};

		/// <summary>
		/// Asserts that a &gt;= b.
		/// </summary>
		inline static constexpr assertion<greater_than_or_equal_to> ge{};
	};
}
