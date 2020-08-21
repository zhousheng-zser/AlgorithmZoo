#pragma once

#include "guid.hpp"
#include "meta.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace glasssix::exposing::impl
{
	/// <summary>
	/// Stores the GUID of a type.
	/// </summary>
	template<typename T, typename = void>
	struct guid_storage;

	template<typename T>
	inline constexpr auto& guid_storage_v = guid_storage<T>::value;

	template<> struct guid_storage<guid>
	{
		static constexpr guid value{ "47534958-0000-0000-0000-000047554944" };
	};

	template<> struct guid_storage<bool>
	{
		static constexpr guid value{ "47534958-0000-0000-0000-0000424F4F4C" };
	};

	template<> struct guid_storage<std::int8_t>
	{
		static constexpr guid value{ "47534958-0000-0000-0000-0000494E5438" };
	};

	template<> struct guid_storage<std::int16_t>
	{
		static constexpr guid value{ "47534958-0000-0000-0000-00494E543136" };
	};

	template<> struct guid_storage<std::int32_t>
	{
		static constexpr guid value{ "47534958-0000-0000-0000-00494E543332" };
	};

	template<> struct guid_storage<std::int64_t>
	{
		static constexpr guid value{ "47534958-0000-0000-0000-00494E543634" };
	};

	template<> struct guid_storage<std::uint8_t>
	{
		static constexpr guid value{ "47534958-0000-0000-0000-0055494E5438" };
	};

	template<> struct guid_storage<std::uint16_t>
	{
		static constexpr guid value{ "47534958-0000-0000-0000-55494E543136" };
	};

	template<> struct guid_storage<std::uint32_t>
	{
		static constexpr guid value{ "47534958-0000-0000-0000-55494E543332" };
	};

	template<> struct guid_storage<std::uint64_t>
	{
		static constexpr guid value{ "47534958-0000-0000-0000-55494E543634" };
	};

	template<> struct guid_storage<float>
	{
		static constexpr guid value{ "47534958-0000-0000-0000-00464C4F4154" };
	};

	template<> struct guid_storage<double>
	{
		static constexpr guid value{ "47534958-0000-0000-0000-444F55424C45" };
	};
	
	/// <summary>
	/// A type identity.
	/// </summary>
	template<typename T, typename = void>
	struct type_identity;

	template<typename T>
	using type_identity_t = typename type_identity<T>::type;

	namespace details
	{
		template<typename T, typename = void>
		struct has_type_identity_top_level : std::false_type {};

		template<typename T>
		struct has_type_identity_top_level<T, std::void_t<type_identity_t<T>>> : std::true_type {};
	}

	template<typename T>
	struct has_type_identity : details::has_type_identity_top_level<T>{};
	
	template<template<typename...> typename T, typename... Args>
	struct has_type_identity<T<Args...>> : std::conjunction<details::has_type_identity_top_level<T<Args...>>, has_type_identity<Args>...>{};

	/// <summary>
	/// Checks whether a type identity exists recursively.
	/// </summary>
	template<typename T>
	inline constexpr bool has_type_identity_v = has_type_identity<T>::value;

	/// <summary>
	/// C++ primitive types.
	/// </summary>
	struct type_identity_primitive;

	/// <summary>
	/// Public interfaces.
	/// </summary>
	struct type_identity_interface;

	/// <summary>
	/// Delegates.
	/// </summary>
	struct type_identity_delegate;

	/// <summary>
	/// Enumerations.
	/// </summary>
	struct type_identity_enum;

	/// <summary>
	/// Tables.
	/// </summary>
	template<typename... Fields>
	struct type_identity_table;

	/// <summary>
	/// Generic public interfaces.
	/// </summary>
	struct type_identity_generic_interface;

	template<typename T>
	struct is_primitive : meta::is_same_any<T, guid, bool, std::int8_t, std::int16_t, std::int32_t, std::int64_t, std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t, std::intptr_t, std::uintptr_t, float, double>{};
	
	/// <summary>
	/// Checks whether a type is a primitive type.
	/// </summary>
	template<typename T>
	inline constexpr bool is_primitive_v = is_primitive<T>::value;

	/// <summary>
	/// Provides support for primitive types.
	/// </summary>
	template<typename T>
	struct type_identity<T, std::enable_if_t<is_primitive_v<T>>>
	{
		using type = type_identity_primitive;
	};

	/// <summary>
	/// Provides support for enum types.
	/// </summary>
	template<typename T>
	struct type_identity<T, std::enable_if_t<std::is_enum_v<T>>>
	{
		using type = type_identity_enum;
	};

	/// <summary> 
	/// The signature of a type identity.
	/// </summary>
	template<typename Identity, typename T>
	struct type_identity_signature;

	template<typename Identity, typename T>
	inline constexpr auto& type_identity_signature_v = type_identity_signature<Identity, T>::value;

	/// <summary>
	/// The signature of an enum type.
	/// </summary>
	template<typename T>
	struct type_identity_signature<type_identity_primitive, T>
	{
		static constexpr auto guid_primitive{ to_array(guid{ "47534958-0000-0050-5249-4D4954495645" }) };
		static constexpr auto value{ meta::concat_arrays(guid_primitive, to_array(guid_storage_v<T>)) };
	};

	/// <summary>
	/// The signature of a primitive type.
	/// </summary>
	template<typename T>
	struct type_identity_signature<type_identity_enum, T>
	{
		static constexpr auto guid_enum{ to_array(guid{ "47534958-0000-0000-0000-0000454E554D" }) };
		static constexpr auto value{ meta::concat_arrays(guid_enum, to_array(guid_storage_v<std::underlying_type_t<T>>)) };
	};

	/// <summary>
	/// The signature of an interface type.
	/// </summary>
	template<typename T>
	struct type_identity_signature<type_identity_interface, T>
	{
		static constexpr auto guid_interface{ to_array(guid{ "47534958-0000-0049-4E54-455246414345" }) };
		static constexpr auto value{ meta::concat_arrays(guid_interface, to_array(guid_storage_v<T>)) };
	};

	/// <summary>
	/// The signature of a generic interface type.
	/// </summary>
	template<typename T>
	struct type_identity_signature<type_identity_generic_interface, T>
	{
		static constexpr auto guid_generic_interface{ to_array(guid{ "47534958-0000-4749-4E54-455246414345" }) };
		static constexpr auto value{ meta::concat_arrays(guid_generic_interface, to_array(guid_storage_v<T>)) };
	};

	template<typename... Fields, typename T>
	struct type_identity_signature<type_identity_table<Fields...>, T>
	{
		static constexpr auto guid_table{ to_array(guid{ "47534958-0000-0000-0000-007461626C65" }) };
		static constexpr auto value{ meta::concat_arrays(guid_table, to_array(guid_storage_v<T>)) };
	};

	/// <summary>
	/// The signature of a type.
	/// </summary>
	template<typename T>
	struct type_signature
	{
		static constexpr auto& value{ type_identity_signature_v<type_identity_t<T>, T> };
	};

	template<typename T>
	inline constexpr auto& type_signature_v = type_signature<T>::value;
}

namespace glasssix::exposing
{
	template<typename T>
	inline constexpr auto& guid_of_v = impl::guid_storage_v<T>;

	template<typename T>
	constexpr bool is_guid_of(const guid& id) noexcept
	{
		return id == guid_of_v<T>;
	}
}
