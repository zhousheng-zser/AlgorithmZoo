#pragma once

#include "dllexport.hpp"
#include "g6_attributes.hpp"
#include "param_string_allocations.hpp"

#include "fmt/format.h"

#include <cstdint>
#include <utility>
#include <optional>
#include <stdexcept>
#include <type_traits>

namespace glasssix::exposing::allocations
{
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void* G6_ABI_CALL get_abi_exception_what() noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void G6_ABI_CALL clear_abi_exception_what() noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void G6_ABI_CALL set_abi_exception_what(void* what_abi) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void* G6_ABI_CALL create_abi_exception_message(std::int32_t code, void* optional_inner_what_abi = nullptr) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES std::int32_t G6_ABI_CALL set_abi_exception_what_ex(std::int32_t code, void* optional_inner_what_abi = nullptr) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES std::int32_t G6_ABI_CALL set_abi_exception_what_ex_narrow(std::int32_t code, const char* optional_inner_narrow_what = nullptr) noexcept;
}

#define IMPLEMENT_ABI_ERROR(name, code) struct name : glasssix::exposing::abi_error_impl<code>{ using abi_error_impl::abi_error_impl; }

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
		abi_error(abi_result result) noexcept : result_{ result }, what_{ take_over_abi_from_void_ptr{ allocations::create_abi_exception_message(result) } }
		{
		}

		abi_error(abi_result result, utf8_string_view inner_what) noexcept : result_{ result }, what_{ take_over_abi_from_void_ptr{ allocations::create_abi_exception_message(result, get_abi(basic_param_string{ inner_what })) } }
		{
		}

		abi_error(abi_result result, void* what_abi) noexcept : result_{ result }, what_{ create_from_abi<basic_param_string>(what_abi) }
		{
		}

		abi_result result() const noexcept
		{
			return result_;
		}

		basic_param_string what() const noexcept
		{
			return what_;
		}

		std::string what_to_narrow() const noexcept
		{
			return to_narrow_string(what_);
		}
	private:
		abi_result result_;
		basic_param_string what_;
	};

	template<std::int32_t code>
	struct abi_error_impl : abi_error
	{
		abi_error_impl() noexcept : abi_error{ code }
		{
		}

		template<typename T, typename = std::enable_if_t<std::is_convertible_v<T, utf8_string_view>>>
		abi_error_impl(T&& inner_what) noexcept : abi_error{ code, std::forward<T>(inner_what) }
		{
		}

		abi_error_impl(void* what_abi) noexcept : abi_error{ code, what_abi }
		{
		}
	};

	IMPLEMENT_ABI_ERROR(abi_failure, error_failure);
	IMPLEMENT_ABI_ERROR(abi_not_implemented, error_not_implemented);
	IMPLEMENT_ABI_ERROR(abi_null_pointer, error_null_pointer);
	IMPLEMENT_ABI_ERROR(abi_invalid_argument, error_invalid_argument);
	IMPLEMENT_ABI_ERROR(abi_out_of_bounds, error_out_of_bounds);
	IMPLEMENT_ABI_ERROR(abi_no_interface, error_no_interface);
	IMPLEMENT_ABI_ERROR(abi_invalid_operation, error_invalid_operation);
	IMPLEMENT_ABI_ERROR(abi_key_not_found, error_key_not_found);
	IMPLEMENT_ABI_ERROR(abi_bad_alloc, error_bad_alloc);
	IMPLEMENT_ABI_ERROR(abi_not_initialized, error_not_initialized);

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
			return allocations::set_abi_exception_what_ex_narrow(error_bad_alloc, ex.what());
		}
		catch (const std::out_of_range& ex)
		{
			return allocations::set_abi_exception_what_ex_narrow(error_out_of_bounds, ex.what());
		}
		catch (const std::invalid_argument& ex)
		{
			return allocations::set_abi_exception_what_ex_narrow(error_invalid_argument, ex.what());
		}
		catch (const fmt::format_error& ex)
		{
			return allocations::set_abi_exception_what_ex_narrow(error_invalid_argument, ex.what());
		}
		catch (const std::exception& ex)
		{
			return allocations::set_abi_exception_what_ex_narrow(error_failure, ex.what());
		}
		catch (const abi_error& ex)
		{
			return (allocations::set_abi_exception_what(detach_abi(ex.what())), ex.result());
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
			return std::forward<Callable>(handler)();
		}
		else
		{
			return (std::forward<Callable>(handler)(), error_success);
		}
	}
	catch (...)
	{
		return translate_current_exception();
	}

	/// Rethrows the exception across the DLL boundary.
	/// <summary>
	/// </summary>
	/// <typeparam name="Exception">The exception type</typeparam>
	template<typename Exception, typename = std::enable_if_t<std::is_base_of_v<abi_error, Exception>>>
	void rethrow_exception_across_boundary()
	{
		if (auto what_abi = allocations::get_abi_exception_what())
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
