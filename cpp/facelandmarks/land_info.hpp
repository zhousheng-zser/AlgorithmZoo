#pragma once
#ifndef _FACELANDMARKS_LAND_INFO_HPP_
#define _FACELANDMARKS_LAND_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::facelandmarks
{
    struct land_info;
}

namespace glasssix::exposing::impl
{
    template<>
    struct abi<facelandmarks::land_info>
    {
        using identity_type = type_identity_interface;
        static constexpr guid id{ "{C7FE4143-0548-42A2-8CFA-93F94AD759CE}" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL pts(abi_out_t<exposing::param_vector<exposing::param_pair<float,float>>> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept = 0;

        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, facelandmarks::land_info> : interface_vtable_base<Derived, facelandmarks::land_info>
    {
        virtual std::int32_t G6_ABI_CALL pts(abi_out_t<exposing::param_vector<exposing::param_pair<float,float>>> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().pts());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().score());
                }
            );
        }

    };

    template <>
    struct abi_adapter<facelandmarks::land_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, facelandmarks::land_info>
        {
            exposing::param_vector<exposing::param_pair<float,float>> pts() const
            {
                exposing::param_vector<exposing::param_pair<float,float>> result{ nullptr };
                return (check_abi_result(this->self_abi().pts(put_abi(result))), result);
            }

            float score() const
            {
                float result;
                return (check_abi_result(this->self_abi().score(put_abi(result))), result);
            }

        };
    };
}

namespace glasssix::facelandmarks
{
    struct land_info : exposing::inherits<land_info>
    {
        using inherits::inherits;
    };
}

#endif
