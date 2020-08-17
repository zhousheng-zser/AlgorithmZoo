#pragma once

#include <abi/consumer.hpp>

namespace glasssix::irisviel
{
	struct search_result;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<irisviel::search_result>
	{
		using identity_type = type_identity_interface;

		static constexpr guid id{ "D0869CF1-689F-4310-B7FB-0700F0AA5E68" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL similarity(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL key(abi_out_t<param_string> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL feature(abi_out_t<param_vector<float>> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, irisviel::search_result> : interface_vtable_base<Derived, irisviel::search_result>
	{
		virtual std::int32_t G6_ABI_CALL similarity(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().similarity()); });
		}

		virtual std::int32_t G6_ABI_CALL key(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().key()); });
		}

		virtual std::int32_t G6_ABI_CALL feature(abi_out_t<param_vector<float>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().feature()); });
		}
	};

	template<> struct abi_adapter<irisviel::search_result>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, irisviel::search_result>
		{
			float similarity() const
			{
				float result = 0.f;

				return (check_abi_result(this->self_abi().similarity(put_abi(result))), result);
			}

			param_string key() const
			{
				param_string result{ nullptr };

				return (check_abi_result(this->self_abi().key(put_abi(result))), result);
			}

			param_vector<float> feature() const
			{
				param_vector<float> result{ nullptr };

				return (check_abi_result(this->self_abi().feature(put_abi(result))), result);
			}
		};
	};
}

namespace glasssix::irisviel
{
	struct search_result : exposing::inherits<search_result>
	{
		using inherits::inherits;
	};
}
