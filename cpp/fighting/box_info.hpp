#pragma once
#ifndef _FIGHTING_BOX_INFO_HPP_
#define _FIGHTING_BOX_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::fighting
{
	struct box_info;
}

namespace glasssix::exposing::impl
{
	template<>
	struct abi<fighting::box_info>
	{
		using identity_type = type_identity_interface;
		static constexpr guid id{ "{694D32AD-C0D2-4225-ACCB-424E78FBF63A}" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL x1(abi_out_t<std::int32_t> result) noexcept = 0;

			virtual std::int32_t G6_ABI_CALL x2(abi_out_t<std::int32_t> result) noexcept = 0;

			virtual std::int32_t G6_ABI_CALL y1(abi_out_t<std::int32_t> result) noexcept = 0;

			virtual std::int32_t G6_ABI_CALL y2(abi_out_t<std::int32_t> result) noexcept = 0;

			virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept = 0;

			virtual std::int32_t G6_ABI_CALL category(abi_out_t<std::int32_t> result) noexcept = 0;
		};
	};

	template <typename Derived>
	struct interface_vtable<Derived, fighting::box_info> : interface_vtable_base<Derived, fighting::box_info>
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

	};

	template <>
	struct abi_adapter<fighting::box_info>
	{
		template <typename Derived>
		struct type : enable_self_abi_awareness<Derived, fighting::box_info>
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
		};
	};
}

namespace glasssix::fighting
{
	struct box_info : exposing::inherits<box_info>
	{
		using inherits::inherits;
	};
}

#endif
