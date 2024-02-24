// abi类
#pragma once
#include <abi/consumer.hpp>
#include "pumptop_helmet_info.hpp"


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
			virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> models_directory, std::int32_t device) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL detect(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			abi_in_t<exposing::param_hash_map<exposing::param_string, float>> param_map,abi_out_t<param_vector<pumptop_helmet::pumptop_helmet_info>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, pumptop_helmet::pumptop_helmet_detector> : interface_vtable_base<Derived, pumptop_helmet::pumptop_helmet_detector>
	{
		virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> models_directory, std::int32_t device) noexcept override
		{
			return abi_safe_call([&] { this->self().init(create_from_abi<param_string>(models_directory), device); });
		}

		virtual std::int32_t G6_ABI_CALL detect( abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			abi_in_t<exposing::param_hash_map<exposing::param_string, float>> param_map, abi_out_t<param_vector<pumptop_helmet::pumptop_helmet_info>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().detect(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width,
                        create_from_abi<exposing::param_hash_map<exposing::param_string, float>>(param_map))); });
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
			void init(const exposing::param_string& models_directory, std::int32_t device = -1) const
			{
				check_abi_result(this->self_abi().init(get_abi(models_directory), get_abi(device)));
			}

			param_vector<pumptop_helmet::pumptop_helmet_info> detect( param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, const exposing::param_hash_map<exposing::param_string, float>& param_map) const
			{
				param_vector<pumptop_helmet::pumptop_helmet_info> result{ nullptr };
				return (check_abi_result(this->self_abi().detect( get_abi(bitmap), channels, height, width, get_abi(param_map), put_abi(result))), result);
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
	struct pumptop_helmet_detector : exposing::inherits<pumptop_helmet_detector>
	{
		using inherits::inherits;
	};
}
