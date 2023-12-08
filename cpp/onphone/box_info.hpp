#pragma once
#ifndef _ONPHONE_BOX_INFO_HPP_
#define _ONPHONE_BOX_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::onphone
{
    struct box_info;
}

namespace glasssix::exposing::impl
{
    template<>
    struct abi<onphone::box_info>
    {
        using identity_type = type_identity_interface;
        static constexpr guid id{ "{446E5551-BD1A-448E-A0B7-B16C02914D43}" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL x1(abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL x2(abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL y1(abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL y2(abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL category(abi_out_t<float> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL confidence(abi_out_t<float> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL phonelocal_list(abi_out_t<exposing::param_vector<std::int32_t>> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL phonescore_list(abi_out_t<exposing::param_vector<float>> result) noexcept = 0;

        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, onphone::box_info> : interface_vtable_base<Derived, onphone::box_info>
    {
        virtual std::int32_t G6_ABI_CALL x1(abi_out_t<std::int32_t> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().x1());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL x2(abi_out_t<std::int32_t> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().x2());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL y1(abi_out_t<std::int32_t> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().y1());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL y2(abi_out_t<std::int32_t> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().y2());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL category(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().category());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL confidence(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().confidence());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL phonelocal_list(abi_out_t<exposing::param_vector<std::int32_t>> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().phonelocal_list());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL phonescore_list(abi_out_t<exposing::param_vector<float>> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().phonescore_list());
                }
            );
        }

    };

    template <>
    struct abi_adapter<onphone::box_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, onphone::box_info>
        {
            std::int32_t x1() const
            {
                std::int32_t result;
                return (check_abi_result(this->self_abi().x1(put_abi(result))), result);
            }

            std::int32_t x2() const
            {
                std::int32_t result;
                return (check_abi_result(this->self_abi().x2(put_abi(result))), result);
            }

            std::int32_t y1() const
            {
                std::int32_t result;
                return (check_abi_result(this->self_abi().y1(put_abi(result))), result);
            }

            std::int32_t y2() const
            {
                std::int32_t result;
                return (check_abi_result(this->self_abi().y2(put_abi(result))), result);
            }

            float category() const
            {
                float result;
                return (check_abi_result(this->self_abi().category(put_abi(result))), result);
            }

            float confidence() const
            {
                float result;
                return (check_abi_result(this->self_abi().confidence(put_abi(result))), result);
            }

            exposing::param_vector<std::int32_t> phonelocal_list() const
            {
                exposing::param_vector<std::int32_t> result{ nullptr };
                return (check_abi_result(this->self_abi().phonelocal_list(put_abi(result))), result);
            }

            exposing::param_vector<float> phonescore_list() const
            {
                exposing::param_vector<float> result{ nullptr };
                return (check_abi_result(this->self_abi().phonescore_list(put_abi(result))), result);
            }

        };
    };
}

namespace glasssix::onphone
{
    struct box_info : exposing::inherits<box_info>
    {
        using inherits::inherits;
    };
}

#endif
