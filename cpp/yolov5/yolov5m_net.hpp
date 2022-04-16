#ifndef _yolov5m_NET_HPP_
#define _yolov5m_NET_HPP_

#include <abi/consumer.hpp>
#include "result_info.hpp"

namespace glasssix::yolov5
{
    struct yolov5m_net;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<yolov5::yolov5m_net>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "{FD7C148F-23CD-438D-A156-25CC6F220363}" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL init(
                abi_in_t<param_string> yolov5m_racy_path,
                std::int32_t device
            ) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL init(
                abi_in_t<param_string> yolov5m_phai,
                abi_in_t<param_string> yolov5m_racy_path,
                std::int32_t device
            ) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL init(
                abi_in_t<param_span<const param_string>> yolov5m_phai,
                abi_in_t<param_string> yolov5m_racy_path, 
                std::int32_t device
            ) noexcept = 0;


            virtual std::int32_t G6_ABI_CALL detect(
                abi_in_t<param_span<std::uint8_t>> bitmap, 
                std::int32_t channels, 
                std::int32_t height, 
                std::int32_t width,
                std::int32_t order,
                abi_out_t<yolov5::result_info> result
            ) noexcept = 0;

            //result
            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, yolov5::yolov5m_net> : interface_vtable_base<Derived, yolov5::yolov5m_net>
    {
        virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> yolov5m_racy_path, std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                { this->self().init(
                    create_from_abi<param_string>(yolov5m_racy_path),
                    device);
                });
        }

        virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> yolov5m_phai, abi_in_t<param_string> yolov5m_racy_path, std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                { this->self().init(
                    create_from_abi<param_string>(yolov5m_phai),
                    create_from_abi<param_string>(yolov5m_racy_path),
                    device);
                });
        }

        virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> yolov5m_phai, abi_in_t<param_string> yolov5m_racy_path, std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                { this->self().init(
                    create_from_abi<param_span<const param_string>>(yolov5m_phai),
                    create_from_abi<param_string>(yolov5m_racy_path),
                    device); 
                });
        }

        virtual std::int32_t G6_ABI_CALL detect(
            abi_in_t<param_span<std::uint8_t>> bitmap, 
            std::int32_t channels, 
            std::int32_t height, 
            std::int32_t width,
            std::int32_t order,
            abi_out_t<yolov5::result_info> result ) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().detect(
                    create_from_abi<param_span<std::uint8_t>>(bitmap),
                    channels,
                    height,
                    width,
                    order));
                });
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
    struct abi_adapter<yolov5::yolov5m_net>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, yolov5::yolov5m_net>
        {
            void init(param_string yolov5m_racy_path, std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(
                    get_abi(yolov5m_racy_path),
                    get_abi(device)
                ));
            }

            void init(param_string yolov5m_phai, param_string yolov5m_racy_path, std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(
                    get_abi(yolov5m_phai),
                    get_abi(yolov5m_racy_path),
                    get_abi(device)
                ));
            }

            void init(param_span<const param_string> yolov5m_phai, param_string yolov5m_racy_path, std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(
                    get_abi(yolov5m_phai), 
                    get_abi(yolov5m_racy_path), 
                    get_abi(device)
                ));
            }

            yolov5::result_info detect(
                param_span<std::uint8_t> bitmap, 
                std::int32_t channels, 
                std::int32_t height, 
                std::int32_t width, 
                std::int32_t order) const
            {
                yolov5::result_info result{ nullptr };
                return (
                    check_abi_result(this->self_abi().detect(
                    get_abi(bitmap),
                    channels,
                    height,
                    width,
                    order,
                    put_abi(result))),
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

namespace glasssix::yolov5
{
    struct yolov5m_net : exposing::inherits<yolov5m_net>
    {
        using inherits::inherits;
    };
}

#endif