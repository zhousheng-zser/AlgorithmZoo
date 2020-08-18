#pragma once

#include "base.hpp"
#include "base_abi.hpp"
#include "implements.hpp"
#include "exceptions.hpp"
#include "param_string.hpp"
#include "param_vector.hpp"

#include <cstddef>
#include <cstdint>

namespace glasssix::exposing
{
	struct class_factory;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<class_factory>
	{
		using identity_type = type_identity_interface;
		static constexpr guid id{ "DCE95478-E317-43C2-B5E2-42DB0ECD4BD5" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL create_by_name(abi_in_t<param_string> qualified_name, abi_out_t<unknown_object> object) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL create_by_interface_id(abi_in_t<guid> interface_id, abi_out_t<unknown_object> object) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL interface_ids(abi_out_t<param_vector<guid>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL qualified_names(abi_out_t<param_vector<param_string>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL library_name(abi_out_t<param_string> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, class_factory> : interface_vtable_base<Derived, class_factory>
	{
		virtual std::int32_t G6_ABI_CALL create_by_name(abi_in_t<param_string> qualified_name, abi_out_t<unknown_object> object) noexcept override
		{
			return abi_safe_call([&] { *object = detach_abi(this->self().create_by_name(create_from_abi<param_string>(qualified_name))); });
		}

		virtual std::int32_t G6_ABI_CALL create_by_interface_id(abi_in_t<guid> interface_id, abi_out_t<unknown_object> object) noexcept override
		{
			return abi_safe_call([&] { *object = detach_abi(this->self().create_by_interface_id(create_from_abi<guid>(interface_id))); });
		}

		virtual std::int32_t G6_ABI_CALL interface_ids(abi_out_t<param_vector<guid>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().interface_ids()); });
		}

		virtual std::int32_t G6_ABI_CALL qualified_names(abi_out_t<param_vector<param_string>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().qualified_names()); });
		}

		virtual std::int32_t G6_ABI_CALL library_name(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().library_name()); });
		}
	};

	template<> struct abi_adapter<class_factory>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, class_factory>
		{
			/// <summary>
			/// Creates an instance by a qualified name.
			/// </summary>
			/// <param name="qualified_name">The qualified name</param>
			/// <returns>The instance</returns>
			unknown_object create_by_name(const param_string& qualified_name) const
			{
				unknown_object result{ nullptr };
				
				return (check_abi_result(this->self_abi().create_by_name(get_abi(qualified_name), put_abi(result))), result);
			}

			/// <summary>
			/// Creates an instance by first interface ID.
			/// </summary>
			/// <param name="interface_id">The interface ID</param>
			/// <returns>The instance</returns>
			unknown_object create_by_interface_id(const guid& interface_id) const
			{
				unknown_object result{ nullptr };

				return (check_abi_result(this->self_abi().create_by_interface_id(get_abi(interface_id), put_abi(result))), result);
			}

			/// <summary>
			/// Gets the available interface IDs.
			/// </summary>
			/// <returns>The qualified names</returns>
			param_vector<guid> interface_ids() const
			{
				param_vector<guid> result{ nullptr };

				return (check_abi_result(this->self_abi().interface_ids(put_abi(result))), result);
			}

			/// <summary>
			/// Gets the available qualified names.
			/// </summary>
			/// <returns>The qualified names</returns>
			param_vector<param_string> qualified_names() const
			{
				param_vector<param_string> result{ nullptr };

				return (check_abi_result(this->self_abi().qualified_names(put_abi(result))), result);
			}

			/// <summary>
			/// Gets the name of the library.
			/// </summary>
			/// <returns>The name of the library</returns>
			param_string library_name() const
			{
				param_string result{ nullptr };
				
				return (check_abi_result(this->self_abi().library_name(put_abi(result))), result);
			}
		};
	};
}

namespace glasssix::exposing
{
	struct class_factory : inherits<class_factory>
	{
		using inherits::inherits;
	};
}
