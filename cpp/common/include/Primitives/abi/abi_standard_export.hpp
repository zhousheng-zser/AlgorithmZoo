#pragma once

#include "base.hpp"
#include "base_abi.hpp"
#include "implements.hpp"
#include "param_string.hpp"
#include "param_vector.hpp"
#include "class_factory.hpp"
#include "g6_attributes.hpp"
#include "fundamental_semantics.hpp"

#include <tuple>
#include <mutex>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <string_view>
#include <unordered_map>

#ifdef _WIN32
#define EXPORT_DIRECTIVE_FOR_MAKE_ABI_STANDARD_EXPORT_FUNCTIONS __declspec(dllexport)
#else
#define EXPORT_DIRECTIVE_FOR_MAKE_ABI_STANDARD_EXPORT_FUNCTIONS __attribute__((visibility("default")))
#endif

#define MAKE_ABI_STANDARD_EXPORT_FUNCTIONS(name, ...) \
	namespace { constexpr glasssix::exposing::utf8_string_view dll_module_library_name{ name }; } \
	extern "C" EXPORT_DIRECTIVE_FOR_MAKE_ABI_STANDARD_EXPORT_FUNCTIONS std::int32_t dll_create_factory(void** factory) noexcept { return glasssix::exposing::make_standard_export_functions<dll_module_library_name, __VA_ARGS__>::dll_create_factory_impl(factory); }; \
	extern "C" EXPORT_DIRECTIVE_FOR_MAKE_ABI_STANDARD_EXPORT_FUNCTIONS bool dll_can_unload_now() noexcept { return glasssix::exposing::get_module_ref_count() == 0; };

namespace glasssix::exposing
{
	namespace details
	{
		template<const utf8_string_view& LibraryName, typename Tuple, typename = void>
		struct make_standard_export_functions_impl;

		template<const utf8_string_view& LibraryName, typename... ComponentImpls>
		struct make_standard_export_functions_impl<LibraryName, std::tuple<ComponentImpls...>, std::enable_if_t<std::conjunction_v<std::is_default_constructible<ComponentImpls>..., impl::has_external_qualified_name<ComponentImpls>...>>>
		{
			template<typename Impl>
			static unknown_object make_component_impl()
			{
				return make_as_first<Impl>();
			}

			/// <summary>
			/// Implements a corresponding class factory.
			/// </summary>
			struct class_factory_impl : implements<class_factory_impl, class_factory>
			{
				inline static std::unordered_map<guid, std::function<unknown_object()>> guid_map;
				inline static std::unordered_map<param_string, std::function<unknown_object()>> name_map;

				/// <summary>
				/// Creates an instance.
				/// </summary>
				class_factory_impl()
				{
					static std::once_flag flag;

					std::call_once(flag, []
						{
							// Adds constructors for external qualified names.
							// Adds constructors for default implementations.
							(name_map.insert_or_assign(impl::get_external_qualified_name_v<ComponentImpls>, &make_component_impl<ComponentImpls>), ...);
							(guid_map.insert_or_assign(guid_of_v<impl::first_interface_t<ComponentImpls>>, &make_component_impl<ComponentImpls>), ...);
						});
				}

				/// <summary>
				/// Creates an instance by a qualified name.
				/// </summary>
				/// <param name="qualified_name">The qualified name</param>
				/// <returns>The instance</returns>
				unknown_object create_by_name(const param_string& qualified_name) const
				{
					auto iter = name_map.find(qualified_name);

					return iter != name_map.end() ? iter->second() : nullptr;
				}

				/// <summary>
				/// Creates an instance by first interface ID.
				/// </summary>
				/// <param name="interface_id">The interface ID</param>
				/// <returns>The instance</returns>
				unknown_object create_by_interface_id(const guid& interface_id) const
				{
					auto iter = guid_map.find(interface_id);

					return iter != guid_map.end() ? iter->second() : nullptr;
				}

				/// <summary>
				/// Gets the available interface IDs.
				/// </summary>
				/// <returns>The qualified names</returns>
				param_vector<guid> interface_ids() const
				{
					return make_param_vector<guid>(guid_of_v<impl::first_interface_t<ComponentImpls>>...);
				}

				/// <summary>
				/// Gets the available qualified names.
				/// </summary>
				/// <returns>The qualified names</returns>
				param_vector<param_string> qualified_names() const
				{
					return make_param_vector<param_string>(impl::get_external_qualified_name_v<ComponentImpls>...);
				}

				/// <summary>
				/// Gets the name of the library.
				/// </summary>
				/// <returns>The name of the library</returns>
				param_string library_name() const
				{
					return LibraryName;
				}
			};

			static std::int32_t G6_ABI_CALL dll_create_factory_impl(void** factory) noexcept
			{
				if (factory == nullptr)
				{
					return error_null_pointer;
				}

				return (*factory = detach_abi(make_as_first<class_factory_impl>()), error_success);
			}
		};
	}

	/// <summary>
	/// Makes DLL standard export functions for a couple of components.
	/// </summary>
	template<const utf8_string_view& LibraryName, typename... ComponentImpls>
	struct make_standard_export_functions : details::make_standard_export_functions_impl<LibraryName, std::tuple<ComponentImpls...>>
	{
	};
}
