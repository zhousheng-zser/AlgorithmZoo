#ifndef _LIGHT_BOX_INFO_HPP_
#define _LIGHT_BOX_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::pump_light
{
    struct box_info;
}

namespace glasssix::exposing::impl
{
    template<>
    struct abi<pump_light::box_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "10E25447-CE94-4EA7-829F-C25D5B7D04B4" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL red_ratio(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL white_ratio(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL orange_ratio(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL light_status(abi_out_t<bool> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, pump_light::box_info> : interface_vtable_base<Derived, pump_light::box_info>
    {

        virtual std::int32_t G6_ABI_CALL red_ratio(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().red_ratio()); });
        }
        virtual std::int32_t G6_ABI_CALL white_ratio(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().white_ratio()); });
        }

        virtual std::int32_t G6_ABI_CALL orange_ratio(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().orange_ratio()); });
        }

        virtual std::int32_t G6_ABI_CALL light_status(abi_out_t<bool> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().light_status()); });
        }

        virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
        }
    };

    template <>
    struct abi_adapter<pump_light::box_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, pump_light::box_info>
        {
            float red_ratio() const
            {
                float result;

                return (check_abi_result(this->self_abi().red_ratio(put_abi(result))), result);
            }
            float white_ratio() const
            {
                float result;

                return (check_abi_result(this->self_abi().white_ratio(put_abi(result))), result);
            }
            float orange_ratio() const
            {
                float result;

                return (check_abi_result(this->self_abi().orange_ratio(put_abi(result))), result);
            }
            bool light_status() const
            {
                bool result;

                return (check_abi_result(this->self_abi().light_status(put_abi(result))), result);
            }
            param_string version() const
            {
                param_string result{ nullptr };

                return (check_abi_result(this->self_abi().version(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::pump_light
{
    struct box_info : exposing::inherits<box_info>
    {
        using inherits::inherits;
    };
}
#endif