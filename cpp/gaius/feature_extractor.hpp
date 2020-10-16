#pragma once

#include <abi/consumer.hpp>

namespace glasssix::gaius
{
	struct feature_extractor;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<gaius::feature_extractor>
	{
		using identity_type = type_identity_interface;

		static constexpr guid id{ "BA483C79-C12A-482C-B11B-952DC2F54ACC" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> phai_path, abi_in_t<param_string> racy_path, abi_in_t<param_string> mask_phai_path, abi_in_t<param_string> mask_racy_path, std::int32_t device) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL get(abi_in_t<param_span<std::uint8_t>> bitmaps, std::uint64_t count, std::int32_t order, bool has_mask, abi_out_t<param_vector<param_vector<float>>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> racy_path, abi_in_t<param_string> mask_racy_path, std::int32_t device) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, gaius::feature_extractor> : interface_vtable_base<Derived, gaius::feature_extractor>
	{
		virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> racy_path, abi_in_t<param_string> mask_racy_path, std::int32_t device) noexcept override
		{
			return abi_safe_call([&] { this->self().init(create_from_abi<param_string>(racy_path), create_from_abi<param_string>(mask_racy_path), device); });
		}
		virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> phai_path, abi_in_t<param_string> racy_path, abi_in_t<param_string> mask_phai_path, abi_in_t<param_string> mask_racy_path, std::int32_t device) noexcept override
		{
			return abi_safe_call([&] { this->self().init(create_from_abi<param_string>(phai_path), create_from_abi<param_string>(racy_path), create_from_abi<param_string>(mask_phai_path), create_from_abi<param_string>(mask_racy_path), device); });
		}

		virtual std::int32_t G6_ABI_CALL get(abi_in_t<param_span<std::uint8_t>> bitmaps, std::uint64_t count, std::int32_t order, bool has_mask, abi_out_t<param_vector<param_vector<float>>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().get(create_from_abi<param_span<std::uint8_t>>(bitmaps), count, order, has_mask)); });
		}

		virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
		}
	};

	template<> struct abi_adapter<gaius::feature_extractor>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, gaius::feature_extractor>
		{
			void init(const param_string& racy_path, const param_string& mask_racy_path, std::int32_t device) const
			{
				check_abi_result(this->self_abi().init(get_abi(racy_path), get_abi(mask_racy_path), get_abi(device)));
			}

			void init(const param_string& phai_path, const param_string& racy_path, const param_string& mask_phai_path, const param_string& mask_racy_path, std::int32_t device) const
			{
				check_abi_result(this->self_abi().init(get_abi(phai_path), get_abi(racy_path), get_abi(mask_phai_path), get_abi(mask_racy_path), get_abi(device)));
			}

			param_vector<param_vector<float>> get(param_span<std::uint8_t> bitmaps, std::uint64_t count, std::int32_t order, bool has_mask) const
			{
				param_vector<param_vector<float>> result{ nullptr };

				return (check_abi_result(this->self_abi().get(get_abi(bitmaps), get_abi(count), get_abi(order), has_mask, put_abi(result))), result);
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
	struct feature_extractor : exposing::inherits<feature_extractor>
	{
		using inherits::inherits;
	};
}
