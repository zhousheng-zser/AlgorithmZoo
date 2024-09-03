#ifndef _PUMP_COVER_PLATE_BOX_INFO_HPP_
#define _PUMP_COVER_PLATE_BOX_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::pump_cover_plate
{
    struct box_info;
}

namespace glasssix::exposing::impl
{
    template<>
    struct abi<pump_cover_plate::box_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "2D314959-69EA-4157-9D29-58F998E256C1" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL cover_plate_status(abi_out_t<bool> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, pump_cover_plate::box_info> : interface_vtable_base<Derived, pump_cover_plate::box_info>
    {
        virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().score()); });
        }

        virtual std::int32_t G6_ABI_CALL cover_plate_status(abi_out_t<bool> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().cover_plate_status()); });
        }

        virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
        }
    };

    template <>
    struct abi_adapter<pump_cover_plate::box_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, pump_cover_plate::box_info>
        {
            float score() const
            {
                float result;

                return (check_abi_result(this->self_abi().score(put_abi(result))), result);
            }
            bool cover_plate_status() const
            {
                bool result;

                return (check_abi_result(this->self_abi().cover_plate_status(put_abi(result))), result);
            }
            param_string version() const
            {
                param_string result{ nullptr };

                return (check_abi_result(this->self_abi().version(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::pump_cover_plate
{
    struct box_info : exposing::inherits<box_info>
    {
        using inherits::inherits;
    };
}
#endif