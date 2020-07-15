#pragma once

#include "dllexport.hpp"
#include "param_string.hpp"
#include "g6_attributes.hpp"
#include "fmt/format.h"

#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

namespace glasssix::exposing::allocations
{
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void* G6_ABI_CALL get_current_exception_what() noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void G6_ABI_CALL clear_current_exception_what() noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void G6_ABI_CALL set_current_exception_what(void* what_abi) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES std::int32_t G6_ABI_CALL set_current_exception_what_from_abi_result(std::int32_t code, const char* optional_inner_narrow_what = nullptr) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void* G6_ABI_CALL create_error_message_from_abi_result(std::int32_t code, void* optional_inner_what_abi = nullptr) noexcept;
}

namespace glasssix::exposing
{
	/// <summary>
	/// Standard codes of the return value of an ABI function.
	/// </summary>
	struct abi_result
	{
		std::int32_t code;

		constexpr abi_result() noexcept : code{}
		{
		}

		constexpr abi_result(std::int32_t code) noexcept : code{ code }
		{
		}

		constexpr operator std::int32_t() const noexcept
		{
			return code;
		}

		constexpr bool operator==(abi_result& right) const noexcept
		{
			return code == right.code;
		}

		constexpr bool operator!=(abi_result& right) const noexcept
		{
			return code != right.code;
		}

		constexpr bool no_error() const noexcept
		{
			return code >= 0;
		}
	};

	inline constexpr abi_result error_success{ 0 };
	inline constexpr abi_result error_success_false{ 1 };
	inline constexpr abi_result error_failure{ -1 };
	inline constexpr abi_result error_not_implemented{ -2 };
	inline constexpr abi_result error_null_pointer{ -3 };
	inline constexpr abi_result error_invalid_argument{ -4 };
	inline constexpr abi_result error_out_of_bounds{ -5 };
	inline constexpr abi_result error_no_interface{ -6 };
	inline constexpr abi_result error_invalid_operation{ -7 };
	inline constexpr abi_result error_key_not_found{ -8 };
	inline constexpr abi_result error_bad_alloc{ -9 };
	inline constexpr abi_result error_not_initialized{ -10 };

	/// <summary>
	/// Converts a boolean to a ABI result code.
	/// </summary>
	/// <param name="value">The boolean value</param>
	/// <returns>The result code</returns>
	inline abi_result to_abi_result(bool value) noexcept
	{
		return value ? error_success : error_success_false;
	}

	/// <summary>
	/// An ABI exception.
	/// </summary>
	class abi_error
	{
	public:
		abi_error(abi_result result) noexcept : result_{ result }, what_{ take_over_abi_from_void_ptr{ allocations::create_error_message_from_abi_result(result) } }
		{
		}

		abi_error(abi_result result, utf8_string_view inner_what) noexcept : result_{ result }, what_{ take_over_abi_from_void_ptr{ allocations::create_error_message_from_abi_result(result, get_abi(inner_what)) } }
		{
		}

		abi_error(abi_result result, void* what_abi) noexcept : result_{ result }, what_{ create_from_abi<param_string>(what_abi) }
		{
		}

		abi_result result() const noexcept
		{
			return result_;
		}

		param_string what() const noexcept
		{
			return what_;
		}

		std::string what_to_narrow() const noexcept
		{
			return to_narrow_string(what_);
		}
	private:
		abi_result result_;
		param_string what_;
	};

	struct abi_failure : abi_error
	{
		abi_failure() noexcept : abi_error{ error_failure }
		{
		}

		abi_failure(utf8_string_view inner_what) noexcept : abi_error{ error_failure, inner_what }
		{
		}

		abi_failure(void* what_abi) noexcept : abi_error{ error_failure, what_abi }
		{
		}
	};

	struct abi_not_implemented : abi_error
	{
		abi_not_implemented() noexcept : abi_error{ error_not_implemented }
		{
		}

		abi_not_implemented(utf8_string_view inner_what) noexcept : abi_error{ error_not_implemented, inner_what }
		{
		}

		abi_not_implemented(void* what_abi) noexcept : abi_error{ error_not_implemented, what_abi }
		{
		}
	};

	struct abi_null_pointer : abi_error
	{
		abi_null_pointer() noexcept : abi_error{ error_null_pointer }
		{
		}

		abi_null_pointer(utf8_string_view inner_what) noexcept : abi_error{ error_null_pointer, inner_what }
		{
		}

		abi_null_pointer(void* what_abi) noexcept : abi_error{ error_null_pointer, what_abi }
		{
		}
	};

	struct abi_invalid_argument : abi_error
	{
		abi_invalid_argument() noexcept : abi_error{ error_invalid_argument }
		{
		}

		abi_invalid_argument(utf8_string_view inner_what) noexcept : abi_error{ error_invalid_argument, inner_what }
		{
		}

		abi_invalid_argument(void* what_abi) noexcept : abi_error{ error_invalid_argument, what_abi }
		{
		}
	};

	struct abi_out_of_bounds : abi_error
	{
		abi_out_of_bounds() noexcept : abi_error{ error_out_of_bounds }
		{
		}

		abi_out_of_bounds(utf8_string_view inner_what) noexcept : abi_error{ error_out_of_bounds, inner_what }
		{
		}

		abi_out_of_bounds(void* what_abi) noexcept : abi_error{ error_out_of_bounds, what_abi }
		{
		}
	};

	struct abi_no_interface : abi_error
	{
		abi_no_interface() noexcept : abi_error{ error_no_interface }
		{
		}

		abi_no_interface(utf8_string_view inner_what) noexcept : abi_error{ error_no_interface, inner_what }
		{
		}

		abi_no_interface(void* what_abi) noexcept : abi_error{ error_no_interface, what_abi }
		{
		}
	};

	struct abi_invalid_operation : abi_error
	{
		abi_invalid_operation() noexcept : abi_error{ error_invalid_operation }
		{
		}

		abi_invalid_operation(utf8_string_view inner_what) noexcept : abi_error{ error_invalid_operation, inner_what }
		{
		}

		abi_invalid_operation(void* what_abi) noexcept : abi_error{ error_invalid_operation, what_abi }
		{
		}
	};

	struct abi_key_not_found : abi_error
	{
		abi_key_not_found() noexcept : abi_error{ error_key_not_found }
		{
		}

		abi_key_not_found(utf8_string_view inner_what) noexcept : abi_error{ error_key_not_found, inner_what }
		{
		}

		abi_key_not_found(void* what_abi) noexcept : abi_error{ error_key_not_found, what_abi }
		{
		}
	};

	struct abi_bad_alloc : abi_error
	{
		abi_bad_alloc() noexcept : abi_error{ error_bad_alloc }
		{
		}

		abi_bad_alloc(utf8_string_view inner_what) noexcept : abi_error{ error_bad_alloc, inner_what }
		{
		}

		abi_bad_alloc(void* what_abi) noexcept : abi_error{ error_bad_alloc, what_abi }
		{
		}
	};

	struct abi_not_initialized : abi_error
	{
		abi_not_initialized() noexcept : abi_error{ error_not_initialized }
		{
		}

		abi_not_initialized(utf8_string_view inner_what) noexcept : abi_error{ error_not_initialized, inner_what }
		{
		}

		abi_not_initialized(void* what_abi) noexcept : abi_error{ error_not_initialized, what_abi }
		{
		}
	};

	/// <summary>
	/// Catches the current exception and translates it into an ABI result code.
	/// </summary>
	/// <returns>The ABI result code</returns>
	inline abi_result translate_current_exception() noexcept
	{
		try
		{
			throw;
		}
		catch (const std::bad_alloc& ex)
		{
			return allocations::set_current_exception_what_from_abi_result(error_bad_alloc, ex.what());
		}
		catch (const std::out_of_range& ex)
		{
			return allocations::set_current_exception_what_from_abi_result(error_out_of_bounds, ex.what());
		}
		catch (const std::invalid_argument& ex)
		{
			return allocations::set_current_exception_what_from_abi_result(error_invalid_argument, ex.what());
		}
		catch (const fmt::format_error& ex)
		{
			return allocations::set_current_exception_what_from_abi_result(error_invalid_argument, ex.what());
		}
		catch (const std::exception& ex)
		{
			return allocations::set_current_exception_what_from_abi_result(error_failure, ex.what());
		}
		catch (const abi_error& ex)
		{
			return (allocations::set_current_exception_what(detach_abi(ex.what())), ex.result());
		}
	}

	/// <summary>
	/// Invokes a call and forwards all the exceptions.
	/// </summary>
	/// <typeparam name="Callable">The callable type</typeparam>
	/// <param name="handler">The handler</param>
	/// <returns>The ABI result code</returns>
	template<typename Callable>
	abi_result abi_safe_call(Callable&& handler) noexcept try
	{
		if constexpr (std::is_convertible_v<decltype(std::declval<Callable>()()), abi_result>)
		{
			auto result = std::forward<Callable>(handler)();

			return (std::forward<Callable>(handler)(), result);
		}
		else
		{
			return (std::forward<Callable>(handler)(), allocations::clear_current_exception_what(), error_success);
		}
	}
	catch (...)
	{
		return translate_current_exception();
	}

	/// <summary>
	/// Rethrows the exception across the DLL boundary.
	/// </summary>
	/// <typeparam name="Exception">The exception type</typeparam>
	template<typename Exception, typename = std::enable_if_t<std::is_base_of_v<abi_error, Exception>>>
	void rethrow_exception_across_boundary()
	{
		if (auto what_abi = allocations::get_current_exception_what())
		{
			throw Exception{ what_abi };
		}
		else
		{
			throw Exception{};
		}
	}

	/// <summary>
	/// Checks a result code and throws an exception if neccessary.
	/// </summary>
	/// <param name="result">The result code</param>
	inline void check_abi_result(abi_result result)
	{
		if (!result.no_error())
		{
			switch (result)
			{
			case error_failure:
				rethrow_exception_across_boundary<abi_failure>();
				break;
			case error_not_implemented:
				rethrow_exception_across_boundary<abi_not_implemented>();
				break;
			case error_null_pointer:
				rethrow_exception_across_boundary<abi_null_pointer>();
				break;
			case error_invalid_argument:
				rethrow_exception_across_boundary<abi_invalid_argument>();
				break;
			case error_out_of_bounds:
				rethrow_exception_across_boundary<abi_out_of_bounds>();
				break;
			case error_no_interface:
				rethrow_exception_across_boundary<abi_no_interface>();
				break;
			case error_invalid_operation:
				rethrow_exception_across_boundary<abi_invalid_operation>();
				break;
			case error_key_not_found:
				rethrow_exception_across_boundary<abi_key_not_found>();
				break;
			case error_bad_alloc:
				rethrow_exception_across_boundary<abi_bad_alloc>();
				break;
			case error_not_initialized:
				rethrow_exception_across_boundary<abi_not_initialized>();
				break;
			default:
				rethrow_exception_across_boundary<abi_failure>();
			}
		}
	}
}
