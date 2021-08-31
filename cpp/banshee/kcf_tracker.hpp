#ifndef KCF_TRACKER
#define KCF_TRACKER

#include "track_info.hpp"
#include <abi/consumer.hpp>

namespace glasssix::banshee
{
    struct kcf_tracker;
    struct Object;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<banshee::kcf_tracker>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{"{4F444652-FF0B-408E-A511-0236F0736867}"};

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<std::uint8_t>> bitmap, abi_in_t<std::int32_t> width, abi_in_t<std::int32_t> height, abi_in_t<std::int32_t> x, abi_in_t<std::int32_t> y, abi_in_t<std::int32_t> roi_width, abi_in_t<std::int32_t> roi_height) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL update(abi_in_t<param_span<std::uint8_t>> bitmap, abi_in_t<std::int32_t> width, abi_in_t<std::int32_t> height, abi_out_t<banshee::track_info> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, banshee::kcf_tracker> : interface_vtable_base<Derived, banshee::kcf_tracker>
    {
        virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<std::uint8_t>> bitmap, abi_in_t<std::int32_t> width, abi_in_t<std::int32_t> height, abi_in_t<std::int32_t> x, abi_in_t<std::int32_t> y, abi_in_t<std::int32_t> roi_width, abi_in_t<std::int32_t> roi_height) noexcept override
        {
            return abi_safe_call([&]
                                 { this->self().init(create_from_abi<param_span<std::uint8_t>>(bitmap), create_from_abi<std::int32_t>(width), create_from_abi<std::int32_t>(height), create_from_abi<std::int32_t>(x), create_from_abi<std::int32_t>(y), create_from_abi<std::int32_t>(roi_width), create_from_abi<std::int32_t>(roi_height)); });
        }

        virtual std::int32_t G6_ABI_CALL update(abi_in_t<param_span<std::uint8_t>> bitmap, abi_in_t<std::int32_t> width, abi_in_t<std::int32_t> height, abi_out_t<banshee::track_info> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().update(create_from_abi<param_span<std::uint8_t>>(bitmap), create_from_abi<std::int32_t>(width), create_from_abi<std::int32_t>(height))); });
        }

        virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().version()); });
        }
    };

    template <>
    struct abi_adapter<banshee::kcf_tracker>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, banshee::kcf_tracker>
        {
            void init(param_span<std::uint8_t> bitmap, std::int32_t width, std::int32_t height, std::int32_t x, std::int32_t y, std::int32_t roi_width, std::int32_t roi_height) const
            {
                check_abi_result(this->self_abi().init(get_abi(bitmap), get_abi(width), get_abi(height), get_abi(x), get_abi(y), get_abi(roi_width), get_abi(roi_height)));
            }

            banshee::track_info update(param_span<std::uint8_t> bitmap, std::int32_t width, std::int32_t height) const
            {
                banshee::track_info result{nullptr};
                return (check_abi_result(this->self_abi().update(get_abi(bitmap), put_abi(width), put_abi(height), put_abi(result))), result);
            }

            param_string version() const
            {
                param_string result{nullptr};

                return (check_abi_result(this->self_abi().version(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::banshee
{
    struct kcf_tracker : exposing::inherits<kcf_tracker>
    {
        using inherits::inherits;
    };
}
#endif