#ifndef _SMOKE_DETECT_CODE_HPP_
#define _SMOKE_DETECT_CODE_HPP_

#include "box_info.hpp"
#include <abi/consumer.hpp>

namespace glasssix::smoke
{
    struct detect_code;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<smoke::detect_code>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "A6A5AD56-B427-6AFA-F77E-FE508FAFD9EE" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL init(
                abi_in_t<param_string> model_directory,
                std::int32_t device) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL detect(
                abi_in_t<param_span<std::uint8_t>> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width,
                std::int32_t roi_x,
                std::int32_t roi_y,
                std::int32_t roi_width,
                std::int32_t roi_height,
                abi_in_t<exposing::param_hash_map<exposing::param_string, float>> param_map_abi,
                abi_out_t<exposing::param_vector<smoke::box_info>> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, smoke::detect_code> : interface_vtable_base<Derived, smoke::detect_code>
    {

        virtual std::int32_t G6_ABI_CALL init(
            abi_in_t<param_string> model_directory,
            std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                { this->self().init(
                    create_from_abi<param_string>(model_directory),
                    device); });
        }

        virtual std::int32_t G6_ABI_CALL detect(abi_in_t<param_span<std::uint8_t>> bitmap,
            std::int32_t channels,
            std::int32_t height,
            std::int32_t width,
            std::int32_t roi_x,
            std::int32_t roi_y,
            std::int32_t roi_width,
            std::int32_t roi_height,
            abi_in_t<exposing::param_hash_map<exposing::param_string, float>> param_map_abi,
            abi_out_t<exposing::param_vector<smoke::box_info>> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().detect(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, roi_x, roi_y, roi_width, roi_height,
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
    struct abi_adapter<smoke::detect_code>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, smoke::detect_code>
        {
            void init(
                const param_string& model_directory,
                std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(
                    get_abi(model_directory),
                    get_abi(device)));
            }

            exposing::param_vector<smoke::box_info> detect(
                param_span<std::uint8_t> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width,
                std::int32_t roi_x,
                std::int32_t roi_y,
                std::int32_t roi_width,
                std::int32_t roi_height,
                const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const
            {
                exposing::param_vector<smoke::box_info> result{ nullptr };

                return (check_abi_result(
                    this->self_abi().detect(
                        get_abi(bitmap),
                        channels,
                        height,
                        width,
                        roi_x,
                        roi_y,
                        roi_width,
                        roi_height,
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

namespace glasssix::smoke
{
    struct detect_code : exposing::inherits<detect_code>
    {
        using inherits::inherits;
    };
}

#endif
