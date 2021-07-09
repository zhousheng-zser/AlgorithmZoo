#ifndef _RESULT_INFO_HPP_
#define _RESULT_INFO_HPP_

#include <abi/consumer.hpp>
#include "vp_info.hpp"

namespace glasssix::valklyrs
{
    struct result_info;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<valklyrs::result_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{"{5C9F1437-FEA5-4F78-B0FB-C307DF8CC4CB}"};

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL vehicle_list(abi_out_t<param_vector<valklyrs::vp_info>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL person_list(abi_out_t<param_vector<valklyrs::vp_info>> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, valklyrs::result_info> : interface_vtable_base<Derived, valklyrs::result_info>
    {
        virtual std::int32_t G6_ABI_CALL vehicle_list(abi_out_t<param_vector<valklyrs::vp_info>> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().vehicle_list()); });
        }

        virtual std::int32_t G6_ABI_CALL person_list(abi_out_t<param_vector<valklyrs::vp_info>> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().person_list()); });
        }
    };

    template <>
    struct abi_adapter<valklyrs::result_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, valklyrs::result_info>
        {
            param_vector<valklyrs::vp_info> vehicle_list() const
            {
                param_vector<valklyrs::vp_info> result;

                return (check_abi_result(this->self_abi().vehicle_list(put_abi(result))), result);
            }

            param_vector<valklyrs::vp_info> person_list() const
            {
                param_vector<valklyrs::vp_info> result;

                return (check_abi_result(this->self_abi().person_list(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::valklyrs
{
    struct result_info : exposing::inherits<result_info>
    {
        using inherits::inherits;
    };
}
#endif