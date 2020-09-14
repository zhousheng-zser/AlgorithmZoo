#pragma once

#include <abi/consumer.hpp>

namespace glasssix::cassius
{
	struct feature_extractor;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<cassius::feature_extractor>
	{
		using identity_type = type_identity_interface;

		static constexpr guid id{ "34AF10F9-C02F-427D-9037-E547BC1A7E97" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> phai, abi_in_t<param_string> racy_path, std::int32_t device) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL get(abi_in_t<param_span<std::uint8_t>> bitmaps, std::uint64_t count, std::int32_t order, abi_out_t<param_vector<param_vector<float>>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, cassius::feature_extractor> : interface_vtable_base<Derived, cassius::feature_extractor>
	{
		virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> phai, abi_in_t<param_string> racy_path, std::int32_t device) noexcept override
		{
			return abi_safe_call([&] { this->self().init(create_from_abi<param_span<const param_string>>(phai), create_from_abi<param_string>(racy_path), device); });
		}

		virtual std::int32_t G6_ABI_CALL get(abi_in_t<param_span<std::uint8_t>> bitmaps, std::uint64_t count, std::int32_t order, abi_out_t<param_vector<param_vector<float>>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().get(create_from_abi<param_span<std::uint8_t>>(bitmaps), count, order)); });
		}

		virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
		}
	};

	template<> struct abi_adapter<cassius::feature_extractor>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, cassius::feature_extractor>
		{
			void init(param_span<const param_string> phai, const param_string& racy_path, std::int32_t device) const
			{
				check_abi_result(this->self_abi().init(get_abi(phai_path), get_abi(racy_path), get_abi(device)));
			}

			param_vector<param_vector<float>> get(param_span<std::uint8_t> bitmaps, std::uint64_t count, std::int32_t order) const
			{
				param_vector<param_vector<float>> result{ nullptr };

				return (check_abi_result(this->self_abi().get(get_abi(bitmaps), get_abi(count), get_abi(order), put_abi(result))), result);
			}

			param_string version() const
			{
				param_string result{ nullptr };

				return (check_abi_result(this->self_abi().version(put_abi(result))), result);
			}
		};
	};
}

namespace glasssix::cassius
{
	struct feature_extractor : exposing::inherits<feature_extractor>
	{
		using inherits::inherits;
	};
}
