#ifndef _MATERIAL_CODE_HPP_
#define _MATERIAL_CODE_HPP_

#include "box_info.hpp"
#include <abi/consumer.hpp>

namespace glasssix::ring
{
    struct material_code;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<ring::material_code>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "{4E399FFA-49E0-41B4-B72E-A4EFC3204349}" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL init(
                abi_in_t<param_string> model_directory,
                abi_in_t<std::int32_t> factory_type, 
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
                abi_out_t<param_vector<ring::box_info>> result) noexcept = 0;

            //result
            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, ring::material_code> : interface_vtable_base<Derived, ring::material_code>
    {

        virtual std::int32_t G6_ABI_CALL init(
            abi_in_t<param_string> model_directory, 
            abi_in_t<std::int32_t> factory_type, 
            std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                { this->self().init(
                    create_from_abi<param_string>(model_directory), 
                    create_from_abi<std::int32_t>(factory_type), 
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
            abi_out_t<param_vector<ring::box_info>> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().detect(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order,
                                                                           x, y, roi_width, roi_height)); });
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
    struct abi_adapter<ring::material_code>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, ring::material_code>
        {
            void init(
                const param_string& model_directory, 
                std::int32_t factory_type, 
                std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(
                    get_abi(model_directory), 
                    get_abi(factory_type), 
                    get_abi(device)));
            }

            param_vector<ring::box_info> detect(
                param_span<std::uint8_t> bitmap, 
                std::int32_t channels, 
                std::int32_t height, 
                std::int32_t width, 
                std::int32_t order, 
                std::int32_t x, 
                std::int32_t y, 
                std::int32_t roi_width, 
                std::int32_t roi_height) const
            {
                param_vector<ring::box_info> result{ nullptr };

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

namespace glasssix::ring
{
    struct material_code : exposing::inherits<material_code>
    {
        using inherits::inherits;
    };
}

#endif