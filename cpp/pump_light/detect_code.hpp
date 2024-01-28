#ifndef _LIGHT_DETECT_CODE_HPP_
#define _LIGHT_DETECT_CODE_HPP_

#include "box_info.hpp"
#include <abi/consumer.hpp>

namespace glasssix::pump_light
{
    struct detect_code;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<pump_light::detect_code>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "8C6BA15F-56A0-4101-85DD-A891F4175D18" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL init() noexcept = 0;

            virtual std::int32_t G6_ABI_CALL detect(
                abi_in_t<param_span<std::uint8_t>> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width,
                abi_in_t<exposing::param_hash_map<exposing::param_string, float>> param_map_abi,
                abi_out_t<pump_light::box_info> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, pump_light::detect_code> : interface_vtable_base<Derived, pump_light::detect_code>
    {

        virtual std::int32_t G6_ABI_CALL init() noexcept override
        {
            return abi_safe_call([&]
                { this->self().init(); });
        }

        virtual std::int32_t G6_ABI_CALL detect(abi_in_t<param_span<std::uint8_t>> bitmap,
            std::int32_t channels,
            std::int32_t height,
            std::int32_t width,
            abi_in_t<exposing::param_hash_map<exposing::param_string, float>> param_map_abi,
            abi_out_t<pump_light::box_info> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().detect(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width,
                    create_from_abi<exposing::param_hash_map<exposing::param_string, float>>(param_map_abi))); });
        }

        virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call(
                [&]
                {
                    *result = detach_abi(this->self().version());
                }
                );
        }
    };

    template <>
    struct abi_adapter<pump_light::detect_code>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, pump_light::detect_code>
        {
            void init() const
            {
                check_abi_result(this->self_abi().init());
            }

            pump_light::box_info detect(
                param_span<std::uint8_t> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width,
                const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const
            {
                pump_light::box_info result{};

                return (check_abi_result(
                    this->self_abi().detect(
                        get_abi(bitmap),
                        channels,
                        height,
                        width,
                        get_abi(param_map_abi),
                        put_abi(result))
                ),
                    result);
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
    struct detect_code : exposing::inherits<detect_code>
    {
        using inherits::inherits;
    };
}

#endif
