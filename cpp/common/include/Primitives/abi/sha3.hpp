#pragma once

#include "meta.hpp"
#include "sha3_details.hpp"

#include <array>
#include <cstdint>
#include <cstddef>
#include <algorithm>

namespace glasssix::exposing::hashing::sha3
{
	namespace details
	{
		/// <summary>
		/// A helper class that digests input data.
		/// </summary>
		template<sha3_type Type>
		class hash_digest
		{
		public:
			using context_type = details::hash_context<Type>;

			/// <summary>
			/// Updates the context with a piece of data.
			/// </summary>
			/// <param name="Size">The size in bytes</param>
			/// <param name="data">The data</param>
			/// <returns>A reference to self</returns>
			template<std::size_t Size>
			constexpr hash_digest& update(const std::array<std::uint8_t, Size>& data) noexcept
			{
				return update(data.data(), data.size());
			}

			/// <summary>
			/// Updates the context with a piece of data.
			/// </summary>
			/// <param name="data">The data</param>
			/// <param name="size">The size in bytes</param>
			/// <returns>A reference to self</returns>
			constexpr hash_digest& update(const std::uint8_t* data, std::size_t size) noexcept
			{
				auto source_ptr = data;
				auto source_end_ptr = data + size;
				auto destination_ptr = context_.block.data() + context_.block_index;

				std::size_t real_size = 0;

				// Fills the internal buffer and updates the context if neccessary.
				while (source_ptr < source_end_ptr)
				{
					real_size = std::min<std::size_t>(source_end_ptr - source_ptr, context_.block_remaining_size());
					meta::copy_bytes(destination_ptr, source_ptr, real_size);

					source_ptr += real_size;
					destination_ptr += real_size;
					context_.block_index += real_size;

					// Updates the state when the internal buffer is full.
					if (context_.block_index >= context_type::block_size)
					{
						details::sponge_step_6(context_);
						destination_ptr = context_.block.data();
					}
				}

				return *this;
			}

			/// <summary>
			/// Finalizes the context (pads the data and calculates the final hash value).
			/// </summary>
			constexpr auto finalize() noexcept
			{
				return details::sponge_finalize(context_);
			}
		private:
			context_type context_;
		};

		template<sha3_type Type, std::size_t Size>
		constexpr auto hash_sha3_impl(const std::array<std::uint8_t, Size>& data) noexcept
		{
			return hash_digest<Type>{}.update(data).finalize();
		}

		template<sha3_type Type>
		constexpr auto hash_sha3_impl(const std::uint8_t* data, std::size_t size) noexcept
		{
			return hash_digest<Type>{}.update(data, size).finalize();
		}
	}

	/// <summary>
	/// Computes SHA3-224 of a byte array.
	/// </summary>
	/// <param name="Size">The size in bytes</param>
	/// <param name="data">The data</param>
	/// <returns>The final hash</returns>
	template<std::size_t Size>
	constexpr auto hash_sha3_224(const std::array<std::uint8_t, Size>& data) noexcept
	{
		return details::hash_sha3_impl<sha3_type::sha3_224>(data);
	}

	/// <summary>
	/// Computes SHA3-256 of a byte array.
	/// </summary>
	/// <param name="Size">The size in bytes</param>
	/// <param name="data">The data</param>
	/// <returns>The final hash</returns>
	template<std::size_t Size>
	constexpr auto hash_sha3_256(const std::array<std::uint8_t, Size>& data) noexcept
	{
		return details::hash_sha3_impl<sha3_type::sha3_256>(data);
	}

	/// <summary>
	/// Computes SHA3-384 of a byte array.
	/// </summary>
	/// <param name="Size">The size in bytes</param>
	/// <param name="data">The data</param>
	/// <returns>The final hash</returns>
	template<std::size_t Size>
	constexpr auto hash_sha3_384(const std::array<std::uint8_t, Size>& data) noexcept
	{
		return details::hash_sha3_impl<sha3_type::sha3_384>(data);
	}

	/// <summary>
	/// Computes SHA3-512 of a byte array.
	/// </summary>
	/// <param name="Size">The size in bytes</param>
	/// <param name="data">The data</param>
	/// <returns>The final hash</returns>
	template<std::size_t Size>
	constexpr auto hash_sha3_512(const std::array<std::uint8_t, Size>& data) noexcept
	{
		return details::hash_sha3_impl<sha3_type::sha3_512>(data);
	}

	/// <summary>
	/// Computes SHA3-224 of a buffer.
	/// </summary>
	/// <param name="data">The data</param>
	/// <param name="size">The size in bytes</param>
	/// <returns>The final hash</returns>
	constexpr auto hash_sha3_224(const std::uint8_t* data, std::size_t size) noexcept
	{
		return details::hash_sha3_impl<sha3_type::sha3_224>(data, size);
	}

	/// <summary>
	/// Computes SHA3-256 of a buffer.
	/// </summary>
	/// <param name="data">The data</param>
	/// <param name="size">The size in bytes</param>
	/// <returns>The final hash</returns>
	constexpr auto hash_sha3_256(const std::uint8_t* data, std::size_t size) noexcept
	{
		return details::hash_sha3_impl<sha3_type::sha3_256>(data, size);
	}

	/// <summary>
	/// Computes SHA3-384 of a buffer.
	/// </summary>
	/// <param name="data">The data</param>
	/// <param name="size">The size in bytes</param>
	/// <returns>The final hash</returns>
	constexpr auto hash_sha3_384(const std::uint8_t* data, std::size_t size) noexcept
	{
		return details::hash_sha3_impl<sha3_type::sha3_384>(data, size);
	}

	/// <summary>
	/// Computes SHA3-512 of a buffer.
	/// </summary>
	/// <param name="data">The data</param>
	/// <param name="size">The size in bytes</param>
	/// <returns>The final hash</returns>
	constexpr auto hash_sha3_512(const std::uint8_t* data, std::size_t size) noexcept
	{
		return details::hash_sha3_impl<sha3_type::sha3_512>(data, size);
	}
}
