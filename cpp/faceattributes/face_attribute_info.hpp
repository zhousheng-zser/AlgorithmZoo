#pragma once

#include <abi/consumer.hpp>

namespace glasssix::face_attributes
{
	struct face_attribute_info;
}

namespace glasssix::exposing::impl
{
	template<> 
	struct abi<face_attributes::face_attribute_info>
	{
		using identity_type = type_identity_interface;

		static constexpr guid id{ "D5A7EE6A-AF75-4D28-8C5C-C26661DC63EB" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL age(abi_out_t<int> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL gender(abi_out_t<int> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL mask(abi_out_t<int> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL glass(abi_out_t<int> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, face_attributes::face_attribute_info> : interface_vtable_base<Derived, face_attributes::face_attribute_info>
	{
		virtual int G6_ABI_CALL age(abi_out_t<int> result) noexcept override
		{
			return abi_safe_call([&] 
			{*result = detach_abi(this->self().age()); });
		}
		
		virtual int G6_ABI_CALL gender(abi_out_t<int> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().gender()); });
		}

		virtual int G6_ABI_CALL glass(abi_out_t<int> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().glass()); });
		}
        
		virtual int G6_ABI_CALL mask(abi_out_t<int> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().mask()); });
		}

	};

	template<> 
	struct abi_adapter<face_attributes::face_attribute_info>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, face_attributes::face_attribute_info>
		{
			int age() const
			{
				int result = 0;

				return (check_abi_result(this->self_abi().age(put_abi(result))), result);
			}
			
			int gender() const
			{
				int result = 0;

				return (check_abi_result(this->self_abi().gender(put_abi(result))), result);
			}

			int glass() const
			{
				int result = 0;

				return (check_abi_result(this->self_abi().glass(put_abi(result))), result);
			}

			int mask() const
			{
				int result = 0;

				return (check_abi_result(this->self_abi().mask(put_abi(result))), result);
			}
		
		};
	};
}

namespace glasssix::face_attributes
{
	struct face_attribute_info : exposing::inherits<face_attribute_info>
	{
		using inherits::inherits;
	};
}
