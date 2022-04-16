#ifndef _deepMarMobileNet_net_HPP_
#define _deepMarMobileNet_net_HPP_

#include <abi/consumer.hpp>

namespace glasssix::rifleman
{
    struct deepMarMobileNet_net;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<rifleman::deepMarMobileNet_net>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "{FD7C148F-23CD-438D-A156-25CC6F220363}" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL init(
                abi_in_t<param_string> deepMarMobileNet_racy_path,
                std::int32_t device
            ) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL init(
                abi_in_t<param_string> deepMarMobileNet_phai,
                abi_in_t<param_string> deepMarMobileNet_racy_path,
                std::int32_t device
            ) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL init(
                abi_in_t<param_span<const param_string>> deepMarMobileNet_phai,
                abi_in_t<param_string> deepMarMobileNet_racy_path, 
                std::int32_t device
            ) noexcept = 0;


            virtual std::int32_t G6_ABI_CALL detect(
                abi_in_t<param_span<std::uint8_t>> bitmap, 
                std::int32_t channels, 
                std::int32_t height, 
                std::int32_t width,
                std::int32_t order,
                abi_out_t<exposing::param_vector<exposing::param_vector<exposing::param_pair<float, exposing::param_string>>> > result
            ) noexcept = 0;

            //result
            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, rifleman::deepMarMobileNet_net> : interface_vtable_base<Derived, rifleman::deepMarMobileNet_net>
    {
        virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> deepMarMobileNet_racy_path, std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                { this->self().init(
                    create_from_abi<param_string>(deepMarMobileNet_racy_path),
                    device);
                });
        }

        virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> deepMarMobileNet_phai, abi_in_t<param_string> deepMarMobileNet_racy_path, std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                { this->self().init(
                    create_from_abi<param_string>(deepMarMobileNet_phai),
                    create_from_abi<param_string>(deepMarMobileNet_racy_path),
                    device);
                });
        }

        virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> deepMarMobileNet_phai, abi_in_t<param_string> deepMarMobileNet_racy_path, std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                { this->self().init(
                    create_from_abi<param_span<const param_string>>(deepMarMobileNet_phai),
                    create_from_abi<param_string>(deepMarMobileNet_racy_path),
                    device); 
                });
        }

        virtual std::int32_t G6_ABI_CALL detect(
            abi_in_t<param_span<std::uint8_t>> bitmap, 
            std::int32_t channels, 
            std::int32_t height, 
            std::int32_t width,
            std::int32_t order,
            abi_out_t<exposing::param_vector<exposing::param_vector<exposing::param_pair<float, exposing::param_string>>>> result ) noexcept override
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
    struct abi_adapter<rifleman::deepMarMobileNet_net>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, rifleman::deepMarMobileNet_net>
        {
            void init(param_string deepMarMobileNet_racy_path, std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(
                    get_abi(deepMarMobileNet_racy_path),
                    get_abi(device)
                ));
            }

            void init(param_string deepMarMobileNet_phai, param_string deepMarMobileNet_racy_path, std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(
                    get_abi(deepMarMobileNet_phai),
                    get_abi(deepMarMobileNet_racy_path),
                    get_abi(device)
                ));
            }

            void init(param_span<const param_string> deepMarMobileNet_phai, param_string deepMarMobileNet_racy_path, std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(
                    get_abi(deepMarMobileNet_phai), 
                    get_abi(deepMarMobileNet_racy_path), 
                    get_abi(device)
                ));
            }

            exposing::param_vector<exposing::param_vector<exposing::param_pair<float, exposing::param_string>>> detect(
                param_span<std::uint8_t> bitmap, 
                std::int32_t channels, 
                std::int32_t height, 
                std::int32_t width, 
                std::int32_t order) const
            {
                exposing::param_vector<exposing::param_vector<exposing::param_pair<float, exposing::param_string>>> result{ nullptr };
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

namespace glasssix::rifleman
{
    struct deepMarMobileNet_net : exposing::inherits<deepMarMobileNet_net>
    {
        using inherits::inherits;
    };
}

#endif