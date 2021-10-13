#ifndef _MATERIAL_CODE_track_info_HPP_
#define _MATERIAL_CODE_track_info_HPP_

#include <abi/consumer.hpp>

namespace glasssix::banshee
{
    struct track_info;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<banshee::track_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{"{036EA7EF-4DF8-4E74-BA66-31D5F11FD560}"};

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL x(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL y(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL width(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL height(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL prob(abi_out_t<float> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, banshee::track_info> : interface_vtable_base<Derived, banshee::track_info>
    {
        virtual std::int32_t G6_ABI_CALL x(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().x()); });
        }

        virtual std::int32_t G6_ABI_CALL y(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().y()); });
        }

        virtual std::int32_t G6_ABI_CALL width(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().width()); });
        }

        virtual std::int32_t G6_ABI_CALL height(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().height()); });
        }

        virtual std::int32_t G6_ABI_CALL prob(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().prob()); });
        }
    };

    template <>
    struct abi_adapter<banshee::track_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, banshee::track_info>
        {
            float x() const
            {
                float result;

                return (check_abi_result(this->self_abi().x(put_abi(result))), result);
            }

            float y() const
            {
                float result;

                return (check_abi_result(this->self_abi().y(put_abi(result))), result);
            }

            float width() const
            {
                float result;

                return (check_abi_result(this->self_abi().width(put_abi(result))), result);
            }

            float height() const
            {
                float result;

                return (check_abi_result(this->self_abi().height(put_abi(result))), result);
            }

            float prob() const
            {
                float result;

                return (check_abi_result(this->self_abi().prob(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::banshee
{
    struct track_info : exposing::inherits<track_info>
    {
        using inherits::inherits;
    };
}
#endif