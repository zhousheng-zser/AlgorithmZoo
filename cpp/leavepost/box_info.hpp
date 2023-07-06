#ifndef __LEAVEPOST_HPP__
#define __LEAVEPOST_HPP__

#include <abi/consumer.hpp>

namespace glasssix::leavepost
{
    struct box_info;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<leavepost::box_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{"85507728-84E1-47DC-BCC6-F6B842E2E84D"};

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL x(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL y(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL width(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL height(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL confidence(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL label(abi_out_t<float> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, leavepost::box_info> : interface_vtable_base<Derived, leavepost::box_info>
    {
        virtual std::int32_t G6_ABI_CALL x(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().x()); });
        }

        virtual std::int32_t G6_ABI_CALL y(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().y()); });
        }

        virtual std::int32_t G6_ABI_CALL width(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().width()); });
        }

        virtual std::int32_t G6_ABI_CALL height(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().height()); });
        }

        virtual std::int32_t G6_ABI_CALL confidence(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().confidence()); });
        }

        virtual std::int32_t G6_ABI_CALL label(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().label()); });
        }
    };

    template <>
    struct abi_adapter<leavepost::box_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, leavepost::box_info>
        {
            float x() const
            {
                float result = 0;

                return (check_abi_result(this->self_abi().x(put_abi(result))), result);
            }
            float y() const
            {
                float result = 0;

                return (check_abi_result(this->self_abi().y(put_abi(result))), result);
            }
            float width() const
            {
                float result = 0;

                return (check_abi_result(this->self_abi().width(put_abi(result))), result);
            }
            float height() const
            {
                float result = 0;

                return (check_abi_result(this->self_abi().height(put_abi(result))), result);
            }
            float confidence() const
            {
                float result = 0;

                return (check_abi_result(this->self_abi().confidence(put_abi(result))), result);
            }
            float label() const
            {
                float result = 0;

                return (check_abi_result(this->self_abi().label(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::leavepost
{
    struct box_info : exposing::inherits<box_info>
    {
        using inherits::inherits;
    };
}
#endif