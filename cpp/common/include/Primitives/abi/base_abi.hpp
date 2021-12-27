#pragma once

#include "base.hpp"
#include "guid.hpp"
#include "meta.hpp"
#include "demangle.hpp"
#include "exceptions.hpp"
#include "param_string.hpp"
#include "g6_attributes.hpp"
#include "fundamental_semantics.hpp"

#include <cstddef>
#include <utility>
#include <type_traits>

namespace glasssix::exposing
{
	class unknown_object;

	void* get_abi(const unknown_object& object) noexcept;
	void** put_abi(unknown_object& object) noexcept;
}

namespace glasssix::exposing::impl
{
	template<typename To, typename From>
	To as(From* ptr);

	template<typename To, typename From>
	To try_as(From* ptr) noexcept;

	template<typename T, typename = void>
	struct abi;

	template<typename T>
	using abi_t = typename abi<T>::type;

	namespace details
	{
		template<typename T, typename = void>
		struct has_abi_type_top_level : std::false_type {};

		template<typename T>
		struct has_abi_type_top_level<T, std::void_t<abi_t<T>>> : std::true_type {};
	}

	template<typename T>
	struct has_abi_type : details::has_abi_type_top_level<T> {};

	template<template<typename...> typename T, typename... Args>
	struct has_abi_type<T<Args...>> : std::conjunction<details::has_abi_type_top_level<T<Args...>>, has_abi_type<Args>...> {};

	/// <summary>
	/// Checks whether a type contains a corresponding ABI type recursively.
	/// </summary>
	template<typename T>
	inline constexpr bool has_abi_type_v = has_abi_type<T>::value;

	/// <summary>
	/// The ABI is its own for a primitive type.
	/// </summary>
	template<typename T>
	struct abi<T, std::enable_if_t<is_primitive_v<T>>>
	{
		using type = T;
	};

	template<typename T>
	struct abi<T, std::enable_if_t<std::is_enum_v<T>>>
	{
		using type = std::underlying_type_t<T>;
	};

	/// <summary>
	/// The ABI of a param_string.
	/// </summary>
	template<> struct abi<param_string>
	{
		using identity_type = type_identity_primitive;
		using type = void*;

		static constexpr guid id{ "47534958-7061-7261-0605-737472696E67" };
	};

	/// <summary>
	/// The type identity of a ABI.
	/// </summary>
	template<typename T>
	struct type_identity<T, std::void_t<typename abi<T>::identity_type>>
	{
		using type = typename abi<T>::identity_type;
	};

	/// <summary>
	/// Checks whether a type is an ABI interface.
	/// </summary>
	template<typename T>
	struct is_well_defined_interface : std::conjunction<std::is_standard_layout<T>, has_abi_type<T>, std::is_base_of<unknown_object, T>> {};

	template<typename T>
	inline constexpr bool is_well_defined_interface_v = is_well_defined_interface<T>::value;

	template<typename T, typename = void>
	struct abi_in
	{
		using type = abi_t<T>;
	};

	/// <summary>
	/// An input argument of an ABI function.
	/// </summary>
	template<typename T>
	using abi_in_t = typename abi_in<T>::type;

	template<typename T>
	struct abi_in<T, std::enable_if_t<is_well_defined_interface_v<T>>>
	{
		using type = void*;
	};

	template<typename T>
	struct abi_out
	{
		using type = abi_in_t<T>*;
	};

	/// <summary>
	/// An output argument of an ABI function.
	/// </summary>
	template<typename T>
	using abi_out_t = typename abi_out<T>::type;

	/// <summary>
	/// Stores the GUID of a ABI.
	/// </summary>
	template<typename T>
	struct guid_storage<T, std::void_t<decltype(abi<T>::id)>>
	{
		static constexpr auto& value{ abi<T>::id };
	};

	/// <summary>
	/// Stores the GUID of a generic interface ABI.
	/// </summary>
	template<template<typename...> typename T, typename... Args>
	struct guid_storage<T<Args...>, std::void_t<decltype(abi<T<Args...>>::id)>>
	{
		static constexpr auto value{ create_guid_from_bytes(meta::concat_arrays(to_array(abi<T<Args...>>::id), type_signature_v<Args>...)) };
	};

	template<typename T, typename = void>
	struct null_value;

	template<typename T>
	struct null_value<T, std::enable_if_t<std::conjunction_v<std::is_convertible<std::nullptr_t, T>, std::negation<std::is_same<T, bool>>>>>
	{
		static constexpr std::nullptr_t value{};
	};

	template<typename T>
	struct null_value<T, std::enable_if_t<std::disjunction_v<is_primitive<T>, std::is_enum<T>>>>
	{
		static constexpr T value{};
	};

	/// <summary>
	/// Gets a null type of a specified type.
	/// </summary>
	template<typename T>
	inline constexpr auto& null_value_v = null_value<T>::value;

	/// <summary>
	/// The root interface ABI.
	/// </summary>
	template<> struct abi<unknown_object>
	{
		using identity_type = type_identity_interface;
		static constexpr guid id{ "00000000-0000-0000-C000-000000000046" };

		struct type
		{
			virtual std::int32_t G6_ABI_CALL query_interface(guid id, void** object) noexcept = 0;
			virtual std::uint32_t G6_ABI_CALL add_ref() noexcept = 0;
			virtual std::uint32_t G6_ABI_CALL release() noexcept = 0;
		};
	};

	using abi_unknown_object = abi_t<unknown_object>;
}

namespace glasssix::exposing
{
	/// <summary>
	/// A fundamental wrapper for the underlying ABI.
	/// </summary>
	class unknown_object
	{
	public:
		/// <summary>
		///  Creates an instance.
		/// </summary>
		unknown_object() noexcept : abi_{}
		{
		}

		/// <summary>
		/// Create an instance with an ABI from which ownership is taken.
		/// </summary>
		/// <param name="abi">The ABI</param>
		unknown_object(take_over_abi_from_void_ptr abi) noexcept : abi_{ abi.to<impl::abi_unknown_object*>() }
		{
		}

		/// <summary>
		/// Creates an instance with nullptr.
		/// </summary>
		unknown_object(std::nullptr_t) noexcept : unknown_object{}
		{
		}

		unknown_object(const unknown_object& other) noexcept : abi_{ other.abi_ }
		{
			add_ref();
		}

		unknown_object(unknown_object&& other) noexcept : abi_{ std::exchange(other.abi_, nullptr) }
		{
		}

		~unknown_object() noexcept
		{
			release();
		}

		unknown_object& operator=(const unknown_object& right) noexcept
		{
			if (this != &right)
			{
				release();
				abi_ = right.abi_;
				add_ref();
			}

			return *this;
		}

		unknown_object& operator=(unknown_object&& right) noexcept
		{
			if (this != &right)
			{
				release();
				abi_ = std::exchange(right.abi_, nullptr);
			}

			return *this;
		}

		unknown_object& operator=(std::nullptr_t) noexcept
		{
			release();

			return *this;
		}

		friend bool operator==(const unknown_object& left, const unknown_object& right) noexcept
		{
			return left.abi_ == right.abi_;
		}

		friend bool operator!=(const unknown_object& left, const unknown_object& right) noexcept
		{
			return !(left == right);
		}

		/// <summary>
		/// Disables allocations on the heap.
		/// </summary>
		void* operator new(std::size_t) = delete;

		/// <summary>
		/// Indicates whether the ABI is valid.
		/// </summary>
		/// <returns>True if the ABI is valid; otherwise false</returns>
		operator bool() const noexcept
		{
			return abi_;
		}

		/// <summary>
		/// Converts to another implemented interface.
		/// </summary>
		/// <typeparam name="To">The destination type</typeparam>
		/// <returns>The result</returns>
		template<typename To>
		auto as() const
		{
			return impl::as<To>(abi_);
		}

		/// <summary>
		/// Converts to another implemented interface without any exceptions to be thrown.
		/// </summary>
		/// <typeparam name="To">The destination type</typeparam>
		/// <returns>The result</returns>
		template<typename To>
		auto try_as() const noexcept
		{
			return impl::try_as<To>(abi_);
		}

		/// <summary>
		/// Swaps the ABIs between two objects.
		/// </summary>
		/// <param name="left">The left value</param>
		/// <param name="right">The right value</param>
		friend void swap(unknown_object& left, unknown_object& right) noexcept
		{
			std::swap(left.abi_, right.abi_);
		}
	private:
		void add_ref() const noexcept
		{
			if (abi_)
			{
				abi_->add_ref();
			}
		}

		void release() noexcept
		{
			if (abi_)
			{
				std::exchange(abi_, nullptr)->release();
			}
		}

		impl::abi_unknown_object* abi_;
	};

	/// <summary>
	/// Gets the ABI of an object with type information erased.
	/// </summary>
	/// <param name="object">The object</param>
	/// <returns>The ABI</returns>
	inline void* get_abi(const unknown_object& object) noexcept
	{
		return meta::get_standard_layout_first_member<impl::abi_unknown_object*>(object);
	}

	/// <summary>
	/// Gets the ABI of a primitive object.
	/// </summary>
	/// <typeparam name="T">The object type</typeparam>
	/// <param name="object">The object</param>
	/// <returns>The ABI</returns>
	template<typename T, std::enable_if_t<impl::is_primitive_v<T>>* = nullptr>
	auto get_abi(const T& object) noexcept
	{
		return impl::abi_t<T>(object);
	}

	template<typename T, std::enable_if_t<std::is_enum_v<T>>* = nullptr>
	auto get_abi(const T& object) noexcept
	{
		return static_cast<impl::abi_t<T>>(object);
	}

	/// <summary>
	/// Gets a pointer to the ABI of an object with type information erased.
	/// The ABI will not be cleared and the caller must ensure safety.
	/// </summary>
	/// <param name="object">The object</param>
	/// <returns>The pointer to the ABI</returns>
	inline void** put_abi_dangerous(unknown_object& object) noexcept
	{
		return reinterpret_cast<void**>(&meta::get_standard_layout_first_member<impl::abi_unknown_object*>(object));
	}

	/// <summary>
	/// Gets a pointer to the ABI of a primitive object.
	/// The ABI will not be cleared and the caller must ensure safety.
	/// </summary>
	/// <typeparam name="T">The object type</typeparam>
	/// <param name="object">The object</param>
	/// <returns>The pointer to the ABI</returns>
	template<typename T, std::enable_if_t<impl::is_primitive_v<T>>* = nullptr>
	auto put_abi_dangerous(T& object) noexcept
	{
		return &static_cast<impl::abi_t<T>&>(object);
	}

	/// <summary>
	/// Gets a pointer to the ABI of an enumeration.
	/// </summary>
	/// <typeparam name="T">The enumeration type</typeparam>
	/// <param name="object">The enumeration</param>
	/// <returns>The pointer to the ABI</returns>
	template<typename T, std::enable_if_t<std::is_enum_v<T>>* = nullptr>
	auto put_abi_dangerous(T& object) noexcept
	{
		return enum_ref{ object };
	}

	/// <summary>
	/// Gets a pointer to the ABI of an object with type information erased.
	/// </summary>
	/// <param name="object">The object</param>
	/// <returns>The pointer to the ABI</returns>
	inline void** put_abi(unknown_object& object) noexcept
	{
		return (object = nullptr, put_abi_dangerous(object));
	}

	/// <summary>
	/// Gets a pointer to the ABI of a primitive object.
	/// </summary>
	/// <typeparam name="T">The object type</typeparam>
	/// <param name="object">The object</param>
	/// <returns>The pointer to the ABI</returns>
	template<typename T, typename = std::enable_if_t<std::disjunction_v<impl::is_primitive<T>, std::is_enum<T>>>>
	auto put_abi(T& object) noexcept
	{
		return (object = {}, put_abi_dangerous(object));
	}

	/// <summary>
	/// Detaches the ABI from an object.
	/// </summary>
	/// <param name="object">The object</param>
	/// <returns>The ABI detached from the object</returns>
	template<typename T, std::enable_if_t<std::disjunction_v<impl::is_well_defined_interface<std::decay_t<T>>, std::is_null_pointer<T>>>* = nullptr>
	void* detach_abi(T&& object) noexcept
	{
		if constexpr (std::is_null_pointer_v<T>)
		{
			return nullptr;
		}
		else
		{
			// When the object is a named rvalue reference, we just pass it to the put_abi function as a lvalue reference (without perfect forwarding).
			// Thus, the ABI of the object is capable of being exchanged.
			return std::exchange(*put_abi_dangerous(object), nullptr);
		}
	}

	/// <summary>
	/// Detaches the ABI from a primitive object.
	/// </summary>
	/// <param name="object">The object</param>
	/// <returns>The ABI detached from the object</returns>
	template<typename T, std::enable_if_t<std::disjunction_v<impl::is_primitive<std::decay_t<T>>, std::is_enum<std::decay_t<T>>>>* = nullptr>
	auto detach_abi(T&& object) noexcept
	{
		return impl::abi_t<std::decay_t<T>>{ (std::forward<T>(object)) };
	}

	/// <summary>
	/// Creates an interface from an ABI.
	/// </summary>
	/// <typeparam name="T">The type</typeparam>
	/// <param name="abi">The ABI</param>
	/// <returns>The result</returns>
	template<typename T, std::enable_if_t<impl::is_well_defined_interface_v<T>>* = nullptr>
	T create_from_abi(void* abi) noexcept
	{
		if (abi)
		{
			static_cast<impl::abi_unknown_object*>(abi)->add_ref();
		}

		return T{ take_over_abi_from_void_ptr{ abi } };
	}

	/// <summary>
	/// Creates a primitive type from an ABI.
	/// </summary>
	/// <typeparam name="T">The primitive type</typeparam>
	/// <typeparam name="ABI">The ABI type</typeparam>
	/// <param name="abi">The ABI</param>
	/// <returns>The result</returns>
	template<typename T, typename Abi, typename = std::enable_if_t<std::conjunction_v<std::disjunction<impl::is_primitive<T>, std::is_enum<T>>, std::is_same<impl::abi_t<T>, std::decay_t<Abi>>>>>
	T create_from_abi(Abi&& abi) noexcept
	{
		return T{ std::forward<Abi>(abi) };
	}
}

namespace glasssix::exposing::impl
{
	namespace details
	{
		template<typename To, bool HasException, typename From, typename = std::enable_if_t<std::conjunction_v<std::is_base_of<abi_unknown_object, From>, is_well_defined_interface<To>>>>
		To as_impl(From* ptr)
		{
			To result{ nullptr };

			if (ptr == nullptr)
			{
				if constexpr (HasException)
				{
					throw abi_null_pointer{ format(FMT_STRING(u8"Failed to query an interface: {}. The source interface was null."), to_param_string(guid_of_v<To>)) };
				}
				else
				{
					return result;
				}
			}

			if (!abi_result{ ptr->query_interface(guid_of_v<To>, put_abi(result)) }.no_error())
			{
				if constexpr (HasException)
				{
					throw abi_no_interface{ format(FMT_STRING(u8"Failed to convert the object to {}."), to_param_string(guid_of_v<To>)) };
				}
			}

			return result;
		}
	}

	template<typename To, typename From>
	To as(From* ptr)
	{
		return details::as_impl<To, true>(ptr);
	}

	template<typename To, typename From>
	To try_as(From* ptr) noexcept
	{
		return details::as_impl<To, false>(ptr);
	}
}
