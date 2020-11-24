#pragma once

#include "base.hpp"
#include "base_abi.hpp"
#include "exceptions.hpp"
#include "param_string.hpp"
#include "fundamental_semantics.hpp"

#include <cstdint>
#include <cstddef>
#include <utility>
#include <type_traits>

namespace glasssix::exposing
{
	template<typename T>
	class param_span;
}

namespace glasssix::exposing::impl
{
	template<typename T>
	struct abi<param_span<T>>
	{
		using identity_type = type_identity_generic_interface;
		using type = param_span<T>;

		static constexpr guid id{ "4BBC2561-97C4-4C12-A413-7636DBCD70F9" };
	};

	template<typename T>
	struct is_param_span : std::false_type {};

	template<typename T>
	struct is_param_span<param_span<T>> : std::true_type {};

	/// <summary>
	/// Checks whether a type is a span.
	/// </summary>
	template<typename T>
	inline constexpr bool is_param_span_v = is_param_span<T>::value;

	template<typename T, typename = void>
	struct has_data_and_size : std::false_type{};

	template<typename T>
	struct has_data_and_size<T, std::void_t<decltype(std::declval<T>().data()), decltype(std::declval<T>().size())>> : std::true_type{};

	/// <summary>
	/// Checks whether a type has a member function "data" and a member function "size".
	/// </summary>
	template<typename T>
	inline constexpr bool has_data_and_size_v = has_data_and_size<T>::value;

	template<typename T>
	struct is_random_accessible_container : std::conjunction<std::negation<is_param_span<T>>, meta::is_iterator_category_same<T, std::random_access_iterator_tag>, has_data_and_size<T>> {};

	/// <summary>
	/// Checks whether a type is a random-accessible container.
	/// </summary>
	template<typename T>
	inline constexpr bool is_random_accessible_container_v = is_random_accessible_container<T>::value;
}

namespace glasssix::exposing
{
	/// <summary>
	/// Contains a span of elements.
	/// </summary>
	/// <typeparam name="T">The element type</typeparam>
	template<typename T>
	class param_span
	{
	public:
		using value_type = T;
		using iterator = value_type*;
		using const_iterator = const value_type*;
		using reverse_iterator = std::reverse_iterator<iterator>;
		using const_reverse_iterator = std::reverse_iterator<const_iterator>;

		/// <summary>
		/// Creates an instance.
		/// </summary>
		/// <param name="data">The data</param>
		/// <param name="size">The size</param>
		param_span(T* data, std::size_t size) noexcept : data_{ data }, size_{ size }
		{
		}

		/// <summary>
		/// Creates an instance from a random access container.
		/// </summary>
		/// <typeparam name="Container">The container type</typeparam>
		/// <param name="container">The container</param>
		template<typename Container, typename = std::enable_if_t<impl::is_random_accessible_container_v<std::decay_t<Container>>>>
		param_span(Container&& container) noexcept : data_{ std::forward<Container>(container).data() }, size_{ std::forward<Container>(container).size() }
		{
		}

		/// <summary>
		/// Creates an instance with nullptr.
		/// This overload is only for initialization purpose only.
		/// </summary>
		param_span(std::nullptr_t) noexcept : data_{}, size_{}
		{
		}

		/// <summary>
		/// Create an instance with an ABI from which ownership is taken.
		/// </summary>
		/// <param name="abi">The ABI</param>
		param_span(take_over_abi_from_void_ptr abi) noexcept
		{
			*this = *abi.to<param_span*>();
		}

		param_span(const param_span& other) noexcept : data_{ other.data() }, size_{ other.size_ }
		{
		}

		param_span(param_span&& other) noexcept : data_{ std::exchange(other.data_, nullptr) }, size_{ std::exchange(other.size_, 0) }
		{
		}

		param_span& operator=(const param_span& right) noexcept
		{
			return (data_ = right.data_, size_ = right.size_, *this);
		}

		param_span& operator=(param_span&& right) noexcept
		{
			return (data_ = std::exchange(right.data_, nullptr), size_ = std::exchange(right.size_, 0), *this);
		}

		T& operator[](std::size_t index) const
		{
			return index < size_ ? data_[index] : throw abi_out_of_bounds{ format(FMT_STRING(u8"Index: {}, Size: {}"), index, size_) };
		}

		explicit operator bool() const noexcept
		{
			return data_;
		}

		bool empty() const noexcept
		{
			return data_ == nullptr || size_ == 0;
		}

		T* data() const noexcept
		{
			return data_;
		}

		std::size_t size() const noexcept
		{
			return size_;
		}

		iterator begin() noexcept
		{
			return data_;
		}

		const_iterator begin() const noexcept
		{
			return data_;
		}

		const_iterator cbegin() const noexcept
		{
			return begin();
		}

		reverse_iterator rbegin() noexcept
		{
			return reverse_iterator{ end() };
		}

		const_reverse_iterator rbegin() const noexcept
		{
			return const_reverse_iterator{ end() };
		}

		reverse_iterator crbegin() const noexcept
		{
			return rbegin();
		}

		iterator end() noexcept
		{
			return data_ + size_;
		}

		const_iterator end() const noexcept
		{
			return data_ + size_;
		}

		iterator cend() const noexcept
		{
			return end();
		}

		reverse_iterator rend() noexcept
		{
			return reverse_iterator{ begin() };
		}

		const_reverse_iterator rend() const noexcept
		{
			return const_reverse_iterator{ begin() };
		}

		const_reverse_iterator crend() const noexcept
		{
			return rend();
		}

		param_span<T> sub_span(std::size_t index, std::size_t size)
		{
			return index < size_ && index + size <= size_ ? param_span<T>{ data_ + index, size } : throw abi_out_of_bounds{ format(FMT_STRING(u8"Index: {}, Size: {}"), index, size) };
		}
	private:
		T* data_;
		std::size_t size_;
	};
}

namespace glasssix::exposing
{
	/// <summary>
	/// Gets the ABI of a span with type information erased.
	/// </summary>
	/// <typeparam name="T">The element type</typeparam>
	/// <param name="span">The span</param>
	/// <returns>The ABI</returns>
	template<typename T>
	param_span<T> get_abi(const param_span<T>& span) noexcept
	{
		return span;
	}

	/// <summary>
	/// Gets a pointer to the ABI of a span with type information erased.
	/// The ABI will not be cleared and the caller must ensure safety.
	/// </summary>
	/// <typeparam name="T">The element type</typeparam>
	/// <param name="span">The span</param>
	/// <returns>The pointer to the ABI</returns>
	template<typename T>
	param_span<T>* put_abi_dangerous(param_span<T>& span) noexcept
	{
		return &span;
	}

	/// <summary>
	/// Gets a pointer to the ABI of a span with type information erased.
	/// </summary>
	/// <typeparam name="T">The element type</typeparam>
	/// <param name="span">The span</param>
	/// <returns>The pointer to the ABI</returns>
	template<typename T>
	param_span<T>* put_abi(param_span<T>& span) noexcept
	{
		return (span = nullptr, put_abi_dangerous(span));
	}

	/// <summary>
	/// Detaches the ABI from a span.
	/// </summary>
	/// <typeparam name="T">The element type</typeparam>
	/// <param name="span">The span</param>
	/// <returns>The ABI detached from the span</returns>
	template<typename T>
	param_span<T> detach_abi(param_span<T>& span) noexcept
	{
		return get_abi(span);
	}

	/// <summary>
	/// Detaches the ABI from a span.
	/// </summary>
	/// <param name="span">The span</param>
	/// <returns>The ABI detached from the span</returns>
	template<typename T>
	param_span<T> detach_abi(param_span<T>&& span) noexcept
	{
		return get_abi(span);
	}

	/// <summary>
	/// Creates a span from an ABI.
	/// </summary>
	/// <typeparam name="T">The span type</typeparam>
	/// <param name="abi">The ABI</param>
	/// <returns>The span</returns>
	template<typename T, std::enable_if_t<impl::is_param_span_v<std::decay_t<T>>>* = nullptr>
	T create_from_abi(const T& abi) noexcept
	{
		return abi;
	}
}
