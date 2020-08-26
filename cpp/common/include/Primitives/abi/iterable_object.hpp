#pragma once

#include "base.hpp"
#include "base_abi.hpp"
#include "implements.hpp"
#include "exceptions.hpp"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <iterator>
#include <type_traits>

namespace glasssix::exposing
{
	template<typename T>
	struct object_iterator;

	template<typename T>
	struct iterable_object;
}

namespace glasssix::exposing::impl
{
	/// <summary>
	/// The ABI of an object iterator.
	/// </summary>
	template<typename T>
	struct abi<object_iterator<T>>
	{
		using identity_type = type_identity_generic_interface;
		static constexpr guid id{ "0EEE0761-A2EA-4422-9777-C639BDEEE431" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL current(abi_out_t<T> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL valid(abi_out_t<bool> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL move_to_next(abi_out_t<bool> result) noexcept = 0;
		};
	};

	/// <summary>
	/// The ABI of an iterable object.
	/// </summary>
	template<typename T>
	struct abi<iterable_object<T>>
	{
		using identity_type = type_identity_generic_interface;
		static constexpr guid id{ "939BA2A7-C897-4F14-B0BE-5DF0F21889A0" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL get_iterator(abi_out_t<object_iterator<T>> result) noexcept = 0;
		};
	};

	/// <summary>
	/// The vtable of an object iterator.
	/// </summary>
	template<typename Derived, typename T>
	struct interface_vtable<Derived, object_iterator<T>> : interface_vtable_base<Derived, object_iterator<T>>
	{
		virtual std::int32_t G6_ABI_CALL current(abi_out_t<T> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().current()); });
		}

		virtual std::int32_t G6_ABI_CALL valid(abi_out_t<bool> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().valid()); });
		}

		virtual std::int32_t G6_ABI_CALL move_to_next(abi_out_t<bool> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().move_to_next()); });
		}
	};

	/// <summary>
	/// The vtable of an iterable object.
	/// </summary>
	template<typename Derived, typename T>
	struct interface_vtable<Derived, iterable_object<T>> : interface_vtable_base<Derived, iterable_object<T>>
	{
		virtual std::int32_t G6_ABI_CALL get_iterator(abi_out_t<object_iterator<T>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().get_iterator()); });
		}
	};

	/// <summary>
	/// The ABI adapter of an object iterator.
	/// </summary>
	template<typename T>
	struct abi_adapter<object_iterator<T>>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, object_iterator<T>>
		{
			T current() const
			{
				T result{ null_value_v<T> };

				return (check_abi_result(this->self_abi().current(put_abi(result))), result);
			}

			bool valid() const
			{
				bool result = false;

				return (check_abi_result(this->self_abi().valid(put_abi(result))), result);
			}

			bool move_to_next() const
			{
				bool result = false;

				return (check_abi_result(this->self_abi().move_to_next(put_abi(result))), result);
			}

			auto& operator++()
			{
				// Here we define a nullptr as an end iterator.
				if (!move_to_next())
				{
					static_cast<Derived&>(*this) = nullptr;
				}

				return *this;
			}

			T operator*() const
			{
				return current();
			}
		};
	};

	/// <summary>
	/// The ABI adapter of an iterable object.
	/// </summary>
	template<typename T>
	struct abi_adapter<iterable_object<T>>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, iterable_object<T>>
		{
			object_iterator<T> get_iterator() const
			{
				object_iterator<T> result{ nullptr };

				return (check_abi_result(this->self_abi().get_iterator(put_abi(result))), result);
			}
		};
	};
}

namespace glasssix::exposing
{
	/// <summary>
	/// An object iterator.
	/// </summary>
	template<typename T>
	struct object_iterator : inherits<object_iterator<T>>
	{
		using inherits<object_iterator<T>>::inherits;
	};

	/// <summary>
	/// An iterable object.
	/// </summary>
	template<typename T>
	struct iterable_object : inherits<iterable_object<T>>
	{
		using inherits<iterable_object<T>>::inherits;
	};
}

namespace std
{
	/// <summary>
	/// Adds a specialization for iterator_traits.
	/// It is a must to implement the pattern because the standard library needs the information to validate and evaluate the iterators.
	/// </summary>
	template<typename T>
	struct iterator_traits<glasssix::exposing::object_iterator<T>>
	{
		using iterator_category = std::input_iterator_tag;
		using value_type = T;
		using pointer = value_type*;
		using reference = value_type&;
		using difference_type = std::ptrdiff_t;
	};
}

namespace glasssix::exposing
{
	template<typename T, typename = std::void_t<decltype(std::declval<T>().get_iterator())>>
	auto end(T&& obj) noexcept
	{
		return decltype(std::declval<T>().get_iterator()){};
	}

	template<typename T, typename = std::void_t<decltype(std::declval<T>().get_iterator())>>
	auto begin(T&& obj) noexcept
	{
		auto iter = std::forward<T>(obj).get_iterator();

		return iter.valid() ? iter : end(obj);
	}
}
