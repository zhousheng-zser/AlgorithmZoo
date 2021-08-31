#ifndef _MATERIAL_CODE_BOX_INFO_HPP_
#define _MATERIAL_CODE_BOX_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::heimdall
{
    struct box_info;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<heimdall::box_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{"{B379E158-C696-45BE-9D6F-797361FD1EC0}"};

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL location(abi_out_t<param_vector<float>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL strinfo(abi_out_t<param_string> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL angle(abi_out_t<float> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, heimdall::box_info> : interface_vtable_base<Derived, heimdall::box_info>
    {
        virtual std::int32_t G6_ABI_CALL location(abi_out_t<param_vector<float>> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().location()); });
        }

        virtual std::int32_t G6_ABI_CALL strinfo(abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().strinfo()); });
        }

        virtual std::int32_t G6_ABI_CALL angle(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().angle()); });
        }
    };

    template <>
    struct abi_adapter<heimdall::box_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, heimdall::box_info>
        {
            param_vector<float> location() const
            {
                param_vector<float> result;

                return (check_abi_result(this->self_abi().location(put_abi(result))), result);
            }

            param_string strinfo() const
            {
                param_string result;

                return (check_abi_result(this->self_abi().strinfo(put_abi(result))), result);
            }

            float angle() const
            {
                float result;

                return (check_abi_result(this->self_abi().angle(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::heimdall
{
    struct box_info : exposing::inherits<box_info>
    {
        using inherits::inherits;
    };
}
#endif