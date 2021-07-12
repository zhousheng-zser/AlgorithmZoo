#ifndef _VP_INFO_HPP_
#define _VP_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::valklyrs
{
    struct vp_info;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<valklyrs::vp_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{"{25D94737-B44E-4838-94E4-A0B0B7C11EE0}"};

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL coordinates(abi_out_t<param_vector<float>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL attributes(abi_out_t<param_vector<param_string>> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, valklyrs::vp_info> : interface_vtable_base<Derived, valklyrs::vp_info>
    {
        virtual std::int32_t G6_ABI_CALL coordinates(abi_out_t<param_vector<float>> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().coordinates()); });
        }

        virtual std::int32_t G6_ABI_CALL attributes(abi_out_t<param_vector<param_string>> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().attributes()); });
        }
    };

    template <>
    struct abi_adapter<valklyrs::vp_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, valklyrs::vp_info>
        {
            param_vector<float> coordinates() const
            {
                param_vector<float> result;

                return (check_abi_result(this->self_abi().coordinates(put_abi(result))), result);
            }

            param_vector<param_string> attributes() const
            {
                param_vector<param_string> result;

                return (check_abi_result(this->self_abi().attributes(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::valklyrs
{
    struct vp_info : exposing::inherits<vp_info>
    {
        using inherits::inherits;
    };
}
#endif