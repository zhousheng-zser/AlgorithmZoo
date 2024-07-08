#pragma once
#include <abi/consumer.hpp>
#include <algo_plugin_interface.hpp>
#include "../longinus/face_info.hpp"
#include "face_attribute_info.hpp"

namespace glasssix::face_attributes
{
	struct face_attributes_detector;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<face_attributes::face_attributes_detector>
	{
		using identity_type = type_identity_interface;

		static constexpr guid id{ "2C034584-0D05-4F40-A396-9496B8CB9551" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> str_params) = 0;
			virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL execute(abi_in_t<param_hash_map<param_string, unknown_object>> input_params_map, abi_out_t<param_string> result) = 0;

			virtual std::int32_t G6_ABI_CALL detect(abi_in_t<param_vector<longinus::face_info>> faces, abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
				std::int32_t order, abi_out_t<param_vector<face_attributes::face_attribute_info>> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, face_attributes::face_attributes_detector> : interface_vtable_base<Derived, face_attributes::face_attributes_detector>
	{
		virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> str_params) override
		{
			return abi_safe_call([&] { this->self().init(create_from_abi<param_string>(str_params)); });
		}

		virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
		}

		virtual std::int32_t G6_ABI_CALL execute(abi_in_t<param_hash_map<param_string, unknown_object>> input_params_map, abi_out_t<param_string> result) override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().execute(create_from_abi<param_hash_map<param_string, unknown_object>>(input_params_map))); });
		}

		virtual std::int32_t G6_ABI_CALL detect(abi_in_t<param_vector<longinus::face_info>> faces, abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			std::int32_t order, abi_out_t<param_vector<face_attributes::face_attribute_info>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().detect(create_from_abi<param_vector<longinus::face_info>>(faces), create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order)); });
		}
	};

	template<> struct abi_adapter<face_attributes::face_attributes_detector>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, face_attributes::face_attributes_detector>
		{
			void init(const param_string& str_params)
			{
				check_abi_result(this->self_abi().init(get_abi(str_params)));
			}

			param_string version() const
			{
				param_string result{ nullptr };

				return (check_abi_result(this->self_abi().version(put_abi(result))), result);
			}
			param_string execute(const param_hash_map<param_string, unknown_object>& input_params_map)
			{
				param_string result{ nullptr };

				return (check_abi_result(this->self_abi().execute(get_abi(input_params_map), put_abi(result))), result);
			}

			param_vector<face_attributes::face_attribute_info> detect(const param_vector<longinus::face_info>& faces, param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
			{
				param_vector<face_attributes::face_attribute_info> result{ nullptr };
				return (check_abi_result(this->self_abi().detect(get_abi(faces), get_abi(bitmap), channels, height, width, order, put_abi(result))), result);
			}
		};
	};
}

namespace glasssix::face_attributes
{
	struct face_attributes_detector : exposing::inherits<face_attributes_detector, exposing::nessus::algo_plugin_interface>
	{
		using inherits::inherits;
	};
}
