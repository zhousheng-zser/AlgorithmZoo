#ifndef __HAT_INFO_HPP__
#define __HAT_INFO_HPP__

#include <abi/consumer.hpp>

namespace glasssix::gungnir
{
    struct hat_info;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<gungnir::hat_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{"{D7B27FB9-8C63-45ED-A236-AD644D3097F5}"};

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL x(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL y(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL width(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL height(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL prob(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL label(abi_out_t<float> result) noexcept = 0;

            // virtual std::int32_t G6_ABI_CALL set_x(abi_in_t<float> input) noexcept = 0;
            // virtual std::int32_t G6_ABI_CALL set_y(abi_in_t<float> input) noexcept = 0;
            // virtual std::int32_t G6_ABI_CALL set_width(abi_in_t<float> input) noexcept = 0;
            // virtual std::int32_t G6_ABI_CALL set_height(abi_in_t<float> input) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, gungnir::hat_info> : interface_vtable_base<Derived, gungnir::hat_info>
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

        virtual std::int32_t G6_ABI_CALL prob(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().prob()); });
        }

        virtual std::int32_t G6_ABI_CALL label(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().label()); });
        }
    };

    template <>
    struct abi_adapter<gungnir::hat_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, gungnir::hat_info>
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
            float prob() const
            {
                float result = 0;

                return (check_abi_result(this->self_abi().prob(put_abi(result))), result);
            }
            float label() const
            {
                float result = 0;

                return (check_abi_result(this->self_abi().label(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::gungnir
{
    struct hat_info : exposing::inherits<hat_info>
    {
        using inherits::inherits;
    };
}
#endif