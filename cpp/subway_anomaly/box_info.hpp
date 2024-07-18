#ifndef _SUBWAY_BOX_INFO_HPP_
#define _SUBWAY_BOX_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::subway_anomaly
{
    struct box_info;
}

namespace glasssix::exposing::impl
{
    template<>
    struct abi<subway_anomaly::box_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "6DC4959E-2C8C-4BB1-85DD-3C3B72CF3754" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL anomaly_status(abi_out_t<bool> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, subway_anomaly::box_info> : interface_vtable_base<Derived, subway_anomaly::box_info>
    {
        virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().score()); });
        }

        virtual std::int32_t G6_ABI_CALL anomaly_status(abi_out_t<bool> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().anomaly_status()); });
        }

        virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
        }
    };

    template <>
    struct abi_adapter<subway_anomaly::box_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, subway_anomaly::box_info>
        {
            float score() const
            {
                float result;

                return (check_abi_result(this->self_abi().score(put_abi(result))), result);
            }
            bool anomaly_status() const
            {
                bool result;

                return (check_abi_result(this->self_abi().anomaly_status(put_abi(result))), result);
            }
            param_string version() const
            {
                param_string result{ nullptr };

                return (check_abi_result(this->self_abi().version(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::subway_anomaly
{
    struct box_info : exposing::inherits<box_info>
    {
        using inherits::inherits;
    };
}
#endif