#pragma once

#include "json.hpp"

#include <tuple>
#include <utility>
#include <optional>
#include <type_traits>
#include <string_view>

namespace nlohmann::details
{
	struct json_extended_user_data_tag {};
	struct json_extented_operation_tag {};

	template<typename T, typename... Args>
	struct json_extended_user_data : json_extended_user_data_tag
	{
		using owning_type = std::decay_t<T>;
		using tuple_type = std::tuple<const std::decay_t<T>&, Args...>;

		tuple_type value;

		template<typename U, typename... PrivateArgs, typename = std::enable_if_t<std::conjunction_v<std::is_convertible<U, T>, std::is_convertible<PrivateArgs, Args>...>>>
		json_extended_user_data(U&& parent, PrivateArgs&&... args) : value{ std::forward<U>(parent), std::forward<PrivateArgs>(args)... }
		{
		}
	};

	template<typename T, typename... Args>
	json_extended_user_data(T, Args...)->json_extended_user_data<T, Args...>;

	template<typename UserData, typename T>
	struct is_user_data_owning_type_same : std::is_same<typename std::decay_t<UserData>::owning_type, T> {};

	template<typename UserData, typename T>
	inline constexpr bool is_user_data_owning_type_same_v = is_user_data_owning_type_same<UserData, T>::value;

	struct get_or_default_tag : json_extented_operation_tag
	{
		template<typename T, typename = std::enable_if_t<std::is_default_constructible_v<T>>>
		auto operator()(std::string_view key, T& obj) const
		{
			return json_extended_user_data{ *this, key, std::ref(obj), std::nullopt };
		}

		template<typename T, typename U, typename = std::enable_if_t<std::conjunction_v<std::is_default_constructible<T>, std::is_convertible<U, T>>>>
		auto operator()(std::string_view key, T& obj, U&& default_value) const
		{
			return json_extended_user_data{ *this, key, std::ref(obj), std::forward<U>(default_value) };
		}
	};

	/// <summary>
	/// An extended JSON operation.
	/// </summary>
	template<typename T, typename = void>
	struct json_extended_operation {};
	
	/// <summary>
	/// Extended JSON operation - get_or_default.
	/// </summary>
	template<typename T>
	struct json_extended_operation<T, std::enable_if_t<is_user_data_owning_type_same_v<T, get_or_default_tag>>>
	{
		static void invoke(const json& value, T&& data)
		{
			auto&& [parent, key, obj, default_value] = std::forward<T>(data).value;

			if (value.contains(key))
			{
				return static_cast<void>(value.at(std::string{ key }).get_to(obj.get()));
			}

			if constexpr (std::is_same_v<std::decay_t<decltype(default_value)>, std::nullopt_t>)
			{
				obj.get() = {};
			}
			else
			{
				obj.get() = std::move(default_value);
			}
		}
	};
}

namespace nlohmann
{
	inline constexpr details::get_or_default_tag get_or_default;

	template<typename T, typename = std::enable_if_t<std::is_base_of_v<details::json_extended_user_data_tag, T>>>
	auto operator|(const json& value, T&& data)
	{
		return details::json_extended_operation<T>::invoke(value, std::forward<T>(data));
	}
}
