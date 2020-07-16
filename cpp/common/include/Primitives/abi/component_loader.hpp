#pragma once

#include "singleton.hpp"
#include "dllexport.hpp"
#include "exceptions.hpp"
#include "filesystem.hpp"
#include "g6_attributes.hpp"
#include "class_factory.hpp"
#include "pure_c_handle_utils.h"
#include "platform_encoding.hpp"

#include <tuple>
#include <mutex>
#include <vector>
#include <memory>
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
		bool add_module(utf8_string_view path) noexcept
		{
			return add_module_with_factory(path);
		}

		/// <summary>
		/// Adds a module and returns the class factory.
		/// </summary>
		/// <param name="path">The path</param>
		/// <returns>The class factory</returns>
		class_factory add_module_with_factory(utf8_string_view path) noexcept
		{
			if (dll::dll_handle_ptr handle{ dll::load_library(path.data()), &dll::free_library })
			{
				if (auto dll_create_factory = reinterpret_cast<dll_routines::dll_create_factory_handler_type>(dll::get_symbol_address(handle.get(), dll_routines::dll_create_factory_handler_name.data())))
				{
					if (class_factory factory{ nullptr }; dll_create_factory(put_abi(factory)) == error_success)
					{
						auto names = factory.get_qualified_names();
						auto component_name = factory.get_component_name();
						{
							std::lock_guard<std::mutex> guard{ lock_ };

							modules_.emplace_back(handle);
							factories_.insert_or_assign(component_name, factory);
						}

						return factory;
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
		std::size_t add_modules(std::initializer_list<utf8_string_view> paths) noexcept
		{
			return std::count_if(paths.begin(), paths.end(), [&](utf8_string_view path) { return add_module(path); });
		}

		/// <summary>
		/// Adds a few modules and returns the available class factories.
		/// </summary>
		/// <param name="paths">The paths</param>
		/// <returns>The class factories of the successfully loaded modules</returns>
		param_vector<class_factory> add_modules_with_factories(std::initializer_list<utf8_string_view> paths) noexcept
		{
			auto result = make_param_vector<class_factory>();

			for (auto& item : paths)
			{
				if (auto factory = add_module_with_factory(item))
				{
					result.push_back(factory);
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
		std::size_t add_modules_in_directory(utf8_string_view directory, bool recursive = false) noexcept
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
		param_vector<class_factory> add_modules_with_factories_in_directory(utf8_string_view directory, bool recursive = false) noexcept
		{
			auto handler = [this](param_vector<class_factory>& result, const fs::path& item) { if (auto factory = add_module_with_factory(item.u8string())) { result.push_back(factory); } };

			if (recursive)
			{
				return for_each_dll_files<true>(directory, handler, make_param_vector<class_factory>());
			}
			else
			{
				return for_each_dll_files<false>(directory, handler, make_param_vector<class_factory>());
			}
		}

		/// <summary>
		/// Lookups a class factory by qualified name.
		/// </summary>
		/// <param name="qualified_name">The qualified name</param>
		/// <returns>The class factory</returns>
		class_factory lookup(utf8_string_view qualified_name) const noexcept
		{
			auto iter = factories_.find(qualified_name);

			return iter != factories_.end() ? iter->second : nullptr;
		}
	private:
		template<bool Recursive, typename Result, typename Callable>
		Result for_each_dll_files(utf8_string_view directory, Callable&& handler, Result&& initial_value) noexcept
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

		std::mutex lock_;
		std::vector<dll::dll_handle_ptr> modules_;
		std::unordered_map<param_string, class_factory> factories_;
	};
}
