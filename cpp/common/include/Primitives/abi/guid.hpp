#pragma once

#include "meta.hpp"
#include "sha3.hpp"
#include "hash_utils.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <functional>
#include <string_view>

namespace glasssix::exposing
{
	namespace details
	{
		struct size_offset
		{
			std::size_t offset;
			std::size_t size;

			constexpr std::size_t offset_end() const noexcept
			{
				return offset + size;
			}
		};
	}
	
	/// <summary>
	/// Globally Unique Identifier (GUID).
	/// </summary>
	class guid
	{
	public:
		std::uint32_t data1;
		std::uint16_t data2;
		std::uint16_t data3;
		std::array<std::uint8_t, 8> data4;

		constexpr guid() noexcept : data1{}, data2{}, data3{}, data4{}
		{
		}

		constexpr guid(std::uint32_t data1, std::uint16_t data2, std::uint16_t data3, const std::array<std::uint8_t, 8>& data4) noexcept : data1{ data1 }, data2{ data2 }, data3{ data3 }, data4{ data4 }
		{
		}

		constexpr guid(std::string_view str) noexcept : guid{}
		{
			constexpr std::size_t guid_string_size = 36;

			if (str.size() < guid_string_size)
			{
				return;
			}

			constexpr details::size_offset offset_data1{ 0, meta::hexadecimal_character_size_v<std::uint32_t> };
			constexpr details::size_offset offset_data2{ offset_data1.offset_end() + 1, meta::hexadecimal_character_size_v<std::uint16_t> };
			constexpr details::size_offset offset_data3{ offset_data2.offset_end() + 1,  meta::hexadecimal_character_size_v<std::uint16_t> };
			constexpr details::size_offset offset_data4_first{ offset_data3.offset_end() + 1, meta::hexadecimal_character_size_v<std::uint8_t> * 2 };
			constexpr details::size_offset offset_data4_second{ offset_data4_first.offset_end() + 1, meta::hexadecimal_character_size_v<std::uint8_t> * 6 };

			data1 = meta::to_number<std::uint32_t>(str.substr(offset_data1.offset, offset_data1.size));
			data2 = meta::to_number<std::uint16_t>(str.substr(offset_data2.offset, offset_data2.size));
			data3 = meta::to_number<std::uint16_t>(str.substr(offset_data3.offset, offset_data3.size));

			// On MSVC Toolset 14.1, a local lambda implementing the same function cannot compile with error C1001.
			// Workaround is to create an independent helper function below.
			assign_to_data4_helper(str, offset_data4_first, 0, 2);
			assign_to_data4_helper(str, offset_data4_second, 2, 6);
		}

		constexpr bool operator==(const guid& right) const noexcept
		{
			// Both std::array<T>::operator== and std::equal are not constexpr functions.
			return data1 == right.data1 && data2 == right.data2 && data3 == right.data3 && [&]
			{
				for (auto left_ptr = data4.data(), right_ptr = right.data4.data(), end_ptr = data4.data() + data4.size(); left_ptr < end_ptr; left_ptr++, right_ptr++)
				{
					if (*left_ptr != *right_ptr)
					{
						return false;
					}
				}

				return true;
			}();
		}

		constexpr bool operator!=(const guid& right) const noexcept
		{
			return !(*this == right);
		}
	private:
		constexpr void assign_to_data4_helper(std::string_view str, const details::size_offset& source_offset, std::size_t offset, std::size_t size) noexcept
		{
			for (std::size_t i = offset, j = source_offset.offset; i < offset + size; i++, j += meta::hexadecimal_character_size_v<std::uint8_t>)
			{
				data4[i] = meta::to_number<std::uint8_t>(str.substr(j, meta::hexadecimal_character_size_v<std::uint8_t>));
			}
		}
	};
	
	/// <summary>
	/// Converts a GUID to an array.
	/// </summary>
	/// <param name="id">The GUID</param>
	/// <param name="big_endian">A boolean that indicates whether the byte order is big-endian</param>
	/// <returns>The array</returns>
	constexpr auto to_array(const guid& id, bool is_big_endian = true) noexcept
	{
		return meta::concat_arrays(
			meta::to_array(id.data1, is_big_endian),
			meta::to_array(id.data2, is_big_endian),
			meta::to_array(id.data3, is_big_endian),
			id.data4
		);
	}

	/// <summary>
	/// Creates a GUID from a byte buffer.
	/// </summary>
	/// <param name="Size">The size in bytes</param>
	/// <param name="data">The input data</param>
	/// <returns>The guid</returns>
	template<std::size_t Size>
	constexpr guid create_guid_from_bytes(const std::array<std::uint8_t, Size>& data) noexcept
	{
		constexpr guid guid_root{ "2A4F92A8-051D-48DE-8833-7837A9D30699" };
		constexpr details::size_offset offset_data1{ 0, sizeof(guid::data1) };
		constexpr details::size_offset offset_data2{ offset_data1.offset_end(), sizeof(guid::data2) };
		constexpr details::size_offset offset_data3{ offset_data2.offset_end(), sizeof(guid::data3) };
		constexpr details::size_offset offset_data4{ offset_data3.offset_end(), sizeof(guid::data4) };

		// Computes SHA3-512 of the data.
		auto combined_data = meta::concat_arrays(to_array(guid_root), data);
		auto hash = hashing::sha3::hash_sha3_512(combined_data);

		// Truncates the hash and creates a GUID.
		return guid
		{
			meta::make_number<std::uint32_t>(meta::sub_array<offset_data1.offset, offset_data1.size>::get(hash)),
			meta::make_number<std::uint16_t>(meta::sub_array<offset_data2.offset, offset_data2.size>::get(hash)),
			meta::make_number<std::uint16_t>(meta::sub_array<offset_data3.offset, offset_data3.size>::get(hash)),
			meta::sub_array<offset_data4.offset, offset_data4.size>::get(hash)
		};
	}

	/// <summary>
	/// Converts a GUID to a char array.
	/// </summary>
	/// <param name="id">The GUID</param>
	/// <param name="hyphenated">A boolean that indicates whether the result is hyphenated</param>
	/// <returns>The array</returns>
	constexpr auto to_char_array(const guid& id, bool hyphenated = true) noexcept
	{
		return meta::concat_arrays(
			meta::to_char_array(id.data1), std::array<char, 1>{ '-' },
			meta::to_char_array(id.data2), std::array<char, 1>{ '-' },
			meta::to_char_array(id.data3), std::array<char, 1>{ '-' },
			meta::to_char_array(meta::sub_array<0, 2>::get(id.data4)), std::array<char, 1>{ '-' },
			meta::to_char_array(meta::sub_array<2, 6>::get(id.data4))
		);
	}

	/// <summary>
	/// Converts a GUID to a string.
	/// </summary>
	/// <param name="id">The GUID</param>
	/// <returns>The string</returns>
	inline std::string to_string(const guid& id)
	{
		auto chars = to_char_array(id);

		return std::string{ chars.data(), chars.size() };
	}
}

namespace std
{
	template<> struct hash<glasssix::exposing::guid>
	{
		std::size_t operator()(const glasssix::exposing::guid& id) const
		{
			std::size_t result = 0;
			
			glasssix::utils::hash_combine(result, id.data1);
			glasssix::utils::hash_combine(result, id.data2);
			glasssix::utils::hash_combine(result, id.data3);
			std::for_each(id.data4.begin(), id.data4.end(), [&](std::uint8_t inner) { glasssix::utils::hash_combine(result, inner); });

			return result;
		}
	};
}
