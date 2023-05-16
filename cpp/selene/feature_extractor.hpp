#pragma once

#include <abi/consumer.hpp>

namespace glasssix::selene
{
	struct feature_extractor;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<selene::feature_extractor>
	{
		using identity_type = type_identity_interface;

		static constexpr guid id{ "33592171-B28D-475E-9F18-3AC449B01C0F" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> models_directory, std::int32_t model_type, std::int32_t device, bool use_int8) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL get(abi_in_t<param_span<std::uint8_t>> bitmaps, std::uint64_t count, std::int32_t order, abi_out_t<param_vector<param_vector<float>>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL get_model_type(abi_out_t<std::int32_t> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, selene::feature_extractor> : interface_vtable_base<Derived, selene::feature_extractor>
	{
		virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> models_directory, std::int32_t model_type, std::int32_t device, bool use_int8) noexcept override
		{
			return abi_safe_call([&] { this->self().init(create_from_abi<param_string>(models_directory), model_type, device, use_int8); });
		}

		virtual std::int32_t G6_ABI_CALL get(abi_in_t<param_span<std::uint8_t>> bitmaps, std::uint64_t count, std::int32_t order, abi_out_t<param_vector<param_vector<float>>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().get(create_from_abi<param_span<std::uint8_t>>(bitmaps), count, order)); });
		}

		virtual std::int32_t G6_ABI_CALL get_model_type(abi_out_t<std::int32_t> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().get_model_type()); });
		}

		virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
		}
	};

	template<> struct abi_adapter<selene::feature_extractor>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, selene::feature_extractor>
		{
			void init(const param_string& models_directory, std::int32_t model_type, std::int32_t device, bool use_int8) const
			{
				check_abi_result(this->self_abi().init(get_abi(models_directory), get_abi(model_type), get_abi(device), get_abi(use_int8)));
			}

			param_vector<param_vector<float>> get(param_span<std::uint8_t> bitmaps, std::uint64_t count, std::int32_t order) const
			{
				param_vector<param_vector<float>> result{ nullptr };

				return (check_abi_result(this->self_abi().get(get_abi(bitmaps), get_abi(count), get_abi(order), put_abi(result))), result);
			}

			std::int32_t get_model_type() const
			{
				std::int32_t result;

				return (check_abi_result(this->self_abi().get_model_type(put_abi(result))), result);
			}

			param_string version() const
			{
				param_string result{ nullptr };

				return (check_abi_result(this->self_abi().version(put_abi(result))), result);
			}
		};
	};
}

namespace glasssix::selene
{
	struct feature_extractor : exposing::inherits<feature_extractor>
	{
		using inherits::inherits;
	};
}
