#ifndef __YOLO_NET_HPP__
#define __YOLO_NET_HPP__

#include "hat_info.hpp"
#include <abi/consumer.hpp>

namespace glasssix::gungnir
{
    struct yolo_net;
    struct Object;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<gungnir::yolo_net>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{"{3AA99D8C-FF6A-47CB-A3F0-344FFB232FEC}"};

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> racy_path, std::int32_t device) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> phai, abi_in_t<param_string> racy_path, std::int32_t device) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL detect(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
                                                    std::int32_t order, abi_out_t<param_vector<param_vector<float>>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, gungnir::yolo_net> : interface_vtable_base<Derived, gungnir::yolo_net>
    {
        virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> racy_path, std::int32_t device) noexcept override
        {
            return abi_safe_call([&] { this->self().init(create_from_abi<param_string>(racy_path), device); });
        }

        virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> phai, abi_in_t<param_string> racy_path, std::int32_t device) noexcept override
        {
            return abi_safe_call([&] { this->self().init(create_from_abi<param_span<const param_string>>(phai), create_from_abi<param_string>(racy_path), device); });
        }

        virtual std::int32_t G6_ABI_CALL detect(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
                                                std::int32_t order, abi_out_t<param_vector<param_vector<float>>> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().detect(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order)); });
        }

        virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
        }
    };

    template <>
    struct abi_adapter<gungnir::yolo_net>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, gungnir::yolo_net>
        {
            void init(const param_string &racy_path, std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(get_abi(racy_path), get_abi(device)));
            }

            void init(param_span<const param_string> phai, const param_string &racy_path, std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(get_abi(phai), get_abi(racy_path), get_abi(device)));
            }

            param_vector<gungnir::hat_info> detect(param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
            {
                param_vector<hat_info> result{nullptr};
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

namespace glasssix::gungnir
{
    struct yolo_net : exposing::inherits<yolo_net>
    {
        using inherits::inherits;
    };
}
#endif
