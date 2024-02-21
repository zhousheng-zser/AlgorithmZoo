#pragma once

#include <abi/consumer.hpp>

namespace glasssix::pump_gate_status
{
    struct gate_status;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<pump_gate_status::gate_status>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{"34AF10F9-C02F-427D-9037-E547BC1A7E97"};

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL init(std::int32_t device) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL init(std::int32_t model_type, abi_in_t<param_string> racy_path, std::int32_t device, bool use_int8) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> phai, abi_in_t<param_string> racy_path, std::int32_t device) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL detect(
                abi_in_t<param_span<std::uint8_t>> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width,
                std::int32_t yellow_hsv_lower,
                std::int32_t yellow_hsv_upper,
                std::int32_t gray_hsv_lower,
                std::int32_t gray_hsv_upper,
                exposing::param_vector<std::int32_t> rois,
                abi_in_t<exposing::param_hash_map<exposing::param_string, float>> param_map_abi,
                abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;

        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, pump_gate_status::gate_status> : interface_vtable_base<Derived, pump_gate_status::gate_status>
    {


        virtual std::int32_t G6_ABI_CALL detect(abi_in_t<param_span<std::uint8_t>> bitmap,
            std::int32_t channels,
            std::int32_t height,
            std::int32_t width,
            std::int32_t yellow_hsv_lower,
            std::int32_t yellow_hsv_upper,
            std::int32_t gray_hsv_lower,
            std::int32_t gray_hsv_upper,
            exposing::param_vector<int> rois,
            abi_in_t<exposing::param_hash_map<exposing::param_string, float>> param_map_abi,
            abi_out_t<std::int32_t> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().detect(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, yellow_hsv_lower, yellow_hsv_upper, gray_hsv_lower, gray_hsv_upper,
                   rois,
                   create_from_abi<exposing::param_hash_map<exposing::param_string, float>>(param_map_abi))); });
        }

        virtual std::int32_t G6_ABI_CALL init( std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                                 { this->self().init(device); });
        }
        
        virtual std::int32_t G6_ABI_CALL init(std::int32_t model_type, abi_in_t<param_string> racy_path, std::int32_t device, bool use_int8) noexcept override
        {
            return abi_safe_call([&]
                                 { this->self().init(model_type, create_from_abi<param_string>(racy_path), device, use_int8); });
        }

        virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> phai, abi_in_t<param_string> racy_path, std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                                 { this->self().init(create_from_abi<param_span<const param_string>>(phai), create_from_abi<param_string>(racy_path), device); });
        }


        virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().version()); });
        }
    };

    template <>
    struct abi_adapter<pump_gate_status::gate_status>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, pump_gate_status::gate_status>
        {
             std::int32_t detect(
                param_span<std::uint8_t> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width,
                std::int32_t yellow_hsv_lower,
                std::int32_t yellow_hsv_upper,
                std::int32_t gray_hsv_lower,
                std::int32_t gray_hsv_upper,
                exposing::param_vector<std::int32_t> rois,
                const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const
            {
                std::int32_t result;

                return (check_abi_result(
                    this->self_abi().detect(
                        get_abi(bitmap),
                        channels,
                        height,
                        width,
                        yellow_hsv_lower, 
                        yellow_hsv_upper, 
                        gray_hsv_lower, 
                        gray_hsv_upper,
                        rois,
                        get_abi(param_map_abi),
                        put_abi(result))
                ),
                result);
            }


            void init(std::int32_t device) const
            {
                check_abi_result(this->self_abi().init( get_abi(device)));
            }
           
            void init(std::int32_t model_type, const param_string &racy_path, std::int32_t device, bool use_int8) const
            {
                check_abi_result(this->self_abi().init(get_abi(model_type), get_abi(racy_path), get_abi(device), get_abi(use_int8)));
            }

            void init(param_span<const param_string> phai, const param_string &racy_path, std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(get_abi(phai), get_abi(racy_path), get_abi(device)));
            }

            param_string version() const
            {
                param_string result{nullptr};

                return (check_abi_result(this->self_abi().version(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::pump_gate_status
{
    struct gate_status : exposing::inherits<gate_status>
    {
        using inherits::inherits;
    };
}
