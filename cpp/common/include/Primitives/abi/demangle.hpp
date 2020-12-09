#pragma once

#include "dllexport.hpp"
#include "param_string.hpp"
#include "g6_attributes.hpp"

#include <cstddef>
#include <string_view>

namespace glasssix::exposing::allocations
{
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void* G6_ABI_CALL demangle_cxx_abi_name_narrow(const char* name) noexcept;
}

namespace glasssix::exposing
{
	inline param_string demangle_cxx_abi_name(std::string_view name) noexcept
	{
		return param_string{ take_over_abi_from_void_ptr{ allocations::demangle_cxx_abi_name_narrow(name.data()) } };
	}

	/// <summary>
	/// Gets the friendly name of T.
	/// </summary>
	template<typename T>
	inline const auto name_of_v = demangle_cxx_abi_name(typeid(T).name());
}
