#ifndef _WANDER_BOX_INFO_HPP_
#define _WANDER_BOX_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::wander
{
    struct box_info;
}

namespace glasssix::exposing::impl
{
    template<>
    struct abi<wander::box_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "4C151F47-C811-4C49-B781-7686A86F0A96" };

        struct type : abi_unknown_object
        {   
            virtual std::int32_t G6_ABI_CALL id(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL x1(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL y1(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL x2(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL y2(abi_out_t<int> result) noexcept = 0;
            virtual float G6_ABI_CALL confidence(abi_out_t<float> result) noexcept = 0;
            virtual float G6_ABI_CALL cosine_similarity(abi_out_t<float> result) noexcept = 0;
            virtual double G6_ABI_CALL first_show_time(abi_out_t<double> result) noexcept = 0;
            virtual double G6_ABI_CALL last_show_time(abi_out_t<double> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, wander::box_info> : interface_vtable_base<Derived, wander::box_info>
    {
        virtual std::int32_t G6_ABI_CALL id(abi_out_t<int> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().id()); });
        }

        virtual std::int32_t G6_ABI_CALL x1(abi_out_t<int> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().x1()); });
        }

        virtual std::int32_t G6_ABI_CALL y1(abi_out_t<int> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().y1()); });
        }

        virtual std::int32_t G6_ABI_CALL x2(abi_out_t<int> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().x2()); });
        }

        virtual std::int32_t G6_ABI_CALL y2(abi_out_t<int> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().y2()); });
        }

        virtual float G6_ABI_CALL cosine_similarity(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().cosine_similarity()); });
        }

         virtual double G6_ABI_CALL first_show_time(abi_out_t<double> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().first_show_time()); });
        }

         virtual double G6_ABI_CALL last_show_time(abi_out_t<double> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().last_show_time()); });
        }
         virtual float G6_ABI_CALL confidence(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().confidence()); });
        }
    };

    template <>
    struct abi_adapter<wander::box_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, wander::box_info>
        {
            
            int id() const
            {
                int result = 0;
                return (check_abi_result(this->self_abi().id(put_abi(result))), result);
            }

            int x1() const
            {
                int result = 0;
                return (check_abi_result(this->self_abi().x1(put_abi(result))), result);
            }

            int y1() const
            {
                int result = 0;
                return (check_abi_result(this->self_abi().y1(put_abi(result))), result);
            }

            int x2() const
            {
                int result = 0;

                return (check_abi_result(this->self_abi().x2(put_abi(result))), result);
            }

            int y2() const
            {
                int result = 0;

                return (check_abi_result(this->self_abi().y2(put_abi(result))), result);
            }

            float confidence() const
            {
                float result = 0.f;
                return (check_abi_result(this->self_abi().confidence(put_abi(result))), result);
            }

            double first_show_time() const
            {
                double result = 0.f;
                return (check_abi_result(this->self_abi().first_show_time(put_abi(result))), result);
            }

            double last_show_time() const
            {
                double result = 0.f;
                return (check_abi_result(this->self_abi().last_show_time(put_abi(result))), result);
                
            }

            float cosine_similarity() const
            {
                float result = 0.f;
                return (check_abi_result(this->self_abi().cosine_similarity(put_abi(result))), result);       
            }

        };
    };
}

namespace glasssix::wander
{
    struct box_info : exposing::inherits<box_info>
    {
        using inherits::inherits;
    };
}
#endif