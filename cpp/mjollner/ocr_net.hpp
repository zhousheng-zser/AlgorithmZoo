#ifndef _OCR_NET_HPP_
#define _OCR_NET_HPP_

#include "box_info.hpp"
#include <abi/consumer.hpp>

namespace glasssix::mjollner
{
    struct ocr_net;
    struct Object;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<mjollner::ocr_net>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{"{B04D2DCC-70FF-4AB2-8ECD-DD7B7B628C98}"};

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> det_racy_path, abi_in_t<param_string> rec_racy_path, abi_in_t<param_string> alphabet_path, std::int32_t device) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> det_phai, abi_in_t<param_string> det_racy_path, abi_in_t<param_span<const param_string>> rec_phai, abi_in_t<param_string> rec_racy_path, abi_in_t<param_string> alphabet_path, std::int32_t device) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL detect(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
                                                    std::int32_t order, std::int32_t x, std::int32_t y, std::int32_t roi_width, std::int32_t roi_height, abi_out_t<param_vector<mjollner::box_info>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, mjollner::ocr_net> : interface_vtable_base<Derived, mjollner::ocr_net>
    {
        virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> det_racy_path, abi_in_t<param_string> rec_racy_path, abi_in_t<param_string> alphabet_path, std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                                 { this->self().init(create_from_abi<param_string>(det_racy_path), create_from_abi<param_string>(rec_racy_path), create_from_abi<param_string>(alphabet_path), device); });
        }

        virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> det_phai, abi_in_t<param_string> det_racy_path, abi_in_t<param_span<const param_string>> rec_phai, abi_in_t<param_string> rec_racy_path, abi_in_t<param_string> alphabet_path, std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                                 { this->self().init(create_from_abi<param_span<const param_string>>(det_phai), create_from_abi<param_string>(det_racy_path), create_from_abi<param_span<const param_string>>(rec_phai), create_from_abi<param_string>(rec_racy_path), create_from_abi<param_string>(alphabet_path), device); });
        }

        virtual std::int32_t G6_ABI_CALL detect(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
                                                std::int32_t order, std::int32_t x, std::int32_t y, std::int32_t roi_width, std::int32_t roi_height, abi_out_t<param_vector<mjollner::box_info>> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().detect(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order, x, y, roi_width, roi_height)); });
        }

        virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().version()); });
        }
    };

    template <>
    struct abi_adapter<mjollner::ocr_net>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, mjollner::ocr_net>
        {
            void init(const param_string &det_racy_path, const param_string &rec_racy_path, const param_string &alphabet_path, std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(get_abi(det_racy_path), get_abi(rec_racy_path), get_abi(alphabet_path), get_abi(device)));
            }

            void init(param_span<const param_string> det_phai, const param_string &det_racy_path, param_span<const param_string> rec_phai, const param_string &rec_racy_path, const param_string &alphabet_path, std::int32_t device) const
            {
                check_abi_result(this->self_abi().init(get_abi(det_phai), get_abi(det_racy_path), get_abi(rec_phai), get_abi(rec_racy_path), get_abi(alphabet_path), get_abi(device)));
            }

            param_vector<mjollner::box_info> detect(param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order, std::int32_t x, std::int32_t y, std::int32_t roi_width, std::int32_t roi_height) const
            {
                param_vector<mjollner::box_info> result{nullptr};
                return (check_abi_result(this->self_abi().detect(get_abi(bitmap), channels, height, width, order, x, y, roi_width, roi_height, put_abi(result))), result);
            }

            param_string version() const
            {
                param_string result{nullptr};

                return (check_abi_result(this->self_abi().version(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::mjollner
{
    struct ocr_net : exposing::inherits<ocr_net>
    {
        using inherits::inherits;
    };
}

#endif