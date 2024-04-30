#pragma once
#ifndef _FACELANDMARKS_LAND_CODE_HPP_
#define _FACELANDMARKS_LAND_CODE_HPP_

#include "land_info.hpp"
#include <abi/consumer.hpp>

namespace glasssix::facelandmarks
{
    struct land_code;
}

namespace glasssix::exposing::impl
{
    template<>
    struct abi<facelandmarks::land_code>
    {
        using identity_type = type_identity_interface;
        static constexpr guid id{ "{071E9F8C-A405-4FA2-9962-E2ACF767F578}" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL init(
                abi_in_t<exposing::param_string> model_directory,
                std::int32_t device) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL detect(
                abi_in_t<exposing::param_span<std::uint8_t>> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width,
                abi_out_t<facelandmarks::land_info> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL version(abi_out_t<exposing::param_string> result) noexcept = 0;

        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, facelandmarks::land_code> : interface_vtable_base<Derived, facelandmarks::land_code>
    {
        virtual std::int32_t G6_ABI_CALL init(
            abi_in_t<exposing::param_string> model_directory,
            std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                {this->self().init(
                    create_from_abi<exposing::param_string>(model_directory),
                    device);
                });
        }

        virtual std::int32_t G6_ABI_CALL detect(
            abi_in_t<exposing::param_span<std::uint8_t>> bitmap,
            std::int32_t channels,
            std::int32_t height,
            std::int32_t width,
            abi_out_t<facelandmarks::land_info> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().detect(
                        create_from_abi<exposing::param_span<std::uint8_t>>(bitmap),
                        channels,
                        height,
                        width));
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL version(abi_out_t<exposing::param_string> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().version());
                }
            );
        }

    };

    template <>
    struct abi_adapter<facelandmarks::land_code>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, facelandmarks::land_code>
        {
            void init(
                const exposing::param_string& model_directory,
                std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(
                    get_abi(model_directory),
                    device));
            }

            facelandmarks::land_info detect(
                exposing::param_span<std::uint8_t> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width) const
            {
                facelandmarks::land_info result;
                return (check_abi_result(this->self_abi().detect(
                    get_abi(bitmap),
                    channels,
                    height,
                    width,
                    put_abi(result))), result);
            }

            exposing::param_string version() const
            {
                exposing::param_string result{ nullptr };
                return (check_abi_result(this->self_abi().version(put_abi(result))), result);
            }

        };
    };
}

namespace glasssix::facelandmarks
{
    struct land_code : exposing::inherits<land_code>
    {
        using inherits::inherits;
    };
}

#endif
