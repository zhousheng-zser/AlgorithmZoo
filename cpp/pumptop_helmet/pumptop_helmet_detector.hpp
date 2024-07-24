// abi类
#pragma once
#include <abi/consumer.hpp>
#include "pumptop_helmet_info.hpp"
#include <algo_plugin_interface.hpp>


namespace glasssix::pumptop_helmet
{
	struct pumptop_helmet_detector;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<pumptop_helmet::pumptop_helmet_detector>
	{
		using identity_type = type_identity_interface;

		static constexpr guid id{ "58C266D7-7882-4215-9625-D68858E60EEA" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> str_params) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL execute(abi_in_t<param_hash_map<param_string, unknown_object>> input_params_map, abi_out_t<param_string> result) = 0;
			virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, pumptop_helmet::pumptop_helmet_detector> : interface_vtable_base<Derived, pumptop_helmet::pumptop_helmet_detector>
	{
		virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> str_params) noexcept override
		{
			return abi_safe_call([&] { this->self().init(create_from_abi<param_string>(str_params)); });
		}
		virtual std::int32_t G6_ABI_CALL execute(abi_in_t<param_hash_map<param_string, unknown_object>> input_params_map, abi_out_t<param_string> result)noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().execute(create_from_abi<param_hash_map<param_string, unknown_object>>(input_params_map)));});
		}

		virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
		}
	};

	template<> struct abi_adapter<pumptop_helmet::pumptop_helmet_detector>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, pumptop_helmet::pumptop_helmet_detector>
		{
			void init(const exposing::param_string& str_params) const
			{
				check_abi_result(this->self_abi().init(get_abi(str_params)));
			}

			param_string execute(const param_hash_map<param_string, unknown_object>& input_params_map)
			{
				param_string result{ nullptr };
				return (check_abi_result(this->self_abi().execute(get_abi(input_params_map), put_abi(result))), result);
			}
			param_string version() const
			{
				param_string result{ nullptr };

				return (check_abi_result(this->self_abi().version(put_abi(result))), result);
			}
		};
	};
}

namespace glasssix::pumptop_helmet
{
	struct pumptop_helmet_detector : exposing::inherits<pumptop_helmet_detector,exposing::nessus::algo_plugin_interface>
	{
		using inherits::inherits;
	};
}
