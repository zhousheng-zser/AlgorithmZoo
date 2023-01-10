#ifndef _RING_MATERIAL_CODE_BOX_INFO_HPP_
#define _RING_MATERIAL_CODE_BOX_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::ring
{
    struct box_info;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<ring::box_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{"{4E399FFA-49E0-41B4-B72E-A4EFC3204349}"};

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL location(abi_out_t<param_vector<float>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL strinfos(abi_out_t<param_vector<param_string>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL messages(abi_out_t<param_vector<param_string>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL angle(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL cut_roi(abi_out_t<param_vector<std::uint8_t>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL cut_roi_width(abi_out_t<std::int32_t> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL cut_roi_height(abi_out_t<std::int32_t> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, ring::box_info> : interface_vtable_base<Derived, ring::box_info>
    {
        virtual std::int32_t G6_ABI_CALL location(abi_out_t<param_vector<float>> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().location()); });
        }

        virtual std::int32_t G6_ABI_CALL strinfos(abi_out_t<param_vector<param_string>> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().strinfos()); });
        }

        virtual std::int32_t G6_ABI_CALL messages(abi_out_t<param_vector<param_string>> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().messages()); });
        }

        virtual std::int32_t G6_ABI_CALL angle(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().angle()); });
        }

        virtual std::int32_t G6_ABI_CALL cut_roi(abi_out_t<param_vector<std::uint8_t>> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().cut_roi()); });
        }

        virtual std::int32_t G6_ABI_CALL cut_roi_width(abi_out_t<std::int32_t> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().cut_roi_width()); });
        }

        virtual std::int32_t G6_ABI_CALL cut_roi_height(abi_out_t<std::int32_t> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().cut_roi_height()); });
        }
    };

    template <>
    struct abi_adapter<ring::box_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, ring::box_info>
        {
            param_vector<float> location() const
            {
                param_vector<float> result;

                return (check_abi_result(this->self_abi().location(put_abi(result))), result);
            }

            param_vector<param_string> strinfos() const
            {
                param_vector<param_string> result;

                return (check_abi_result(this->self_abi().strinfos(put_abi(result))), result);
            }

            param_vector<param_string> messages() const
            {
                param_vector<param_string> result;

                return (check_abi_result(this->self_abi().messages(put_abi(result))), result);
            }

            float angle() const
            {
                float result;

                return (check_abi_result(this->self_abi().angle(put_abi(result))), result);
            }

            param_vector<std::uint8_t> cut_roi() const
            {
                param_vector<std::uint8_t> result;

                return (check_abi_result(this->self_abi().cut_roi(put_abi(result))), result);
            }

            std::int32_t cut_roi_width() const
            {
                std::int32_t result;

                return (check_abi_result(this->self_abi().cut_roi_width(put_abi(result))), result);
            }

            std::int32_t cut_roi_height() const
            {
                std::int32_t result;

                return (check_abi_result(this->self_abi().cut_roi_height(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::ring
{
    struct box_info : exposing::inherits<box_info>
    {
        using inherits::inherits;
    };
}
#endif