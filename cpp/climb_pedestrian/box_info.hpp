#ifndef _CLIMB_BOX_INFO_HPP_
#define _CLIMB_BOX_INFO_HPP_

#include <abi/consumer.hpp>

#include "../posture/detect_code.hpp"
#include "../pedestrian/box_info.hpp"

namespace glasssix::climb
{
    struct box_info;
}

namespace glasssix::exposing::impl
{
    template<>
    struct abi<climb::box_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "087FE6A4-C156-4A30-AAE2-398837DE19B5" };

          struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL x1(abi_out_t<int> result) noexcept = 0;//get x值
            // virtual std::int32_t G6_ABI_CALL set_x1(abi_in_t<int> result) noexcept = 0;  set x值
            virtual std::int32_t G6_ABI_CALL y1(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL x2(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL y2(abi_out_t<int> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL category(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL confidence(abi_out_t<float> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, climb::box_info> : interface_vtable_base<Derived, climb::box_info>
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
        virtual std::int32_t G6_ABI_CALL category(abi_out_t<int> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().category()); });
        }
        virtual std::int32_t G6_ABI_CALL confidence(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().confidence()); });
        }
    };

    template <>
    struct abi_adapter<climb::box_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, climb::box_info>
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
			int category() const
            {
                int result = 0;

                return (check_abi_result(this->self_abi().category(put_abi(result))), result);
            }
            float confidence() const
            {
                float result = 0.f;

                return (check_abi_result(this->self_abi().confidence(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::climb
{
    struct box_info : exposing::inherits<box_info>
    {
        using inherits::inherits;
    };
}
#endif