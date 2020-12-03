#pragma once

#include "base.hpp"
#include "base_abi.hpp"
#include "implements.hpp"
#include "dllexport.hpp"
#include "exceptions.hpp"
#include "param_vector.hpp"
#include "param_string.hpp"
#include "g6_attributes.hpp"
#include "class_factory.hpp"
#include "param_hash_map.hpp"
#include "pure_c_handle_utils.h"

#include <utility>
#include <type_traits>

namespace glasssix::exposing::dll
{
	DEFINE_PURE_C_HANDLE(dll);

	using symbol_func_ptr = void(*)();

	extern "C" EXPORT_EXCALIBUR_PRIMITIVES dll_handle G6_ABI_CALL load_library(const utf8_char * path) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void G6_ABI_CALL free_library(dll_handle handle) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES symbol_func_ptr G6_ABI_CALL get_symbol_address(dll_handle handle, const utf8_char * name) noexcept;
}

namespace glasssix::exposing::dll_routines
{
	using dll_can_unload_now_handler_type = bool(G6_ABI_CALL*)() noexcept;
	using dll_create_factory_handler_type = std::int32_t(G6_ABI_CALL*)(void** factory) noexcept;

	inline constexpr utf8_string_view dll_can_unload_now_handler_name{ u8"dll_can_unload_now" };
	inline constexpr utf8_string_view dll_create_factory_handler_name{ u8"dll_create_factory" };
}

namespace glasssix::exposing
{
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void* G6_ABI_CALL glasssix_add_ref_get_component_loader_abi();

	struct component_loader;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<component_loader>
	{
		using identity_type = type_identity_interface;
		static constexpr guid id{ "E510FD23-0134-45D3-8801-862E9F199536" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL add_module(abi_in_t<param_string> path, abi_out_t<bool> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_module_with_factory(abi_in_t<param_string> path, abi_out_t<class_factory> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_modules(abi_in_t<param_span<param_string>> paths, abi_out_t<std::uint64_t> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_modules(abi_in_t<param_vector<param_string>> paths, abi_out_t<std::uint64_t> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_modules_in_directory(abi_in_t<param_string> directory, abi_in_t<bool> recursive, abi_out_t<std::uint64_t> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_modules_with_factories(abi_in_t<param_span<param_string>> paths, abi_out_t<param_hash_map<param_string, class_factory>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_modules_with_factories(abi_in_t<param_vector<param_string>> paths, abi_out_t<param_hash_map<param_string, class_factory>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_modules_with_factories_in_directory(abi_in_t<param_string> directory, abi_in_t<bool> recursive, abi_out_t<param_hash_map<param_string, class_factory>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_module_by_name(abi_in_t<param_string> name, abi_out_t<bool> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_module_by_name_with_factory(abi_in_t<param_string> name, abi_out_t<class_factory> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_modules_by_name(abi_in_t<param_span<param_string>> names, abi_out_t<std::uint64_t> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_modules_by_name(abi_in_t<param_vector<param_string>> names, abi_out_t<std::uint64_t> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_modules_by_name_with_factories(abi_in_t<param_span<param_string>> names, abi_out_t<param_hash_map<param_string, class_factory>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL add_modules_by_name_with_factories(abi_in_t<param_vector<param_string>> names, abi_out_t<param_hash_map<param_string, class_factory>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL lookup_factory(abi_in_t<param_string> library_name, abi_out_t<class_factory> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL library_names(abi_out_t<param_vector<param_string>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL factories(abi_out_t<param_hash_map<param_string, class_factory>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL contains_qualified_name(abi_in_t<param_string> qualified_name, abi_out_t<bool> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL contains_interface_id(abi_in_t<guid> interface_id, abi_out_t<bool> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL create_by_name(abi_in_t<param_string> qualified_name, abi_out_t<unknown_object> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL create_by_interface_id(abi_in_t<guid> interface_id, abi_out_t<unknown_object> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, component_loader> : interface_vtable_base<Derived, component_loader>
	{
		virtual std::int32_t G6_ABI_CALL add_module(abi_in_t<param_string> path, abi_out_t<bool> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().add_module(create_from_abi<param_string>(path))); });
		}

		virtual std::int32_t G6_ABI_CALL add_module_with_factory(abi_in_t<param_string> path, abi_out_t<class_factory> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().add_module_with_factory(create_from_abi<param_string>(path))); });
		}

		virtual std::int32_t G6_ABI_CALL add_modules(abi_in_t<param_span<param_string>> paths, abi_out_t<std::uint64_t> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().add_modules(create_from_abi<param_span<param_string>>(paths))); });
		}

		virtual std::int32_t G6_ABI_CALL add_modules(abi_in_t<param_vector<param_string>> paths, abi_out_t<std::uint64_t> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().add_modules(create_from_abi<param_vector<param_string>>(paths))); });
		}

		virtual std::int32_t G6_ABI_CALL add_modules_in_directory(abi_in_t<param_string> directory, abi_in_t<bool> recursive, abi_out_t<std::uint64_t> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().add_modules_in_directory(create_from_abi<param_string>(directory), recursive)); });
		}

		virtual std::int32_t G6_ABI_CALL add_modules_with_factories(abi_in_t<param_span<param_string>> paths, abi_out_t<param_hash_map<param_string, class_factory>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().add_modules_with_factories(create_from_abi<param_span<param_string>>(paths))); });
		}

		virtual std::int32_t G6_ABI_CALL add_modules_with_factories(abi_in_t<param_vector<param_string>> paths, abi_out_t<param_hash_map<param_string, class_factory>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().add_modules_with_factories(create_from_abi<param_vector<param_string>>(paths))); });
		}

		virtual std::int32_t G6_ABI_CALL add_modules_with_factories_in_directory(abi_in_t<param_string> directory, abi_in_t<bool> recursive, abi_out_t<param_hash_map<param_string, class_factory>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().add_modules_with_factories_in_directory(create_from_abi<param_string>(directory), recursive)); });
		}

		virtual std::int32_t G6_ABI_CALL add_module_by_name(abi_in_t<param_string> name, abi_out_t<bool> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().add_module_by_name(create_from_abi<param_string>(name))); });
		}

		virtual std::int32_t G6_ABI_CALL add_module_by_name_with_factory(abi_in_t<param_string> name, abi_out_t<class_factory> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().add_module_by_name_with_factory(create_from_abi<param_string>(name))); });
		}

		virtual std::int32_t G6_ABI_CALL add_modules_by_name(abi_in_t<param_span<param_string>> names, abi_out_t<std::uint64_t> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().add_modules_by_name(create_from_abi<param_span<param_string>>(names))); });
		}

		virtual std::int32_t G6_ABI_CALL add_modules_by_name(abi_in_t<param_vector<param_string>> names, abi_out_t<std::uint64_t> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().add_modules_by_name(create_from_abi<param_vector<param_string>>(names))); });
		}

		virtual std::int32_t G6_ABI_CALL add_modules_by_name_with_factories(abi_in_t<param_span<param_string>> names, abi_out_t<param_hash_map<param_string, class_factory>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().add_modules_by_name_with_factories(create_from_abi<param_span<param_string>>(names))); });
		}

		virtual std::int32_t G6_ABI_CALL add_modules_by_name_with_factories(abi_in_t<param_vector<param_string>> names, abi_out_t<param_hash_map<param_string, class_factory>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().add_modules_by_name_with_factories(create_from_abi<param_vector<param_string>>(names))); });
		}

		virtual std::int32_t G6_ABI_CALL lookup_factory(abi_in_t<param_string> library_name, abi_out_t<class_factory> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().lookup_factory(create_from_abi<param_string>(library_name))); });
		}

		virtual std::int32_t G6_ABI_CALL library_names(abi_out_t<param_vector<param_string>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().library_names()); });
		}

		virtual std::int32_t G6_ABI_CALL factories(abi_out_t<param_hash_map<param_string, class_factory>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().factories()); });
		}

		virtual std::int32_t G6_ABI_CALL contains_qualified_name(abi_in_t<param_string> qualified_name, abi_out_t<bool> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().contains_qualified_name(create_from_abi<param_string>(qualified_name))); });
		}

		virtual std::int32_t G6_ABI_CALL contains_interface_id(abi_in_t<guid> interface_id, abi_out_t<bool> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().contains_interface_id(create_from_abi<guid>(interface_id))); });
		}

		virtual std::int32_t G6_ABI_CALL create_by_name(abi_in_t<param_string> qualified_name, abi_out_t<unknown_object> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().create_by_name(create_from_abi<param_string>(qualified_name))); });
		}

		virtual std::int32_t G6_ABI_CALL create_by_interface_id(abi_in_t<guid> interface_id, abi_out_t<unknown_object> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().create_by_interface_id(create_from_abi<guid>(interface_id))); });
		}
	};

	template<> struct abi_adapter<component_loader>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, component_loader>
		{
			param_hash_map<guid, param_string> existing_instances() const
			{
				param_hash_map<guid, param_string> result{ nullptr };

				return (check_abi_result(this->self_abi().existing_instances(put_abi(result))), result);
			}

			bool add_module(const param_string& path) const
			{
				bool result{};

				return (check_abi_result(this->self_abi().add_module(get_abi(path), put_abi(result))), result);
			}

			class_factory add_module_with_factory(const param_string& path) const
			{
				class_factory result{ nullptr };

				return (check_abi_result(this->self_abi().add_module_with_factory(get_abi(path), put_abi(result))), result);
			}

			std::uint64_t add_modules(const param_span<param_string>& paths) const
			{
				std::uint64_t result{};

				return (check_abi_result(this->self_abi().add_modules(get_abi(paths), put_abi(result))), result);
			}

			std::uint64_t add_modules(const param_vector<param_string>& paths) const
			{
				std::uint64_t result{};

				return (check_abi_result(this->self_abi().add_modules(get_abi(paths), put_abi(result))), result);
			}

			std::uint64_t add_modules_in_directory(const param_string& directory, bool recursive = false) const
			{
				std::uint64_t result{};

				return (check_abi_result(this->self_abi().add_modules_in_directory(get_abi(directory), get_abi(recursive), put_abi(result))), result);
			}

			param_hash_map<param_string, class_factory> add_modules_with_factories(const param_span<param_string>& paths) const
			{
				param_hash_map<param_string, class_factory> result{ nullptr };

				return (check_abi_result(this->self_abi().add_modules_with_factories(get_abi(paths), put_abi(result))), result);
			}

			param_hash_map<param_string, class_factory> add_modules_with_factories(const param_vector<param_string>& paths) const
			{
				param_hash_map<param_string, class_factory> result{ nullptr };

				return (check_abi_result(this->self_abi().add_modules_with_factories(get_abi(paths), put_abi(result))), result);
			}

			param_hash_map<param_string, class_factory> add_modules_with_factories_in_directory(const param_string& directory, bool recursive = false) const
			{
				param_hash_map<param_string, class_factory> result{ nullptr };

				return (check_abi_result(this->self_abi().add_modules_with_factories_in_directory(get_abi(directory), get_abi(recursive), put_abi(result))), result);
			}

			bool add_module_by_name(const param_string& name) const
			{
				bool result{};

				return (check_abi_result(this->self_abi().add_module_by_name(get_abi(name), put_abi(result))), result);
			}

			class_factory add_module_by_name_with_factory(const param_string& name) const
			{
				class_factory result{ nullptr };

				return (check_abi_result(this->self_abi().add_module_by_name_with_factory(get_abi(name), put_abi(result))), result);
			}

			std::uint64_t add_modules_by_name(const param_span<param_string>& names) const
			{
				std::uint64_t result{};

				return (check_abi_result(this->self_abi().add_modules_by_name(get_abi(names), put_abi(result))), result);
			}

			std::uint64_t add_modules_by_name(const param_vector<param_string>& names) const
			{
				std::uint64_t result{};

				return (check_abi_result(this->self_abi().add_modules_by_name(get_abi(names), put_abi(result))), result);
			}

			param_hash_map<param_string, class_factory> add_modules_by_name_with_factories(const param_span<param_string>& names) const
			{
				param_hash_map<param_string, class_factory> result{ nullptr };

				return (check_abi_result(this->self_abi().add_modules_by_name_with_factories(get_abi(names), put_abi(result))), result);
			}

			param_hash_map<param_string, class_factory> add_modules_by_name_with_factories(const param_vector<param_string>& names) const
			{
				param_hash_map<param_string, class_factory> result{ nullptr };

				return (check_abi_result(this->self_abi().add_modules_by_name_with_factories(get_abi(names), put_abi(result))), result);
			}

			class_factory lookup_factory(const param_string& library_name) const
			{
				class_factory result{ nullptr };

				return (check_abi_result(this->self_abi().lookup_factory(get_abi(library_name), put_abi(result))), result);
			}

			param_vector<param_string> library_names() const
			{
				param_vector<param_string> result{ nullptr };

				return (check_abi_result(this->self_abi().library_names(put_abi(result))), result);
			}

			param_hash_map<param_string, class_factory> factories() const
			{
				param_hash_map<param_string, class_factory> result{ nullptr };

				return (check_abi_result(this->self_abi().factories(put_abi(result))), result);
			}

			bool contains_qualified_name(const param_string& qualified_name) const
			{
				bool result{};

				return (check_abi_result(this->self_abi().contains_qualified_name(get_abi(qualified_name), put_abi(result))), result);
			}

			bool contains_interface_id(const guid& interface_id) const
			{
				bool result{};

				return (check_abi_result(this->self_abi().contains_interface_id(get_abi(interface_id), put_abi(result))), result);
			}

			unknown_object create_by_name(const param_string& qualified_name) const
			{
				unknown_object result{ nullptr };

				return (check_abi_result(this->self_abi().contains_interface_id(get_abi(qualified_name), put_abi(result))), result);
			}

			unknown_object create_by_interface_id(const guid& interface_id) const
			{
				unknown_object result{ nullptr };

				return (check_abi_result(this->self_abi().create_by_interface_id(get_abi(interface_id), put_abi(result))), result);
			}
		};
	};
}

namespace glasssix::exposing
{
	struct component_loader : inherits<component_loader>
	{
		using inherits::inherits;
	};
}

namespace glasssix::exposing
{
	/// <summary>
	/// Gets the component loader singleton.
	/// </summary>
	/// <returns>The singleton</returns>
	inline component_loader get_component_loader()
	{
		return component_loader{ take_over_abi_from_void_ptr{ glasssix_add_ref_get_component_loader_abi() } };
	}

	/// <summary>
	/// A convenient function to create an instance by specified (first) interface.
	/// </summary>
	/// <typeparam name="Interface">The interfacial type</typeparam>
	/// <returns>The instance</returns>
	template<typename Interface, typename = std::enable_if_t<impl::is_well_defined_interface_v<Interface>>>
	auto make_exported_interface()
	{
		return get_component_loader().create_by_interface_id(guid_of_v<Interface>).template as<Interface>();
	}

	/// <summary>
	/// A convenient function to create an instance by specified (first) interface.
	/// </summary>
	/// <typeparam name="Interface">The interfacial type</typeparam>
	/// <typeparam name="...Args">The argument types</typeparam>
	/// <param name="...args">The arguments</param>
	/// <returns>The instance</returns>
	template<typename Interface, typename... Args, typename = std::enable_if_t<impl::is_well_defined_interface_v<Interface>>, typename = std::void_t<decltype(std::declval<Interface>().init(std::declval<Args>()...))>>
	auto make_exported_interface(Args&&... args)
	{
		auto obj = get_component_loader().create_by_interface_id(guid_of_v<Interface>).template as<Interface>();

		return (obj.init(std::forward<Args>(args)...), obj);
	}
}
