#pragma once
#ifndef _VEHICLE_CLASSIFY_CODE_HPP_
#define _VEHICLE_CLASSIFY_CODE_HPP_

#include "box_info.hpp"
#include <abi/consumer.hpp>

namespace glasssix::vehicle
{
    struct classify_code;
}

namespace glasssix::exposing::impl
{
    template<>
    struct abi<vehicle::classify_code>
    {
        using identity_type = type_identity_interface;
        static constexpr guid id{ "{9DD2538C-CBBE-49D7-9BD6-0FD42317C40B}" };

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
                std::int32_t roi_x,
                std::int32_t roi_y,
                std::int32_t roi_width,
                std::int32_t roi_height,
                abi_in_t<exposing::param_hash_map<exposing::param_string,float>> param_map_abi,
                abi_out_t<exposing::param_vector<vehicle::box_info>> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL version(abi_out_t<exposing::param_string> result) noexcept = 0;

        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, vehicle::classify_code> : interface_vtable_base<Derived, vehicle::classify_code>
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
            std::int32_t roi_x,
            std::int32_t roi_y,
            std::int32_t roi_width,
            std::int32_t roi_height,
            abi_in_t<exposing::param_hash_map<exposing::param_string,float>> param_map_abi,
            abi_out_t<exposing::param_vector<vehicle::box_info>> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().detect(
                        create_from_abi<exposing::param_span<std::uint8_t>>(bitmap),
                        channels,
                        height,
                        width,
                        roi_x,
                        roi_y,
                        roi_width,
                        roi_height,
                        create_from_abi<exposing::param_hash_map<exposing::param_string,float>>(param_map_abi)));
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
    struct abi_adapter<vehicle::classify_code>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, vehicle::classify_code>
        {
            void init(
                const exposing::param_string& model_directory,
                std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(
                    get_abi(model_directory),
                    device));
            }

            exposing::param_vector<vehicle::box_info> detect(
                exposing::param_span<std::uint8_t> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width,
                std::int32_t roi_x,
                std::int32_t roi_y,
                std::int32_t roi_width,
                std::int32_t roi_height,
                const exposing::param_hash_map<exposing::param_string,float>& param_map_abi) const
            {
                exposing::param_vector<vehicle::box_info> result{ nullptr };
                return (check_abi_result(this->self_abi().detect(
                    get_abi(bitmap),
                    channels,
                    height,
                    width,
                    roi_x,
                    roi_y,
                    roi_width,
                    roi_height,
                    get_abi(param_map_abi),
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

namespace glasssix::vehicle
{
    struct classify_code : exposing::inherits<classify_code>
    {
        using inherits::inherits;
    };
}

#endif
