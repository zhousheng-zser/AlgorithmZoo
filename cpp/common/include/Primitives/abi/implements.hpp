#pragma once

#include "meta.hpp"
#include "base.hpp"
#include "base_abi.hpp"
#include "exceptions.hpp"
#include "param_string.hpp"
#include "g6_attributes.hpp"
#include "fundamental_semantics.hpp"

#include <new>
#include <tuple>
#include <array>
#include <atomic>
#include <memory>
#include <cstddef>
#include <utility>
#include <algorithm>
#include <type_traits>
#include <string_view>

namespace glasssix::exposing::impl
{
	template<typename Derived, typename Interface>
	class implements_interface_vtable;

	template<typename Derived, typename Interface, typename = void>
	struct interface_vtable_base;

	template<typename T, typename = void>
	struct is_inherits : std::false_type {};

	template<typename T>
	struct is_inherits<T, std::void_t<typename T::inherits_type>> : std::true_type {};

	/// <summary>
	/// Checks whether a type is an "inherits" type.
	/// </summary>
	/// <typeparam name="T">The type</typeparam>
	template<typename T>
	inline constexpr bool is_inherits_v = is_inherits<T>::value;

	template<typename T, typename = void>
	struct is_implements : std::false_type {};

	template<typename T>
	struct is_implements<T, std::void_t<typename T::implements_type>> : std::true_type {};

	/// <summary>
	/// Checks whether a type is an "implements" type.
	/// </summary>
	/// <typeparam name="T">The type</typeparam>
	template<typename T>
	inline constexpr bool is_implements_v = is_implements<T>::value;

	template<typename T>
	struct get_inherits
	{
		using type = typename T::inherits_type;
	};

	template<typename T>
	using get_inherits_t = typename get_inherits<T>::type;

	template<typename T>
	struct get_implements
	{
		using type = typename T::implements_type;
	};

	template<typename T>
	using get_implements_t = typename get_implements<T>::type;

	namespace details
	{
		template<typename Derived, typename T>
		struct implements_interface_vtables_impl;

		template<typename Derived, typename... Interfaces>
		struct implements_interface_vtables_impl<Derived, std::tuple<Interfaces...>> : implements_interface_vtable<Derived, Interfaces>...
		{
		};

		template<typename ImplementsOrInherits>
		struct get_top_level_interfaces;

		template<template<typename, typename...> typename ImplementsOrInherits, typename Derived, typename... Interfaces>
		struct get_top_level_interfaces<ImplementsOrInherits<Derived, Interfaces...>>
		{
			using type = meta::tuple_if_t<is_well_defined_interface, std::tuple<Interfaces...>>;
		};

		/// <summary>
		/// Creates a tuple to accommodate the implemented or inherited interfaces at the top lovel (not recursively).
		/// </summary>
		template<typename ImplementsOrInherits>
		using get_top_level_interfaces_t = typename get_top_level_interfaces<ImplementsOrInherits>::type;

		template<typename Tuple>
		struct get_interfaces_recursively_impl;

		template<typename... Interfaces>
		struct get_interfaces_recursively_impl<std::tuple<Interfaces...>>
		{
			using packed_type = meta::tuple_cat_t<get_top_level_interfaces_t<get_inherits_t<Interfaces>>...>;
			using type = meta::tuple_unique_t<meta::tuple_cat_t<packed_type, typename get_interfaces_recursively_impl<meta::tuple_select_t<get_inherits, packed_type>>::type>>;
		};

		template<> struct get_interfaces_recursively_impl<std::tuple<>>
		{
			using type = std::tuple<>;
		};

		template<typename Tuple>
		using get_interfaces_recursively_impl_t = typename get_interfaces_recursively_impl<Tuple>::type;

		template<typename Tuple>
		struct get_interfaces_recursively;

		template<typename... Args>
		struct get_interfaces_recursively<std::tuple<Args...>>
		{
			using type = meta::tuple_unique_t<meta::tuple_cat_t<std::tuple<Args...>, get_interfaces_recursively_impl_t<std::tuple<Args...>>>>;
		};

		/// <summary>
		/// Gets all interfaces recursively.
		/// </summary>
		template<typename Tuple>
		using get_interfaces_recursively_t = typename get_interfaces_recursively<Tuple>::type;

		template<typename Derived>
		struct get_implemented_interfaces_recursively
		{
			using type = get_interfaces_recursively_t<get_top_level_interfaces_t<get_implements_t<Derived>>>;
		};

		/// <summary>
		/// Gets the implemented interfaces of a "implements" type recursively.
		/// </summary>
		template<typename Derived>
		using get_implemented_interfaces_recursively_t = typename get_implemented_interfaces_recursively<Derived>::type;
	}

	template<typename T, typename = void>
	struct has_unique_release : std::false_type {};

	template<typename T>
	struct has_unique_release < T, std::void_t<decltype(T::unique_release(std::unique_ptr<T>{})) >> : std::true_type{};

	/// <summary>
	/// Checks whether a type contains a static function named unique_release.
	/// </summary>
	/// <typeparam name="T">The type</typeparam>
	template<typename T>
	inline constexpr bool has_unique_release_v = has_unique_release<T>::value;

	template<typename T, typename = void>
	struct has_external_qualified_name : std::false_type {};

	template<typename T>
	struct has_external_qualified_name<T, std::void_t<decltype(T::external_qualified_name_type::value)>> : std::true_type {};

	/// <summary>
	/// Checks whether a type contains an external qualified name.
	/// </summary>
	/// <typeparam name="T">The type</typeparam>
	template<typename T>
	inline constexpr auto& has_external_qualified_name_v = has_external_qualified_name<T>::value;

	template<typename T, typename = void>
	struct get_external_qualified_name;

	template<typename T>
	struct get_external_qualified_name<T, std::enable_if_t<has_external_qualified_name_v<T>>>
	{
		static constexpr auto& value = T::external_qualified_name_type::value;
	};

	/// <summary>
	/// Retrieves the external qualified name of a type if exists.
	/// </summary>
	/// <typeparam name="T">The type</typeparam>
	template<typename T>
	inline constexpr auto& get_external_qualified_name_v = get_external_qualified_name<T>::value;

	/// <summary>
	/// The unknown_object vtable of an interface ABI, which forwards all calls to the derived type.
	/// </summary>
	template<typename Derived, typename Interface>
	struct interface_vtable_base<Derived, Interface, std::enable_if_t<is_well_defined_interface_v<Interface>>> : abi_t<Interface>, std::tuple<void*>
	{
		Derived& self() noexcept
		{
			// For C++ conformance, put workaround here to avoid reinterpret_cast between non-standard-layout objects.
			return static_cast<Derived&>(*static_cast<implements_interface_vtable<Derived, Interface>*>(std::get<void*>(*this)));
		}

		virtual std::int32_t G6_ABI_CALL query_interface(guid id, void** object) noexcept override
		{
			return self().query_interface(id, object);
		}

		virtual std::uint32_t G6_ABI_CALL add_ref() noexcept override
		{
			return self().add_ref();
		}

		virtual std::uint32_t G6_ABI_CALL release() noexcept override
		{
			return self().release();
		}
	};

	/// <summary>
	/// The vtable of an interface ABI, which forwards all calls to the derived type.
	/// </summary>
	template<typename Derived, typename Interface>
	struct interface_vtable : interface_vtable_base<Derived, Interface>
	{
	};

	/// <summary>
	/// A reference to a interfacial vtable.
	/// </summary>
	template<typename Interface>
	struct interface_vtable_ref : Interface
	{
		interface_vtable_ref(void* ptr) noexcept : Interface{ nullptr }
		{
			*put_abi(*this) = ptr;
		}

		~interface_vtable_ref() noexcept
		{
			detach_abi(*this);
		}

		interface_vtable_ref(const interface_vtable_ref&) noexcept = delete;
		interface_vtable_ref(interface_vtable_ref&&) noexcept = delete;
		interface_vtable_ref& operator=(const interface_vtable_ref&) noexcept = delete;
		interface_vtable_ref& operator=(interface_vtable_ref&&) noexcept = delete;
		void* operator new(std::size_t) = delete;
	};

	/// <summary>
	/// Implements a vtable for an interfacial ABI.
	/// </summary>
	template<typename Derived, typename Interface>
	class implements_interface_vtable
	{
	public:
		using vtable_type = interface_vtable<Derived, Interface>;
		friend vtable_type;

		template<typename OtherInterface, typename OtherDerived, typename>
		friend constexpr auto to_abi(OtherDerived& derived) noexcept;

		implements_interface_vtable() noexcept
		{
			std::get<void*>(vtable_) = this;
		}

		/// <summary>
		/// Converts to one of the implemented interfaces.
		/// </summary>
		/// <returns>The interface</returns>
		operator interface_vtable_ref<Interface>() const noexcept
		{
			return interface_vtable_ref<Interface>{ const_cast<vtable_type*>(&vtable_) };
		}
	private:
		vtable_type vtable_;
	};

	/// <summary>
	/// Retrieves an interfacial ABI from a derived object.
	/// </summary>
	/// <typeparam name="OtherInterface">The interfacial type</typeparam>
	/// <typeparam name="OtherDerived">The derived type</typeparam>
	/// <param name="derived">The derived object</param>
	/// <returns>The ABI</returns>
	template<typename OtherInterface, typename OtherDerived, typename = std::enable_if_t<std::conjunction_v<is_implements<OtherDerived>, is_well_defined_interface<OtherInterface>>>>
	constexpr auto to_abi(OtherDerived& derived) noexcept
	{
		return static_cast<abi_t<OtherInterface>*>(&static_cast<implements_interface_vtable<OtherDerived, OtherInterface>&>(derived).vtable_);
	}

	/// <summary>
	/// Implements vtables for interfacial ABIs.
	/// </summary>
	template<typename Derived, typename... Interfaces>
	using implements_interface_vtables = details::implements_interface_vtables_impl<Derived, details::get_interfaces_recursively_t<meta::tuple_if_t<is_well_defined_interface, std::tuple<Interfaces...>>>>;

	template<typename Derived, typename = void>
	struct first_interface;

	template<typename Derived>
	struct first_interface<Derived, std::enable_if_t<is_implements_v<Derived>>>
	{
		using type = meta::tuple_first_t<details::get_implemented_interfaces_recursively_t<Derived>>;
	};

	/// <summary>
	/// Gets the first interface derived from glasssix::exposing::unknown_object.
	/// </summary>
	template<typename Derived>
	using first_interface_t = typename first_interface<Derived>::type;

	template<typename Derived, typename = void>
	class find_interface_by_guid;

	/// <summary>
	/// Finds an interface ABI by GUID.
	/// </summary>
	template<typename Derived>
	class find_interface_by_guid<Derived, std::enable_if_t<is_implements_v<Derived>>>
	{
	public:
		using packed_type = details::get_implemented_interfaces_recursively_t<Derived>;

		/// <summary>
		/// Gets the ABI of an interface by specified ID.
		/// </summary>
		/// <param name="derived">The derived object</param>
		/// <param name="id">The ID</param>
		/// <returns>The ABI</returns>
		static void* get(Derived& derived, const guid& id) noexcept
		{
			return get_impl(derived, id, std::make_index_sequence<std::tuple_size_v<packed_type>>{});
		}
	private:
		template<std::size_t... Indexes>
		static void* get_impl(Derived& derived, const guid& id, std::index_sequence<Indexes...>) noexcept
		{
			static constexpr std::array guids{ guid_of_v<std::tuple_element_t<Indexes, packed_type>>... };
			void* result = nullptr;

			return ((guids[Indexes] == id ? static_cast<void>(result = to_abi<std::tuple_element_t<Indexes, packed_type>>(derived)) : void()), ..., result);
		}
	};

	template<typename Interface, typename = std::enable_if_t<has_abi_type_v<Interface>>>
	struct abi_adapter;

	/// <summary>
	/// The ABI adapter for an interface.
	/// </summary>
	template<typename Derived, typename Interface>
	using abi_adapter_t = typename abi_adapter<Interface>::template type<Derived>;

	/// <summary>
	/// A helper class that enables casting to the derived type of an interface.
	/// </summary>
	template<typename Derived, typename Interface, typename = std::enable_if_t<has_abi_type_v<Interface>>>
	struct enable_self_abi_awareness
	{
		decltype(auto) self_abi() const
		{
			if (auto ptr = static_cast<abi_t<Interface>*>(get_abi(static_cast<const Interface&>(static_cast<const Derived&>(*this)))))
			{
				return *ptr;
			}

			throw abi_null_pointer{ format(FMT_STRING(u8"Cannot invoke a method of a null interface: {}."), to_param_string(guid_of_v<Interface>)) };
		}
	};

	/// <summary>
	/// Inherits a ABI adapter for an interface.
	/// </summary>
	template<typename Derived, typename Interface>
	struct inherits_abi_adapter : abi_adapter_t<Derived, Interface>
	{
		operator Interface() const noexcept
		{
			return static_cast<const Derived&>(*this).template try_as<Interface>();
		}
	};

	namespace details
	{
		template<typename Derived, typename T>
		struct inherits_abi_adapters_impl;

		template<typename Derived, typename... Interfaces>
		struct inherits_abi_adapters_impl<Derived, std::tuple<Interfaces...>> : inherits_abi_adapter<Derived, Interfaces>...
		{
		};
	}

	/// <summary>
	/// Inherits a ABI adapter for an interface recursively (including all implicitly inherited ones).
	/// </summary>
	template<typename Derived, typename... Interfaces>
	struct inherits_abi_adapters : details::inherits_abi_adapters_impl<Derived, details::get_interfaces_recursively_t<meta::tuple_if_t<is_well_defined_interface, std::tuple<Interfaces...>>>>
	{
	};

	/// <summary>
	/// A class that implements the fundamental functions of glasssix::exposing::unknown_object.
	/// </summary>
	template<typename Derived>
	class G6_NOVTABLE unknown_object_impl
	{
	public:
		unknown_object_impl() noexcept : ref_count_{ 1 }
		{
			++get_module_ref_count();
		}

		virtual ~unknown_object_impl() noexcept
		{
			--get_module_ref_count();
		}

		std::int32_t G6_ABI_CALL query_interface(const guid& id, void** object) noexcept
		{
			if (object == nullptr)
			{
				return error_null_pointer;
			}

			if ((*object = find_interface_by_guid<Derived>::get(static_cast<Derived&>(*this), id)))
			{
				add_ref();

				return error_success;
			}

			// Provides the implementation of the first interface.
			if (is_guid_of<exposing::unknown_object>(id))
			{
				using first_interface_type = first_interface_t<Derived>;

				*object = static_cast<impl::abi_unknown_object*>(to_abi<first_interface_type>(static_cast<Derived&>(*this)));
				add_ref();

				return error_success;
			}

			return error_no_interface;
		}

		std::uint32_t G6_ABI_CALL add_ref() noexcept
		{
			return ++ref_count_;
		}

		std::uint32_t G6_ABI_CALL release() noexcept
		{
			std::uint32_t count = --ref_count_;

			if (count == 0)
			{
				// Call the unique_release routine for deferred operations if the derived type defines it.
				if constexpr (has_unique_release_v<Derived>)
				{
					ref_count_ = 1;
					Derived::unique_release(std::unique_ptr<Derived>{ static_cast<Derived*>(this) });
				}
				else
				{
					delete this;
				}
			}

			return count;
		}
	private:
		atomic_ref_count ref_count_;
	};
}

namespace glasssix::exposing
{
	/// <summary>
	/// A helper class to generate standard ABI implementations for a derived class.
	/// </summary>
	template<typename Derived, typename... Interfaces>
	struct implements : impl::implements_interface_vtables<Derived, Interfaces...>, impl::unknown_object_impl<Derived>
	{
		using implements_type = implements;
		using root_implements_type = impl::unknown_object_impl<Derived>;

		std::int32_t G6_ABI_CALL query_interface(const guid& id, void** object) noexcept
		{
			return root_implements_type::query_interface(id, object);
		}

		std::uint32_t G6_ABI_CALL add_ref() noexcept
		{
			return root_implements_type::add_ref();
		}

		std::uint32_t G6_ABI_CALL release() noexcept
		{
			return root_implements_type::release();
		}
	};

	/// <summary>
	/// A helper class to support implicitly casting to one or more interfaces.
	/// </summary>
	template<typename Derived, typename... Interfaces>
	struct inherits : unknown_object, impl::abi_adapter_t<Derived, Derived>, impl::inherits_abi_adapters<Derived, Interfaces...>
	{
		using inherits_type = inherits;

		/// <summary>
		/// Creates an instance with nullptr.
		/// </summary>
		inherits(std::nullptr_t = nullptr) noexcept : unknown_object{ nullptr }
		{
		}

		/// <summary>
		/// Create an instance with an ABI from which ownership is taken.
		/// </summary>
		/// <param name="abi">The ABI</param>
		inherits(take_over_abi_from_void_ptr abi) noexcept : unknown_object{ abi }
		{
		}
	};

	/// <summary>
	/// A helper class that makes the external qualified name of an implementation.
	/// </summary>
	template<const utf8_string_view& name>
	struct make_external_qualified_name
	{
		struct external_qualified_name_type
		{
			static constexpr utf8_string_view value = name;
		};
	};

	/// <summary>
	/// Creates an in-process instance of an implementation type.
	/// </summary>
	/// <typeparam name="T">The implementation type</typeparam>
	/// <typeparam name="Interface">The interfacial type</typeparam>
	/// <typeparam name="...Args">The types of arguments</typeparam>
	/// <param name="...args">The arguments</param>
	/// <returns>The instance</returns>
	template<typename T, typename Interface, typename... Args, typename = std::enable_if_t<std::conjunction_v<impl::is_implements<T>, impl::is_well_defined_interface<Interface>>>>
	auto make(Args&&... args)
	{
		return Interface{ take_over_abi_from_void_ptr{ impl::to_abi<Interface>(*new T(std::forward<Args>(args)...)) } };
	}

	/// <summary>
	/// Creates an in-process instance of an implementation type and returns the first interface.
	/// </summary>
	/// <typeparam name="T">The implementation type</typeparam>
	/// <typeparam name="...Args">The types of arguments</typeparam>
	/// <param name="...args">The arguments</param>
	/// <returns>The instance</returns>
	template<typename T, typename... Args, typename = std::enable_if_t<impl::is_implements_v<T>>>
	auto make_as_first(Args&&... args)
	{
		return make<T, impl::first_interface_t<T>>(std::forward<Args>(args)...);
	}
}
