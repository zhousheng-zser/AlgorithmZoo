#pragma once

#include <abi/consumer.hpp>

namespace glasssix::pumptop_helmet
{
	struct pumptop_helmet_info;
}

namespace glasssix::exposing::impl
{
	template<>
	struct abi<pumptop_helmet::pumptop_helmet_info>
	{
		using identity_type = type_identity_interface;

		static constexpr guid id{ "58C266D7-7882-4215-9625-D68858E60EEA" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL x1(abi_out_t<int> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL y1(abi_out_t<int> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL x2(abi_out_t<int> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL y2(abi_out_t<int> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL category(abi_out_t<int> result) noexcept = 0;
			virtual float G6_ABI_CALL score(abi_out_t<float> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, pumptop_helmet::pumptop_helmet_info> : interface_vtable_base<Derived, pumptop_helmet::pumptop_helmet_info>
	{
		virtual int G6_ABI_CALL x1(abi_out_t<int> result) noexcept override
		{
			return abi_safe_call([&]
			{*result = detach_abi(this->self().x1()); });
		}

		virtual int G6_ABI_CALL y1(abi_out_t<int> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().y1()); });
		}

		virtual int G6_ABI_CALL y2(abi_out_t<int> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().y2()); });
		}

		virtual int G6_ABI_CALL x2(abi_out_t<int> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().x2()); });
		}

		virtual int G6_ABI_CALL category(abi_out_t<int> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().category()); });
		}

		virtual float G6_ABI_CALL score(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().score()); });
		}

	};

	template<>
	struct abi_adapter<pumptop_helmet::pumptop_helmet_info>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, pumptop_helmet::pumptop_helmet_info>
		{
			int x1() const
			{
				int result = 0;

				return (check_abi_result(this->self_abi().x1(put_abi(result))), result);
			}

			int y1() const
			{
				int result = 0;

				return (check_abi_result(this->self_abi().y1(put_abi(result))), result);
			}

			int y2() const
			{
				int result = 0;

				return (check_abi_result(this->self_abi().y2(put_abi(result))), result);
			}

			int x2() const
			{
				int result = 0;

				return (check_abi_result(this->self_abi().x2(put_abi(result))), result);
			}

			int category() const
			{
				int result = 0;

				return (check_abi_result(this->self_abi().category(put_abi(result))), result);
			}

			float score() const
			{
				float result = 0;

				return (check_abi_result(this->self_abi().score(put_abi(result))), result);
			}

		};
	};
}

namespace glasssix::pumptop_helmet
{
	struct pumptop_helmet_info : exposing::inherits<pumptop_helmet_info>
	{
		using inherits::inherits;
	};
}
