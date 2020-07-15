#pragma once

#include "base.hpp"
#include "guid.hpp"
#include "base_abi.hpp"

namespace glasssix::exposing::impl
{
	template<> inline constexpr auto& name_v<bool> = "b1";
	template<> inline constexpr auto& name_v<std::int8_t> = "i1";
	template<> inline constexpr auto& name_v<std::int16_t> = "i2";
	template<> inline constexpr auto& name_v<std::int32_t> = "i4";
	template<> inline constexpr auto& name_v<std::int64_t> = "i8";
	template<> inline constexpr auto& name_v<std::uint8_t> = "u1";
	template<> inline constexpr auto& name_v<std::uint16_t> = "u2";
	template<> inline constexpr auto& name_v<std::uint32_t> = "u4";
	template<> inline constexpr auto& name_v<std::uint64_t> = "u8";
	template<> inline constexpr auto& name_v<float> = "f4";
	template<> inline constexpr auto& name_v<double> = "f8";

	template<> struct type_identity<bool>
	{
		using type = type_identity_primitive;
	};

	template<> struct type_identity<std::int8_t>
	{
		using type = type_identity_primitive;
	};

	template<> struct type_identity<std::int16_t>
	{
		using type = type_identity_primitive;
	};

	template<> struct type_identity<std::int32_t>
	{
		using type = type_identity_primitive;
	};

	template<> struct type_identity<std::int64_t>
	{
		using type = type_identity_primitive;
	};

	template<> struct type_identity<std::uint8_t>
	{
		using type = type_identity_primitive;
	};

	template<> struct type_identity<std::uint16_t>
	{
		using type = type_identity_primitive;
	};

	template<> struct type_identity<std::uint32_t>
	{
		using type = type_identity_primitive;
	};

	template<> struct type_identity<std::uint64_t>
	{
		using type = type_identity_primitive;
	};

	template<> struct type_identity<float>
	{
		using type = type_identity_primitive;
	};

	template<> struct type_identity<double>
	{
		using type = type_identity_primitive;
	};
}
