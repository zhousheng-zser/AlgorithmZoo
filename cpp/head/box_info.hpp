#ifndef _HEAD_BOX_INFO_HPP_
#define _HEAD_BOX_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::head
{
    struct box_info;
}

namespace glasssix::exposing::impl
{
    template<>
    struct abi<head::box_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "004C36B8-A53E-4DE1-820C-516075C703A4" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL x1(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL y1(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL x2(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL y2(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL category(abi_out_t<int> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, head::box_info> : interface_vtable_base<Derived, head::box_info>
    {

        virtual std::int32_t G6_ABI_CALL x1(abi_out_t<int> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().x1()); });
        }

        virtual std::int32_t G6_ABI_CALL y1(abi_out_t<int> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().y1()); });
        }

        virtual std::int32_t G6_ABI_CALL x2(abi_out_t<int> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().x2()); });
        }

        virtual std::int32_t G6_ABI_CALL y2(abi_out_t<int> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().y2()); });
        }
        virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().score()); });
        }
        virtual std::int32_t G6_ABI_CALL category(abi_out_t<int> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().category()); });
        }


    };

    template <>
    struct abi_adapter<head::box_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, head::box_info>
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
            int x2() const
            {
                int result = 0;

                return (check_abi_result(this->self_abi().x2(put_abi(result))), result);
            }
            int y2() const
            {
                int result = 0;

                return (check_abi_result(this->self_abi().y2(put_abi(result))), result);
            }
			float score() const
            {
                float result = 0;

                return (check_abi_result(this->self_abi().score(put_abi(result))), result);
            }
			int category() const
            {
                int result = 0;

                return (check_abi_result(this->self_abi().category(put_abi(result))), result);
            }
           
        };
    };
}

namespace glasssix::head
{
    struct box_info : exposing::inherits<box_info>
    {
        using inherits::inherits;
    };
}
#endif