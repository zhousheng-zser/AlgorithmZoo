#pragma once
#ifndef _PUMP_WELD_DETECT_CODE_HPP_
#define _PUMP_WELD_DETECT_CODE_HPP_

#include "box_info.hpp"
#include <abi/consumer.hpp>

namespace glasssix::pump_weld
{
    struct detect_code;
}

namespace glasssix::exposing::impl
{
    template<>
    struct abi<pump_weld::detect_code>
    {
        using identity_type = type_identity_interface;
        static constexpr guid id{ "{B6D24A1C-116A-49E6-8094-856C66CBB600}" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL init(
                abi_in_t<exposing::param_string> model_directory,
                std::int32_t device) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL detect(
                abi_in_t<exposing::param_span<std::uint8_t>> bitmap,
                std::int32_t batch,
                std::int32_t height,
                std::int32_t width,
                abi_in_t<exposing::param_hash_map<exposing::param_string,float>> param_map_abi,
                abi_out_t<exposing::param_vector<pump_weld::box_info>> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL version(abi_out_t<exposing::param_string> result) noexcept = 0;

        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, pump_weld::detect_code> : interface_vtable_base<Derived, pump_weld::detect_code>
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
            std::int32_t batch,
            std::int32_t height,
            std::int32_t width,
            abi_in_t<exposing::param_hash_map<exposing::param_string,float>> param_map_abi,
            abi_out_t<exposing::param_vector<pump_weld::box_info>> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().detect(
                        create_from_abi<exposing::param_span<std::uint8_t>>(bitmap),
                        batch,
                        height,
                        width,
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
    struct abi_adapter<pump_weld::detect_code>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, pump_weld::detect_code>
        {
            void init(
                const exposing::param_string& model_directory,
                std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(
                    get_abi(model_directory),
                    device));
            }

            exposing::param_vector<pump_weld::box_info> detect(
                exposing::param_span<std::uint8_t> bitmap,
                std::int32_t batch,
                std::int32_t height,
                std::int32_t width,
                const exposing::param_hash_map<exposing::param_string,float>& param_map_abi) const
            {
                exposing::param_vector<pump_weld::box_info> result{ nullptr };
                return (check_abi_result(this->self_abi().detect(
                    get_abi(bitmap),
                    batch,
                    height,
                    width,
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

namespace glasssix::pump_weld
{
    struct detect_code : exposing::inherits<detect_code>
    {
        using inherits::inherits;
    };
}

#endif
