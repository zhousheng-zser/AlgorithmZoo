#pragma once
#ifndef _PUMP_PUMPTOP_PERSON_BOX_INFO_HPP_
#define _PUMP_PUMPTOP_PERSON_BOX_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::pump_pumptop_person
{
	struct box_info;
}

namespace glasssix::exposing::impl
{
	template<>
	struct abi<pump_pumptop_person::box_info>
	{
		using identity_type = type_identity_interface;
		static constexpr guid id{ "{DCF2A72A-DC5A-42DD-AB56-4A094B983D27}" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL x1(abi_out_t<std::int32_t> result) noexcept = 0;

			virtual std::int32_t G6_ABI_CALL x2(abi_out_t<std::int32_t> result) noexcept = 0;

			virtual std::int32_t G6_ABI_CALL y1(abi_out_t<std::int32_t> result) noexcept = 0;

			virtual std::int32_t G6_ABI_CALL y2(abi_out_t<std::int32_t> result) noexcept = 0;

			virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept = 0;

			virtual std::int32_t G6_ABI_CALL category(abi_out_t<std::int32_t> result) noexcept = 0;

			virtual std::int32_t G6_ABI_CALL pump(abi_out_t<exposing::param_vector<std::int32_t>> result) noexcept = 0;

		};
	};

	template <typename Derived>
	struct interface_vtable<Derived, pump_pumptop_person::box_info> : interface_vtable_base<Derived, pump_pumptop_person::box_info>
	{
		virtual std::int32_t G6_ABI_CALL x1(abi_out_t<std::int32_t> result) noexcept override
		{
			return abi_safe_call([&]
				{
					*result = detach_abi(this->self().x1());
				}
			);
		}

		virtual std::int32_t G6_ABI_CALL x2(abi_out_t<std::int32_t> result) noexcept override
		{
			return abi_safe_call([&]
				{
					*result = detach_abi(this->self().x2());
				}
			);
		}

		virtual std::int32_t G6_ABI_CALL y1(abi_out_t<std::int32_t> result) noexcept override
		{
			return abi_safe_call([&]
				{
					*result = detach_abi(this->self().y1());
				}
			);
		}

		virtual std::int32_t G6_ABI_CALL y2(abi_out_t<std::int32_t> result) noexcept override
		{
			return abi_safe_call([&]
				{
					*result = detach_abi(this->self().y2());
				}
			);
		}

		virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&]
				{
					*result = detach_abi(this->self().score());
				}
			);
		}

		virtual std::int32_t G6_ABI_CALL category(abi_out_t<std::int32_t> result) noexcept override
		{
			return abi_safe_call([&]
				{
					*result = detach_abi(this->self().category());
				}
			);
		}

		virtual std::int32_t G6_ABI_CALL pump(abi_out_t<exposing::param_vector<std::int32_t>> result) noexcept override
		{
			return abi_safe_call([&]
				{
					*result = detach_abi(this->self().pump());
				}
			);
		}

	};

	template <>
	struct abi_adapter<pump_pumptop_person::box_info>
	{
		template <typename Derived>
		struct type : enable_self_abi_awareness<Derived, pump_pumptop_person::box_info>
		{
			std::int32_t x1() const
			{
				std::int32_t result;
				return (check_abi_result(this->self_abi().x1(put_abi(result))), result);
			}

			std::int32_t x2() const
			{
				std::int32_t result;
				return (check_abi_result(this->self_abi().x2(put_abi(result))), result);
			}

			std::int32_t y1() const
			{
				std::int32_t result;
				return (check_abi_result(this->self_abi().y1(put_abi(result))), result);
			}

			std::int32_t y2() const
			{
				std::int32_t result;
				return (check_abi_result(this->self_abi().y2(put_abi(result))), result);
			}

			float score() const
			{
				float result;
				return (check_abi_result(this->self_abi().score(put_abi(result))), result);
			}

			std::int32_t category() const
			{
				std::int32_t result;
				return (check_abi_result(this->self_abi().category(put_abi(result))), result);
			}

			exposing::param_vector<std::int32_t> pump() const
			{
				exposing::param_vector<std::int32_t> result{ nullptr };
				return (check_abi_result(this->self_abi().pump(put_abi(result))), result);
			}

		};
	};
}

namespace glasssix::pump_pumptop_person
{
	struct box_info : exposing::inherits<box_info>
	{
		using inherits::inherits;
	};
}

#endif
