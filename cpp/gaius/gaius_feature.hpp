#pragma once

#include <abi/consumer.hpp>

namespace glasssix::gaius
{
	struct gaius_feature;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<gaius::gaius_feature>
	{
		using identity_type = type_identity_interface;

		static constexpr guid id{ "BA483C79-C12A-482C-B11B-952DC2F54ACC" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t forward(abi_in_t<param_span<std::uint8_t>> input_data, std::int32_t num, std::int32_t order, bool mask, abi_out_t<param_vector<float>> result) noexcept = 0;
			virtual std::int32_t version(abi_out_t<param_string> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, gaius::gaius_feature> : interface_vtable_base<Derived, gaius::gaius_feature>
	{
		virtual std::int32_t forward(abi_in_t<param_span<std::uint8_t>> input_data, std::int32_t num, std::int32_t order, bool mask, abi_out_t<param_vector<float>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().forward(create_from_abi<param_span<std::uint8_t>>(input_data), num, order, mask)); });
		}

		virtual std::int32_t version(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
		}
	};

	template<> struct abi_adapter<gaius::gaius_feature>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, gaius::gaius_feature>
		{
			param_vector<float> forward(param_span<std::uint8_t> input_data, std::int32_t num, std::int32_t order, bool mask) const
			{
				param_vector<float> result{ nullptr };

				return (check_abi_result(this->self_abi().forward(get_abi(input_data), get_abi(num), get_abi(order), get_abi(mask), put_abi(result))), result);
			}

			param_string version() const
			{
				param_string result{ nullptr };

				return (check_abi_result(this->self_abi().version(put_abi(result))), result);
			}
		};
	};
}

namespace glasssix::gaius
{
	struct gaius_feature : exposing::inherits<gaius_feature>
	{
		using inherits::inherits;
	};
}
