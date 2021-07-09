#ifndef _YOLOV5S_NET_HPP_
#define _YOLOV5S_NET_HPP_

#include "result_info.hpp"
#include "vp_info.hpp"
#include <abi/consumer.hpp>

namespace glasssix::valklyrs
{
    struct yolov5s_net;
    struct Object;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<valklyrs::yolov5s_net>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{"{53024AAD-AA5E-49DB-BE24-5E98ED4768F8}"};

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> yolov5s_racy_path, abi_in_t<param_string> vehicle_racy_path, abi_in_t<param_string> person_racy_path, std::int32_t device) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> yolov5s_phai, abi_in_t<param_string> yolov5s_racy_path, abi_in_t<param_span<const param_string>> vehicle_phai, abi_in_t<param_string> vehicle_racy_path, abi_in_t<param_span<const param_string>> person_phai, abi_in_t<param_string> person_racy_path, std::int32_t device) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL detect(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
                                                    std::int32_t order, abi_out_t<param_vector<valklyrs::result_info>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, valklyrs::yolov5s_net> : interface_vtable_base<Derived, valklyrs::yolov5s_net>
    {
        virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> yolov5s_racy_path, abi_in_t<param_string> vehicle_racy_path, abi_in_t<param_string> person_racy_path, std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                                 { this->self().init(create_from_abi<param_string>(yolov5s_racy_path), create_from_abi<param_string>(vehicle_racy_path), create_from_abi<param_string>(person_racy_path), device); });
        }

        virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> yolov5s_phai, abi_in_t<param_string> yolov5s_racy_path, abi_in_t<param_span<const param_string>> vehicle_phai, abi_in_t<param_string> vehicle_racy_path, abi_in_t<param_span<const param_string>> person_phai, abi_in_t<param_string> person_racy_path, std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                                 { this->self().init(create_from_abi<param_span<const param_string>>(yolov5s_phai), create_from_abi<param_string>(yolov5s_racy_path), create_from_abi<param_span<const param_string>>(vehicle_phai), create_from_abi<param_string>(vehicle_racy_path), create_from_abi<param_span<const param_string>>(person_phai), create_from_abi<param_string>(person_racy_path), device); });
        }

        virtual std::int32_t G6_ABI_CALL detect(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
                                                std::int32_t order, abi_out_t<param_vector<valklyrs::result_info>> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().detect(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order)); });
        }

        virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().version()); });
        }
    };

    template <>
    struct abi_adapter<valklyrs::yolov5s_net>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, valklyrs::yolov5s_net>
        {
            void init(const param_string &yolov5s_racy_path, const param_string &vehicle_racy_path, const param_string &person_racy_path, std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(get_abi(yolov5s_racy_path), get_abi(vehicle_racy_path), get_abi(person_racy_path), get_abi(device)));
            }

            void init(param_span<const param_string> yolov5s_phai, const param_string &yolov5s_racy_path, param_span<const param_string> vehicle_phai, const param_string &vehicle_racy_path, param_span<const param_string> person_phai, const param_string &person_racy_path, std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(get_abi(yolov5s_phai), get_abi(yolov5s_racy_path), get_abi(vehicle_phai), get_abi(vehicle_racy_path), get_abi(person_phai), get_abi(person_racy_path), get_abi(device)));
            }

            param_vector<valklyrs::result_info> detect(param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
            {
                param_vector<valklyrs::result_info> result{nullptr};
                return (check_abi_result(this->self_abi().detect(get_abi(bitmap), channels, height, width, order, put_abi(result))), result);
            }

            param_string version() const
            {
                param_string result{nullptr};

                return (check_abi_result(this->self_abi().version(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::valklyrs
{
    struct yolov5s_net : exposing::inherits<yolov5s_net>
    {
        using inherits::inherits;
    };
}

#endif