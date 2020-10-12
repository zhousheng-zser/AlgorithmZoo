#pragma once

#include "meta.hpp"

#include <array>
#include <limits>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace glasssix::exposing::hashing::sha3
{
	/// <summary>
	/// Available algorithms of SHA-3.
	/// </summary>
	enum class sha3_type
	{
		sha3_224,
		sha3_256,
		sha3_384,
		sha3_512,
		sha3_type_size
	};
}

namespace glasssix::exposing::hashing::sha3::details
{
	/// <summary>
	/// Defines traits of a SHA3 algorithm.
	/// </summary>
	struct sha3_type_traits
	{
		/// <summary>
		/// The block size of a SHA3 algorithm, in bytes, defined in Table 3, Section 7.
		/// The comment in the footer notes that in general, the input block size of a sponge function is its rate (also known as r).
		/// </summary>
		std::size_t block_size;

		/// <summary>
		/// The final hash size in bytes.
		/// </summary>
		std::size_t final_hash_size;
	};

	/// <summary>
	/// A std::uint64_t is 64-bit which has an identical bit length to parameter w (also known as the length of z coordinate).
	/// The definition is in Section 5.2.
	/// </summary>
	using word_type = std::uint64_t;

	/// <summary>
	/// The number of rounds.
	/// </summary>
	inline constexpr std::size_t round_size = 24;

	/// <summary>
	/// The size of a word, in bits.
	/// </summary>
	inline constexpr std::size_t word_bits = std::numeric_limits<word_type>::digits;

	/// <summary>
	/// l = log2(w).
	/// </summary>
	inline constexpr std::size_t log2_word_bits = meta::log2(word_bits);

	/// <summary>
	/// A common factor which is commonly used, i.e. number 5.
	/// </summary>
	inline constexpr std::size_t common_factor = 5;

	/// <summary>
	/// The size of a sponge, in words, defined in Section 5.2.
	/// </summary>
	inline constexpr std::size_t sponge_words = 1600 / meta::byte_bits / sizeof(word_type);

	/// <summary>
	/// Available traits of available SHA3 algorithms.
	/// </summary>
	inline constexpr std::array<sha3_type_traits, static_cast<std::size_t>(sha3_type::sha3_type_size)> available_sha3_type_traits
	{
		sha3_type_traits{ 144, 224 / meta::byte_bits },
		sha3_type_traits{ 136, 256 / meta::byte_bits },
		sha3_type_traits{ 104, 384 / meta::byte_bits },
		sha3_type_traits{ 72, 512 / meta::byte_bits }
	};

	/// <summary>
	/// Represents a state array defined in Section 3.1, of which the data are word-aligned (std::uint64_t).
	/// </summary>
	class state_array
	{
	public:
		/// <summary>
		/// Creates an instance.
		/// </summary>
		constexpr state_array() noexcept : data_{}
		{
		}

		/// <summary>
		/// Gets the size of the state array.
		/// </summary>
		/// <returns>The size</returns>
		constexpr std::size_t size() const noexcept
		{
			return data_.size();
		}

		/// <summary>
		/// Resets the data.
		/// </summary>
		constexpr void reset() noexcept
		{
			data_ = {};
		}

		/// <summary>
		/// Makes access to a word by index.
		/// </summary>
		/// <param name="index">The index</param>
		/// <returns>A reference to the word</returns>
		constexpr word_type& operator[](std::size_t index) noexcept
		{
			return data_[index];
		}

		/// <summary>
		/// Makes access to a word by index in read-only mode.
		/// </summary>
		/// <param name="index">The index</param>
		/// <returns>A const reference to the word</returns>
		constexpr const word_type& operator[](std::size_t index) const noexcept
		{
			return data_[index];
		}

		/// <summary>
		/// Makes access to a word by (x, y) coordinate.
		/// </summary>
		/// <param name="x">The x coordinate</param>
		/// <param name="y">The y coordinate</param>
		/// <returns>A reference to the word</returns>
		constexpr word_type& operator()(std::size_t x, std::size_t y) noexcept
		{
			return data_[calculate_index(x, y)];
		}

		/// <summary>
		/// Makes access to a word by (x, y) coordinate in read-only mode.
		/// </summary>
		/// <param name="x">The x coordinate</param>
		/// <param name="y">The y coordinate</param>
		/// <returns>A const reference to the word</returns>
		constexpr const word_type& operator()(std::size_t x, std::size_t y) const noexcept
		{
			return data_[calculate_index(x, y)];
		}

		/// <summary>
		/// Truncates the data as N bytes.
		/// </summary>
		/// <param name="Size">The size in bytes</param>
		/// <param name="buffer">The buffer to accomoate the result</param>
		template<std::size_t Size, typename = std::enable_if_t<Size <= sponge_words * sizeof(word_type)>>
		constexpr void truncate_as_bytes(std::array<std::uint8_t, Size>& buffer_) noexcept
		{
			constexpr std::size_t buffer_words = Size / sizeof(word_type);
			constexpr std::size_t remaining_bytes = Size % sizeof(word_type);
			std::size_t index = 0;

			for (std::size_t i = 0; i < buffer_words; i++)
			{
				// According to the NIST standard, "strings" defined in that are littie-endian.
				meta::split_number(data_[i], [&](auto... bytes) { ((buffer_[index++] = bytes), ...); }, false);
			}

			// Add the remaining bytes in the last incomplete word.
			if constexpr (remaining_bytes != 0)
			{
				meta::split_number(data_[buffer_words], [&](auto... bytes) { ((buffer_[index++] = bytes), ...); }, false, std::integral_constant<std::size_t, remaining_bytes>{});
			}
		}

		/// <summary>
		/// Calculates the absoluate index of a given (x, y) coordinate.
		/// </summary>
		/// <param name="x">The x coordinate</param>
		/// <param name="y">The y coordinate</param>
		/// <returns>The absolute index</returns>
		static constexpr std::size_t calculate_index(std::size_t x, std::size_t y) noexcept
		{
			// A[x, y, z] = S[w(5y + x) + z] defined in Section 3.1.2.
			// The calculation is simplied as follows because values of a lane (values along z coordinate) are combined into a word (std::uint64_t).
			return common_factor * (y % common_factor) + (x % common_factor);
		}
	private:
		std::array<word_type, sponge_words> data_;
	};

	/// <summary>
	/// The context of a hash algorithm.
	/// </summary>
	template<sha3_type Type>
	struct hash_context
	{
		static constexpr std::size_t block_size = available_sha3_type_traits[static_cast<std::size_t>(Type)].block_size;
		static constexpr std::size_t final_hash_size = available_sha3_type_traits[static_cast<std::size_t>(Type)].final_hash_size;

		state_array state;
		state_array intermediate;
		std::size_t block_index;
		std::array<word_type, 5> tmp;
		std::array<std::uint8_t, block_size> block;

		/// <summary>
		/// Creates an instance.
		/// </summary>
		/// <param name="type">The type of the algorithm</param>
		constexpr hash_context() noexcept : block_index{}, tmp{}, block{}
		{
		}

		/// <summary>
		/// Gets the current remaining size of bytes that can be appended to the block.
		/// </summary>
		/// <returns>The remaining size</returns>
		constexpr std::size_t block_remaining_size() const noexcept
		{
			return block_size - block_index;
		}

		/// <summary>
		/// Resets all states.
		/// </summary>
		constexpr void reset() noexcept
		{
			tmp = {};
			block = {};
			state.reset();
			intermediate.reset();
			block_index = 0;
		}
	};

	/// <summary>
	/// A helper function for ¦Ó(A) below defined in Section 3.2.5.
	/// </summary>
	/// <param name="number">The number</param>
	/// <returns>The result</returns>
	constexpr std::uint8_t step_mapping_helper_rc(word_type number) noexcept
	{
		// If t mod 255 = 0, return 1.
		if (number % 0xFF == 0)
		{
			return 1;
		}

		// Let R = 1000'0000.
		// Here all numbers are inverted for convenience.
		std::uint8_t bit = 0;
		word_type result = 0b0000'0001;

		for (std::size_t i = 1; i <= number % 0xFF; i++)
		{
			// For i from 1 to t mod 255, let:
			// a.R = 0 || R;
			// b.R[0] = R[0] ¨’ R[8];
			// c.R[4] = R[4] ¨’ R[8];
			// d.R[5] = R[5] ¨’ R[8];
			// e.R[6] = R[6] ¨’ R[8];
			// f.R = Trunc8[R].
			result <<= 1;
			bit = meta::get_number_bit(result, 8);

			meta::set_number_bit(result, 0, meta::get_number_bit(result, 0) ^ bit);
			meta::set_number_bit(result, 4, meta::get_number_bit(result, 4) ^ bit);
			meta::set_number_bit(result, 5, meta::get_number_bit(result, 5) ^ bit);
			meta::set_number_bit(result, 6, meta::get_number_bit(result, 6) ^ bit);

			result &= 0xFF;
		}

		// Return R[0].
		return static_cast<std::uint8_t>(result & 0x01);
	}

	/// <summary>
	/// Generates a table of rotation bits for ¦Ñ(A) at compile-time, defined in Section 3.2.2.
	/// </summary>
	inline constexpr auto step_mapping_rho_rotation_bits = []
	{
		std::array<int, sponge_words> result{};

		/// (x, y) = (1, 0)
		/// For t from 0 to 23, (t + 1)(t + 2) / 2 mod w
		for (std::size_t x = 1, y = 0, t = 0, tmp = 0; t < 24; t++)
		{
			result[state_array::calculate_index(x, y)] = (((t + 1) * (t + 2)) >> 1) % word_bits;

			// (x, y) = (y, (2x + 3y) mod 5)
			tmp = y;
			y = (2 * x + 3 * y) % common_factor;
			x = tmp;
		}

		return result;
	}();

	/// <summary>
	/// Generates a RC table for ¦Ó(A) at compile-time.
	/// </summary>
	inline constexpr auto step_mapping_tau_rc_table = meta::apply_index_sequence<round_size>([](auto... indexes)
		{
			constexpr auto func_rc = [](std::size_t round)
			{
				// Let RC = 0w.
				// For j from 0 to l = log2(w), let RC[2 ^ j ¨C 1] = rc(j + 7ir).
				word_type result = 0;

				for (std::size_t i = 0; i <= log2_word_bits; i++)
				{
					meta::set_number_bit(result, (1 << i) - 1, step_mapping_helper_rc(i + 7 * round));
				}

				return result;
			};

			return std::array<word_type, sizeof...(indexes)>{ func_rc(indexes)... };
		});

	/// <summary>
	/// A step mapping function named ¦È(A) defined in Section 3.2.1.
	/// </summary>
	/// <param name="type">The type of the alogrithm</param>
	/// <param name="context">The hash context</param>
	template<sha3_type Type>
	constexpr void step_mapping_theta(hash_context<Type>& context) noexcept
	{
		// For all pairs (x, z) such that 0 ¡Ü x < 5 and 0 ¡Ü z < w, let C[x, z] = A[x, 0, z] ¨’ A[x, 1, z] ¨’ A[x, 2, z] ¨’ A[x, 3, z] ¨’ A[x, 4, z].
		// Here a lane (values along z coordinate) is represented as a word (std::uint64_t).
		for (std::size_t x = 0; x < common_factor; x++)
		{
			context.tmp[x] = meta::apply_index_sequence<common_factor>([&](auto... indexes) { return (context.state(x, indexes) ^ ...); });
		}

		// For all pairs (x, z) such that 0 ¡Ü x < 5 and 0 ¡Ü z < w, let D[x, z] = C[(x - 1) mod 5, z] ¨’ C[(x + 1) mod 5, (z - 1) mod w].
		// For all triples(x, y, z) such that 0 ¡Ü x < 5, 0 ¡Ü y < 5, and 0 ¡Ü z < w, let A¡ä[x, y, z] = A[x, y, z] ¨’ D[x, z].
		// Figure 3 in Section 3.2.1 is intuitive and (z - 1) mod w is equivalent to rotating a word to the left by one bit.
		for (std::size_t x = 0; x < common_factor; x++)
		{
			for (std::size_t y = 0; y < common_factor; y++)
			{
				context.state(x, y) ^= (context.tmp[meta::minus_mod_unsigned<std::size_t>(x, 1, common_factor)] ^ meta::rotl(context.tmp[(x + 1) % common_factor], 1));
			}
		}
	}

	/// <summary>
	/// A step mapping function named ¦Ñ(A) defined in Section 3.2.2.
	/// </summary>
	/// <param name="type">The type of the alogrithm</param>
	/// <param name="context">The hash context</param>
	template<sha3_type Type>
	constexpr void step_mapping_rho(hash_context<Type>& context) noexcept
	{
		// For all z such that 0 ¡Ü z ¡Ü w, let A'[0, 0, z] = A[0, 0, z].
		context.intermediate(0, 0) = context.state(0, 0);

		// (x, y) = (1, 0)
		// For t from 0 to 23.
		meta::apply_index_sequence<step_mapping_rho_rotation_bits.size()>([&](auto... indexes)
			{
				// For all z such that 0 ¡Ü z < w, let A¡ä[x, y, z] = A[x, y, (z ¨C (t + 1)(t + 2) / 2) mod w].
				// Here a lane (values along z coordinate) is represented as a word (std::uint64_t).
				// (x, y) = (y, (2x + 3y) mod 5)
				((context.intermediate[indexes] = meta::rotl(context.state[indexes], step_mapping_rho_rotation_bits[indexes])), ...);
			});
	}

	/// <summary>
	/// A step mapping function named ¦Ð(A) defined in Section 3.2.3.
	/// </summary>
	/// <param name="type">The type of the alogrithm</param>
	/// <param name="context">The hash context</param>
	template<sha3_type Type>
	constexpr void step_mapping_pi(hash_context<Type>& context) noexcept
	{
		for (std::size_t x = 0; x < common_factor; x++)
		{
			for (std::size_t y = 0; y < common_factor; y++)
			{
				// For all triples (x, y, z) such that 0 ¡Ü x < 5, 0 ¡Ü y < 5, and 0 ¡Ü z < w, let A¡ä[x, y, z] = A[(x + 3y) mod 5, x, z].
				// Here a lane (values along z coordinate) is represented as a word (std::uint64_t).
				context.state(x, y) = context.intermediate(x + 3 * y, x);
			}
		}
	}

	/// <summary>
	/// A step mapping function named ¦Ö(A) defined in Section 3.2.4.
	/// </summary>
	/// <param name="type">The type of the alogrithm</param>
	/// <param name="context">The hash context</param>
	template<sha3_type Type>
	constexpr auto step_mapping_chi(hash_context<Type>& context) noexcept
	{
		for (std::size_t x = 0; x < common_factor; x++)
		{
			for (std::size_t y = 0; y < common_factor; y++)
			{
				// For all triples (x, y, z) such that 0 ¡Ü x < 5, 0 ¡Ü y < 5, and 0 ¡Ü z < w, let A¡ä[x, y, z] = A[x, y, z] ¨’((A[(x + 1) mod 5, y, z] ¨’ 1) ¡¤ A[(x + 2) mod 5, y, z]).
				// Here a lane (values along z coordinate) is represented as a word (std::uint64_t).
				context.intermediate(x, y) = context.state(x, y) ^ (~context.state(x + 1, y) & context.state(x + 2, y));
			}
		}
	}

	/// <summary>
	/// A step mapping function named ¦Ó(A) defined in Section 3.2.5.
	/// </summary>
	/// <param name="type">The type of the alogrithm</param>
	/// <param name="context">The hash context</param>
	/// <param name="round">The index of the current round</param>
	template<sha3_type Type>
	constexpr void step_mapping_tau(hash_context<Type>& context, std::size_t round) noexcept
	{
		// For all triples (x, y, z) such that 0 ¡Ü x < 5, 0 ¡Ü y < 5, and 0 ¡Ü z < w, let A¡ä[x, y, z] = A[x, y, z].
		// For all z such that 0 ¡Ü z < w, let A¡ä [0, 0, z] = A¡ä [0, 0, z] ¨’ RC[z].
		// Here a lane (values along z coordinate) is represented as a word (std::uint64_t).
		context.state = context.intermediate;
		context.state(0, 0) ^= step_mapping_tau_rc_table[round];
	}

	/// <summary>
	/// A function that implements the KECCAK-p[b, nr] permutation.
	/// </summary>
	/// <param name="type">The type of the alogrithm</param>
	/// <param name="context">The hash context</param>
	template<sha3_type Type>
	constexpr void keccak_p(hash_context<Type>& context) noexcept
	{
		for (std::size_t i = 0; i < round_size; i++)
		{
			step_mapping_theta(context);
			step_mapping_rho(context);
			step_mapping_pi(context);
			step_mapping_chi(context);
			step_mapping_tau(context, i);
		}
	}

	/// <summary>
	/// A function that implements pad10*1(x, m) defined in Section 5.1 and M || 01 defined in Section 6.1.
	/// </summary>
	/// <param name="type">The type of the alogrithm</param>
	/// <param name="context">The hash context</param>
	template<sha3_type Type>
	constexpr void pad10_1_and_append_01(hash_context<Type>& context) noexcept
	{
		// pad10*1(x, m) = string P such that m + len(P) is a positive multiple of x.
		// 1. Let j = (¨C m ¨C 2) mod x.
		// 2. Return P = 1 || 0j || 1.
		// For a byte buffer, the padding bytes are 0x01(0b0000'0001) and 0x80(0b1000'0000).
		// Considering M || 01 defined in Section 6.1, the final padding bytes are 0x06((0b0000'0001 << 2) | 0b0000'0010, which shortens the padding zeros by 2 bits) and 0x80;
		if (context.block_index + 1 == hash_context<Type>::block_size)
		{
			// For only one single byte, just combines two parts as a result of 0x86(0b1000'0000 | 0b0000'0110).
			context.block[context.block_index] = 0x86;
		}
		else
		{
			context.block[context.block_index] = 0x06;

			for (std::size_t i = context.block_index + 1; i < hash_context<Type>::block_size - 1; i++)
			{
				context.block[i] = 0;
			}

			context.block.back() = 0x80;
		}
	}

	/// <summary>
	/// A function that updates the current state by SPONGE[f, pad, r](N, d) (when r = block size) defined in Step 6, Algorithm 8, Section 4.
	/// </summary>
	/// <param name="type">The type of the alogrithm</param>
	/// <param name="context">The hash context</param>
	template<sha3_type Type>
	constexpr void sponge_step_6(hash_context<Type>& context) noexcept
	{
		// For i from 0 to n - 1, let S = f(S ¨’ (Pi || 0c)).
		// Here n refers to the block at index n.
		constexpr std::size_t block_words = hash_context<Type>::block_size / sizeof(word_type);

		for (std::size_t i = 0; i < block_words; i++)
		{
			meta::apply_index_sequence<sizeof(word_type)>([&](auto... indexes)
				{
					// According to the NIST standard, "strings" defined in that are littie-endian.
					context.state[i] ^= meta::make_number<word_type>({ context.block[i * sizeof(word_type) + indexes]... }, false);
				});
		}

		// f = KECCAK-p[b, nr]
		keccak_p(context);
		context.block_index = 0;
	}

	/// <summary>
	/// A function that finalizes the state by SPONGE[f, pad, r](N, d) (when r = block size) defined in Algorithm 8, Section 4 and Section 4.
	/// </summary>
	/// <param name="type">The type of the alogrithm</param>
	/// <param name="context">The hash context</param>
	/// <returns>The final hash result</returns>
	template<sha3_type Type>
	constexpr auto sponge_finalize(hash_context<Type>& context) noexcept
	{
		// 1. Let P = N || pad(r, len(N)).
		// 2. Let n = len(P) / r.
		// 3. Let c = b - r.
		// 4. Let P0, ... , Pn - 1 be the unique sequence of strings of length r such that P = P0 || ¡­ || Pn-1.
		// 5. Let S = 0b.
		// 6. For i from 0 to n - 1, let S = f(S ¨’(Pi || 0c)).
		// 7. Let Z be the empty string.
		// 8. Let Z = Z || Trunc_r(S).
		// 9. If d ¡Ü |Z| , then return Trunc d(Z); else continue.
		// 10. Let S = f(S), and continue with Step 8.
		pad10_1_and_append_01(context);
		sponge_step_6(context);

		std::array<std::uint8_t, hash_context<Type>::final_hash_size> result{};

		// The final hash size denoted by d is always smaller than |Z|, so "else" in Step 9, and Step 10 are omitted here.
		context.state.truncate_as_bytes(result);
		context.reset();

		return result;
	}
}
