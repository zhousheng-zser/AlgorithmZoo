#pragma once

#include "base.hpp"
#include "base_abi.hpp"
#include "implements.hpp"
#include "exceptions.hpp"
#include "param_span.hpp"
#include "param_string.hpp"
#include "param_vector.hpp"

#include <new>
#include <type_traits>

namespace glasssix::exposing
{
	template<typename T>
	struct box_value;
}

namespace glasssix::exposing::impl
{
	template<typename T>
	struct abi<box_value<T>>
	{
		using identity_type = type_identity_generic_interface;
		static constexpr guid id{ "CEAEA735-BA42-4B48-96B3-C2F9BAA4F5E2" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL has_value(abi_out_t<bool> value) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL get(abi_out_t<T> value) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set(abi_in_t<T> value) noexcept = 0;
		};
	};

	template<typename Derived, typename T>
	struct interface_vtable<Derived, box_value<T>> : interface_vtable_base<Derived, box_value<T>>
	{
		virtual std::int32_t G6_ABI_CALL has_value(abi_out_t<bool> value) noexcept override
		{
			return abi_safe_call([&] { *value = detach_abi(this->self().has_value()); });
		}

		virtual std::int32_t G6_ABI_CALL get(abi_out_t<T> value) noexcept override
		{
			return abi_safe_call([&] { *value = detach_abi(this->self().get()); });
		}

		virtual std::int32_t G6_ABI_CALL set(abi_in_t<T> value) noexcept override
		{
			return abi_safe_call([&] { this->self().set(create_from_abi<T>(value)); });
		}
	};

	template<typename T>
	struct abi_adapter<box_value<T>>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, box_value<T>>
		{
			bool has_value() const
			{
				bool result = false;

				return (check_abi_result(this->self_abi().has_value(put_abi(result))), result);
			}

			T get() const
			{
				T result{ null_value_v<T> };

				return (check_abi_result(this->self_abi().get(put_abi(result))), result);
			}

			void set(const T& value) const
			{
				check_abi_result(this->self_abi().set(get_abi(value)));
			}
		};
	};
}

namespace glasssix::exposing
{
	template<typename T>
	struct box_value : inherits<box_value<T>>
	{
		using inherits<box_value<T>>::inherits;
	};
}

namespace glasssix::exposing::impl
{
	template<typename T>
	class box_value_impl : public implements<box_value_impl<T>, box_value<T>>
	{
	public:
		box_value_impl() : initialized_{}
		{
		}

		box_value_impl(const T& value) : initialized_{}
		{
			set(value);
		}

		~box_value_impl()
		{
			if (initialized_)
			{
				std::launder(reinterpret_cast<const T*>(&buffer_))->~T();
			}
		}

		bool has_value() const
		{
			return initialized_;
		}

		T get() const
		{
			return initialized_ ? *std::launder(reinterpret_cast<const T*>(&buffer_)) : throw abi_not_initialized{};
		}

		void set(const T& value)
		{
			new (&buffer_) T{ value };
			initialized_ = true;
		}
	private:
		bool initialized_;
		std::aligned_storage_t<sizeof(T), alignof(T)> buffer_;
	};
}

namespace glasssix::exposing
{
	/// <summary>
	/// Boxes a primitive value or a string into an unknown_object.
	/// </summary>
	/// <typeparam name="T">The primitive type or string type</typeparam>
	/// <param name="value">The value</param>
	/// <returns>The boxed value</returns>
	template<typename T, typename = std::enable_if_t<std::disjunction_v<impl::is_primitive<std::decay_t<T>>, std::is_convertible<T, param_string>, impl::is_param_span<std::decay_t<T>>>>>
	auto box(T&& value)
	{
		using boxed_type = std::conditional_t<std::is_convertible_v<T, param_string>, param_string, std::decay_t<T>>;

		return make_as_first<impl::box_value_impl<boxed_type>>(std::forward<T>(value));
	}

	/// <summary>
	/// Unboxes a value.
	/// </summary>
	/// <typeparam name="T">The primitive type or string type</typeparam>
	/// <param name="obj">The object</param>
	/// <returns>The unboxed value</returns>
	template<typename T, typename = std::enable_if_t<std::disjunction_v<impl::is_primitive<T>, std::is_same<T, param_string>, impl::is_param_span<T>>>>
	T unbox(const unknown_object& obj)
	{
		return obj.as<box_value<T>>().get();
	}
}
