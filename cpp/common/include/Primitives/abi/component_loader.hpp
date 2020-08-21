#pragma once

#include "singleton.hpp"
#include "dllexport.hpp"
#include "exceptions.hpp"
#include "filesystem.hpp"
#include "param_vector.hpp"
#include "param_string.hpp"
#include "g6_attributes.hpp"
#include "class_factory.hpp"
#include "param_hash_map.hpp"
#include "pure_c_handle_utils.h"
#include "platform_encoding.hpp"

#include <list>
#include <mutex>
#include <tuple>
#include <vector>
#include <memory>
#include <utility>
#include <algorithm>
#include <type_traits>
#include <unordered_map>

namespace glasssix::exposing::dll
{
	DEFINE_PURE_C_HANDLE(dll);

	using symbol_func_ptr = void(*)();

	extern "C" EXPORT_EXCALIBUR_PRIMITIVES dll_handle G6_ABI_CALL load_library(const utf8_char * path) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES void G6_ABI_CALL free_library(dll_handle handle) noexcept;
	extern "C" EXPORT_EXCALIBUR_PRIMITIVES symbol_func_ptr G6_ABI_CALL get_symbol_address(dll_handle handle, const utf8_char * name) noexcept;

	using dll_handle_ptr = std::shared_ptr<std::remove_pointer_t<dll_handle>>;
}

namespace glasssix::exposing::dll_routines
{
	using dll_can_unload_now_handler_type = bool(G6_ABI_CALL*)() noexcept;
	using dll_create_factory_handler_type = std::int32_t(G6_ABI_CALL*)(void** factory) noexcept;

#ifdef _WIN32
	inline constexpr std::string_view dll_extension{ u8".dll" };
#elif defined(__linux__)
	inline constexpr std::string_view dll_extension{ u8".so" };
#else
#error "Unsupported Platform."
#endif

	inline constexpr utf8_string_view dll_can_unload_now_handler_name{ u8"dll_can_unload_now" };
	inline constexpr utf8_string_view dll_create_factory_handler_name{ u8"dll_create_factory" };
}

namespace glasssix::exposing
{
	/// <summary>
	/// A manager for in-process components (that is a component from within a DLL).
	/// </summary>
	class component_loader final : public singleton<component_loader>
	{
	public:
		friend singleton<component_loader>;

		/// <summary>
		/// Adds a module.
		/// </summary>
		/// <param name="path">The path</param>
		/// <returns>True if the opeartion succeeds; otherwise false</returns>
		bool add_module(utf8_string_view path)
		{
			return add_module_with_factory(path);
		}

		/// <summary>
		/// Adds a module and returns the class factory.
		/// </summary>
		/// <param name="path">The path</param>
		/// <returns>The class factory</returns>
		class_factory add_module_with_factory(utf8_string_view path)
		{
			std::error_code code;

			// Checks whether the DLL has been already loaded.
			if (auto factory = try_get_existing_factory(path))
			{
				return factory;
			}

			if (dll::dll_handle_ptr handle{ dll::load_library(path.data()), &dll::free_library })
			{
				if (auto dll_create_factory = reinterpret_cast<dll_routines::dll_create_factory_handler_type>(dll::get_symbol_address(handle.get(), dll_routines::dll_create_factory_handler_name.data())))
				{
					if (class_factory factory{ nullptr }; dll_create_factory(put_abi(factory)) == error_success)
					{
						return (parse_metadata(path, handle, factory), factory);
					}

					return nullptr;
				}

				return nullptr;
			}

			return nullptr;
		}

		/// <summary>
		/// Adds a few modules.
		/// </summary>
		/// <param name="paths">The paths</param>
		/// <returns>The count of successfully loaded modules</returns>
		std::size_t add_modules(std::initializer_list<utf8_string_view> paths)
		{
			return std::count_if(paths.begin(), paths.end(), [&](utf8_string_view path) { return add_module(path); });
		}

		/// <summary>
		/// Adds a few modules and returns the available class factories.
		/// </summary>
		/// <param name="paths">The paths</param>
		/// <returns>The class factories of the successfully loaded modules</returns>
		param_hash_map<param_string, class_factory> add_modules_with_factories(std::initializer_list<utf8_string_view> paths)
		{
			auto result = make_param_hash_map<param_string, class_factory>();

			for (auto& item : paths)
			{
				if (auto factory = add_module_with_factory(item))
				{
					result.add_or_update(factory.library_name(), factory);
				}
			}

			return result;
		}

		/// <summary>
		/// Finds modules in a directory and adds them.
		/// </summary>
		/// <param name="directory">The directory</param>
		/// <param name="recursive">Indicates whether to find modules recursively</param>
		/// <returns>The count of successfully loaded modules</returns>
		std::size_t add_modules_in_directory(utf8_string_view directory, bool recursive = false)
		{
			auto handler = [this](std::size_t& result, const fs::path& item) { if (add_module(item.u8string())) { result++; } };

			if (recursive)
			{
				return for_each_dll_files<true>(directory, handler, std::size_t{});
			}
			else
			{
				return for_each_dll_files<false>(directory, handler, std::size_t{});
			}
		}

		/// <summary>
		/// Finds modules in a directory, adds them and returns the available class factories.
		/// </summary>
		/// <param name="directory">The directory</param>
		/// <param name="recursive">Indicates whether to find modules recursively</param>
		/// <returns>The class factories of the successfully loaded modules</returns>
		param_hash_map<param_string, class_factory> add_modules_with_factories_in_directory(utf8_string_view directory, bool recursive = false)
		{
			auto handler = [this](param_hash_map<param_string, class_factory>& result, const fs::path& item) { if (auto factory = add_module_with_factory(item.u8string())) { result.add_or_update(factory.library_name(), factory); } };

			if (recursive)
			{
				return for_each_dll_files<true>(directory, handler, make_param_hash_map<param_string, class_factory>());
			}
			else
			{
				return for_each_dll_files<false>(directory, handler, make_param_hash_map<param_string, class_factory>());
			}
		}

		/// <summary>
		/// Lookups a class factory by library name.
		/// </summary>
		/// <param name="library_name">The library name</param>
		/// <returns>The class factory</returns>
		class_factory lookup_factory(utf8_string_view library_name)
		{
			std::scoped_lock lock{ lock_ };
			auto iter = name_factory_map_.find(library_name);

			return iter != name_factory_map_.end() ? iter->second : nullptr;
		}

		/// <summary>
		/// Retrieves the loaded library names.
		/// </summary>
		/// <returns>The loaded library names</returns>
		param_vector<param_string> library_names()
		{
			auto result = make_param_vector<param_string>();
			{
				std::scoped_lock lock{ lock_ };

				for (const auto& [key, value] : name_factory_map_)
				{
					result.push_back(key);
				}

				return result;
			}
		}

		/// <summary>
		/// Retrieves the loaded factories.
		/// </summary>
		/// <returns>The factories</returns>
		param_hash_map<param_string, class_factory> factories()
		{
			auto result = make_param_hash_map<param_string, class_factory>();
			{
				std::scoped_lock lock{ lock_ };

				for (const auto& [key, value] : name_factory_map_)
				{
					result.add_or_update(key, value);
				}

				return result;
			}
		}

		/// <summary>
		/// Checks whether an external qualified name exists.
		/// </summary>
		/// <param name="qualified_name">The qualified name</param>
		/// <returns>True if it exists; otherwise false</returns>
		bool contains_qualified_name(utf8_string_view qualified_name)
		{
			std::scoped_lock lock{ lock_ };

			return qualified_name_activator_map_.find(qualified_name) != qualified_name_activator_map_.end();
		}

		/// <summary>
		/// Checks whether an interface ID exists.
		/// </summary>
		/// <param name="qualified_name">The interface ID</param>
		/// <returns>True if it exists; otherwise false</returns>
		bool contains_interface_id(const guid& interface_id)
		{
			std::scoped_lock lock{ lock_ };

			return interface_id_activator_map_.find(interface_id) != interface_id_activator_map_.end();
		}

		/// <summary>
		/// Creates an instance by external qualified name.
		/// </summary>
		/// <param name="qualified_name">The qualified name</param>
		/// <returns>The object</returns>
		unknown_object create_by_name(utf8_string_view qualified_name)
		{
			auto handler = [&]
			{
				std::scoped_lock lock{ lock_ };
				auto iter = qualified_name_activator_map_.find(qualified_name);

				return iter != qualified_name_activator_map_.end() ? iter->second : std::function<unknown_object()>{};
			}();

			return handler ? handler() : throw abi_key_not_found{ qualified_name };
		}

		/// <summary>
		/// Creates an instance by first interface ID.
		/// </summary>
		/// <param name="qualified_name">The first interface ID</param>
		/// <returns>The object</returns>
		unknown_object create_by_interface_id(const guid& interface_id)
		{
			auto handler = [&]
			{
				std::scoped_lock lock{ lock_ };
				auto iter = interface_id_activator_map_.find(interface_id);

				return iter != interface_id_activator_map_.end() ? iter->second : std::function<unknown_object()>{};
			}();

			return handler ? handler() : throw abi_key_not_found{ to_param_string(interface_id) };
		}
	private:
		template<bool Recursive, typename Result, typename Callable>
		Result for_each_dll_files(utf8_string_view directory, Callable&& handler, Result&& initial_value)
		{
			using iterator_type = std::conditional_t<Recursive, fs::recursive_directory_iterator, fs::directory_iterator>;

			std::error_code code;
			Result result{ std::forward<Result>(initial_value) };

			for (auto& item : iterator_type{ to_narrow_string(directory), fs::directory_options::skip_permission_denied, code })
			{
				if (item.path().has_extension() && item.path().extension() == dll_routines::dll_extension)
				{
					std::forward<Callable>(handler)(result, item.path());
				}
			}

			return result;
		}

		class_factory try_get_existing_factory(utf8_string_view path)
		{
			std::error_code code;
			std::scoped_lock lock{ lock_ };

			if (auto iter_pair = std::find_if(modules_.begin(), modules_.end(), [&](const std::pair<fs::path, dll::dll_handle_ptr>& inner) { return fs::equivalent(inner.first, to_narrow_string(path), code); }); iter_pair != modules_.end())
			{
				if (auto iter = handle_factory_map_.find(iter_pair->second); iter != handle_factory_map_.end())
				{
					return iter->second;
				}

				modules_.erase(iter_pair);
			}

			return nullptr;
		}

		void parse_metadata(utf8_string_view path, const dll::dll_handle_ptr& handle, const class_factory& factory)
		{
			auto names = factory.qualified_names();
			auto library_name = factory.library_name();
			auto interface_ids = factory.interface_ids();
			{
				std::scoped_lock lock{ lock_ };

				modules_.emplace_back(to_narrow_string(path), handle);
				name_factory_map_.insert_or_assign(library_name, factory);
				std::for_each(exposing::begin(names), exposing::end(names), [&](const param_string& inner) { qualified_name_activator_map_.insert_or_assign(inner, [=] { return factory.create_by_name(inner); }); });
				std::for_each(exposing::begin(interface_ids), exposing::end(interface_ids), [&](const guid& inner) { interface_id_activator_map_.insert_or_assign(inner, [=] { return factory.create_by_interface_id(inner); }); });
			}
		}

		std::mutex lock_;
		std::list<std::pair<fs::path, dll::dll_handle_ptr>> modules_;
		std::unordered_map<param_string, class_factory> name_factory_map_;
		std::unordered_map<dll::dll_handle_ptr, class_factory> handle_factory_map_;
		std::unordered_map<guid, std::function<unknown_object()>> interface_id_activator_map_;
		std::unordered_map<param_string, std::function<unknown_object()>> qualified_name_activator_map_;
	};
	
	/// <summary>
	/// A convenient function to create an instance by specified (first) interface.
	/// </summary>
	/// <typeparam name="Interface">The interfacial type</typeparam>
	/// <returns>The instance</returns>
	template<typename Interface, typename = std::enable_if_t<impl::is_well_defined_interface_v<Interface>>>
	auto make_exported_interface()
	{
		return component_loader::instance().create_by_interface_id(guid_of_v<Interface>).as<Interface>();
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
		auto obj = component_loader::instance().create_by_interface_id(guid_of_v<Interface>).as<Interface>();

		return (obj.init(std::forward<Args>(args)...), obj);
	}
}
