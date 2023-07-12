#ifndef __LEAVEPOST_HPP__
#define __LEAVEPOSTT_HPP__

#include "box_info.hpp"
#include <abi/consumer.hpp>

namespace glasssix::leavepost
{
    struct yolo_net;
    struct Object;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<leavepost::yolo_net>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{"9F91EE36-0C72-4B6A-B394-985D2E8B62F5"};

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
                abi_out_t<param_vector<param_vector<float>>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, leavepost::yolo_net> : interface_vtable_base<Derived, leavepost::yolo_net>
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
            abi_out_t<param_vector<param_vector<float>>> result) noexcept override
        {
                return abi_safe_call([&]
                { *result = detach_abi(this->self().detect(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, roi_x, roi_y, roi_width, roi_height,
                   create_from_abi<exposing::param_hash_map<exposing::param_string, float>>(param_map_abi))); });
        }

        virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
        }
    };

    template <>
    struct abi_adapter<leavepost::yolo_net>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, leavepost::yolo_net>
        {
             void init(
                const param_string& model_directory,
                std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(
                    get_abi(model_directory),
                    get_abi(device)));
            }

            param_vector<leavepost::box_info> detect(
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
                param_vector<leavepost::box_info> result{nullptr};
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
                        put_abi(result))
                ),
                result);
            }

            param_string version() const
            {
                param_string result{nullptr};

                return (check_abi_result(this->self_abi().version(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::leavepost
{
    struct yolo_net : exposing::inherits<yolo_net>
    {
        using inherits::inherits;
    };
}
#endif
