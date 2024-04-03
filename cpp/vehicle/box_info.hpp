#pragma once
#ifndef _VEHICLE_BOX_INFO_HPP_
#define _VEHICLE_BOX_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::vehicle
{
    struct box_info;
}

namespace glasssix::exposing::impl
{
    template<>
    struct abi<vehicle::box_info>
    {
        using identity_type = type_identity_interface;
        static constexpr guid id{ "{6AE56276-408D-4BD3-B1C2-C0ECD7C42C27}" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL x1(abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL x2(abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL x3(abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL x4(abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL x5(abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL y1(abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL y2(abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL y3(abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL y4(abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL y5(abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL category(abi_out_t<std::int32_t> result) noexcept = 0;

        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, vehicle::box_info> : interface_vtable_base<Derived, vehicle::box_info>
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

        virtual std::int32_t G6_ABI_CALL x3(abi_out_t<std::int32_t> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().x3());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL x4(abi_out_t<std::int32_t> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().x4());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL x5(abi_out_t<std::int32_t> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().x5());
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

        virtual std::int32_t G6_ABI_CALL y3(abi_out_t<std::int32_t> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().y3());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL y4(abi_out_t<std::int32_t> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().y4());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL y5(abi_out_t<std::int32_t> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().y5());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().score());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL category(abi_out_t<std::int32_t> result) noexcept override
        {
            return abi_safe_call([&]
                {
                    *result = detach_abi(this->self().category());
                }
            );
        }

    };

    template <>
    struct abi_adapter<vehicle::box_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, vehicle::box_info>
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
            std::int32_t x3() const
            {
                std::int32_t result;
                return (check_abi_result(this->self_abi().x3(put_abi(result))), result);
            }

            std::int32_t x4() const
            {
                std::int32_t result;
                return (check_abi_result(this->self_abi().x4(put_abi(result))), result);
            }

            std::int32_t x5() const
            {
                std::int32_t result;
                return (check_abi_result(this->self_abi().x5(put_abi(result))), result);
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

            std::int32_t y3() const
            {
                std::int32_t result;
                return (check_abi_result(this->self_abi().y3(put_abi(result))), result);
            }

            std::int32_t y4() const
            {
                std::int32_t result;
                return (check_abi_result(this->self_abi().y4(put_abi(result))), result);
            }

            std::int32_t y5() const
            {
                std::int32_t result;
                return (check_abi_result(this->self_abi().y5(put_abi(result))), result);
            }

            float score() const
            {
                float result;
                return (check_abi_result(this->self_abi().score(put_abi(result))), result);
            }

            std::int32_t category() const
            {
                std::int32_t result;
                return (check_abi_result(this->self_abi().category(put_abi(result))), result);
            }

        };
    };
}

namespace glasssix::vehicle
{
    struct box_info : exposing::inherits<box_info>
    {
        using inherits::inherits;
    };
}

#endif
