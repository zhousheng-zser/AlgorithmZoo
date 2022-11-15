#ifndef _OCR_CODE_HPP_
#define _OCR_CODE_HPP_

#include "box_info.hpp"
#include <abi/consumer.hpp>

namespace glasssix::plate
{
    struct ocr_code;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<plate::ocr_code>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "{4E399FFA-49E0-41B4-B72E-A4EFC3204349}" };

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
                std::int32_t order,
                std::int32_t x,
                std::int32_t y,
                std::int32_t roi_width,
                std::int32_t roi_height,
                abi_in_t<exposing::param_hash_map<exposing::param_string, float>> param_map_abi,
                abi_out_t<exposing::param_vector<plate::box_info>> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL recognize(
                abi_in_t<param_span<std::uint8_t>> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width,
                std::int32_t order,
                abi_out_t<exposing::param_vector<plate::box_info>> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL trace_init(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order, std::int32_t roi_x, std::int32_t roi_y, std::int32_t roi_w, std::int32_t roi_h) noexcept = 0;
            
            virtual std::int32_t G6_ABI_CALL trace_update(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order, abi_out_t<exposing::param_vector<plate::box_info>> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, plate::ocr_code> : interface_vtable_base<Derived, plate::ocr_code>
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
            std::int32_t order,
            std::int32_t x,
            std::int32_t y,
            std::int32_t roi_width,
            std::int32_t roi_height,
            abi_in_t<exposing::param_hash_map<exposing::param_string, float>> param_map_abi,
            abi_out_t<exposing::param_vector<plate::box_info>> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().detect(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order,
                    x, y, roi_width, roi_height, create_from_abi<exposing::param_hash_map<exposing::param_string, float>>(param_map_abi))); });
        }

        virtual std::int32_t G6_ABI_CALL recognize(
            abi_in_t<param_span<std::uint8_t>> bitmap,
            std::int32_t channels,
            std::int32_t height,
            std::int32_t width,
            std::int32_t order,
            abi_out_t<exposing::param_vector<plate::box_info>> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().recognize(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order)); });
        }

        virtual std::int32_t G6_ABI_CALL trace_init(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order, std::int32_t roi_x, std::int32_t roi_y, std::int32_t roi_w, std::int32_t roi_h) noexcept override
        {
            return abi_safe_call([&] { this->self().trace_init(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order, roi_x, roi_y, roi_w, roi_h); });
        }

        virtual std::int32_t G6_ABI_CALL trace_update(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order, abi_out_t< exposing::param_vector<plate::box_info>> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().trace_update(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order)); });
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
    struct abi_adapter<plate::ocr_code>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, plate::ocr_code>
        {
            void init(
                const param_string& model_directory,
                std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(
                    get_abi(model_directory),
                    get_abi(device)));
            }

            exposing::param_vector<plate::box_info> detect(
                param_span<std::uint8_t> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width,
                std::int32_t order,
                std::int32_t x,
                std::int32_t y,
                std::int32_t roi_width,
                std::int32_t roi_height,
                const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const
            {
                exposing::param_vector<plate::box_info> result{ nullptr };

                return (check_abi_result(
                    this->self_abi().detect(
                        get_abi(bitmap),
                        channels,
                        height,
                        width,
                        order,
                        x,
                        y,
                        roi_width,
                        roi_height,
                        get_abi(param_map_abi),
                        put_abi(result))
                ),
                    result);
            }

            exposing::param_vector<plate::box_info> recognize(
                param_span<std::uint8_t> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width,
                std::int32_t order) const
            {
                exposing::param_vector<plate::box_info> result{ nullptr };

                return (check_abi_result(
                    this->self_abi().recognize(
                        get_abi(bitmap),
                        channels,
                        height,
                        width,
                        order,
                        put_abi(result))
                ),
                    result);
            }

            void trace_init(param_span<std::uint8_t> bitmap, std::int32_t channels,std::int32_t height,std::int32_t width,std::int32_t order, const float roi_x, const float roi_y, const float roi_w, const float roi_h) const
            {
                check_abi_result(this->self_abi().trace_init(get_abi(bitmap), channels, height, width, order, roi_x, roi_y, roi_w, roi_h));
            }

            exposing::param_vector<plate::box_info> trace_update(param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
            {
                exposing::param_vector<plate::box_info> result{ nullptr };

                return (check_abi_result(this->self_abi().trace_update(get_abi(bitmap), channels, height, width, order, put_abi(result))), result);
            }

            param_string version() const
            {
                param_string result{ nullptr };

                return (check_abi_result(this->self_abi().version(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::plate
{
    struct ocr_code : exposing::inherits<ocr_code>
    {
        using inherits::inherits;
    };
}

#endif
