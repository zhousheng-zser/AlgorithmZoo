#pragma once

#include "environment.hpp"

#include <cctype>
#include <string>
#include <cstddef>
#include <utility>
#include <algorithm>

namespace glasssix
{
	namespace utils
	{

#ifdef GLASSSIX_ENVIRONMENT_32_BITS
		template<typename T>
		void hash_combine(std::size_t& result, T&& value)
		{
			using pure_type = std::decay_t<T>;

			constexpr std::size_t magic_factor = 0x9E3779B9;
			auto hash = std::hash<pure_type>{}(std::forward<T>(value));

			result ^= hash + magic_factor + (result << 6) + (result >> 2);
		}
#elif defined(GLASSSIX_ENVIRONMENT_64_BITS)
		template<typename T>
		void hash_combine(std::size_t& result, T&& value)
		{
			using pure_type = std::decay_t<T>;

			constexpr std::size_t magic_factor = 0xC6A4A7935BD1E995;
			auto hash = std::hash<pure_type>{}(std::forward<T>(value));

			hash *= magic_factor;
			hash ^= hash >> 47;
			hash *= magic_factor;

			result ^= hash;
			result *= magic_factor;

			// Completely arbitrary number, to prevent 0's from hashing to 0.
			result += 0xE6546B64;
		}
#endif
		template<typename... Args>
		std::size_t hash_all(Args&&... args)
		{
			std::size_t result = 0;
			(hash_combine(result, std::forward<Args>(args)), ...);

			return result;
		}
	}

	/// <summary>
	/// A hash function for std::string with a case-insensitive calculator.
	/// </summary>
	struct case_insensitive_string_hash
	{
		auto operator()(const std::string& value) const
		{
			std::size_t result = 0;

			std::for_each(value.begin(), value.end(), [&](int c) { utils::hash_combine(result, std::tolower(c)); });

			return result;
		}
	};

	/// <summary>
	/// A case-insensitive comparer for std::string.
	/// </summary>
	struct case_insensitive_string_comparer
	{
		bool operator()(const std::string& left, const std::string& right) const
		{
			return left.size() == right.size() && std::equal(std::begin(left), std::end(left), std::begin(right), std::end(right), [](int c1, int c2) { return c1 == c2 || std::tolower(c1) == std::tolower(c2); });
		}
	};
}
