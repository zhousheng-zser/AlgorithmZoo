#pragma once

#include <abi/consumer.hpp>

namespace glasssix::irisviel
{
	struct record;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<irisviel::record>
	{
		using identity_type = type_identity_interface;

		static constexpr guid id{ "D0869CF1-689F-4310-B7FB-0700F0AA5E68" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL init(std::int32_t dimension) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL dimension(abi_out_t<std::int32_t> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL key(abi_out_t<param_string> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL key(abi_in_t<param_string> value) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL feature(abi_out_t<param_vector<float>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL feature(abi_in_t<param_vector<float>> value) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL feature(abi_in_t<param_span<const float>> value) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, irisviel::record> : interface_vtable_base<Derived, irisviel::record>
	{
		virtual std::int32_t G6_ABI_CALL init(std::int32_t dimension) noexcept override
		{
			return abi_safe_call([&] { this->self().init(dimension); });
		}

		virtual std::int32_t G6_ABI_CALL dimension(abi_out_t<std::int32_t> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().dimension()); });
		}

		virtual std::int32_t G6_ABI_CALL key(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().key()); });
		}

		virtual std::int32_t G6_ABI_CALL key(abi_in_t<param_string> value) noexcept override
		{
			return abi_safe_call([&] { this->self().key(create_from_abi<param_string>(value)); });
		}

		virtual std::int32_t G6_ABI_CALL feature(abi_out_t<param_vector<float>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().feature()); });
		}

		virtual std::int32_t G6_ABI_CALL feature(abi_in_t<param_vector<float>> value) noexcept override
		{
			return abi_safe_call([&] { this->self().feature(create_from_abi<param_vector<float>>(value)); });
		}

		virtual std::int32_t G6_ABI_CALL feature(abi_in_t<param_span<const float>> value) noexcept override
		{
			return abi_safe_call([&] { this->self().feature(create_from_abi<param_span<const float>>(value)); });
		}
	};

	template<> struct abi_adapter<irisviel::record>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, irisviel::record>
		{
			void init(std::int32_t dimension) const
			{
				check_abi_result(this->self_abi().init(get_abi(dimension)));
			}

			std::int32_t dimension() const
			{
				std::int32_t result{};

				return (check_abi_result(this->self_abi().dimension(put_abi(result))), result);
			}

			param_string key() const
			{
				param_string result{ nullptr };

				return (check_abi_result(this->self_abi().key(put_abi(result))), result);
			}

			void key(const param_string& value) const
			{
				check_abi_result(this->self_abi().key(get_abi(value)));
			}

			param_vector<float> feature() const
			{
				param_vector<float> result{ nullptr };

				return (check_abi_result(this->self_abi().feature(put_abi(result))), result);
			}

			void feature(const param_vector<float>& value) const
			{
				check_abi_result(this->self_abi().feature(get_abi(value)));
			}

			void feature(param_span<const float> value) const
			{
				check_abi_result(this->self_abi().feature(get_abi(value)));
			}
		};
	};
}

namespace glasssix::irisviel
{
	struct record : exposing::inherits<record>
	{
		using inherits::inherits;
	};
}
