#ifndef _PEDESTRIAN_BOX_INFO_HPP_
#define _PEDESTRIAN_BOX_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::pedestrian
{
    struct box_info;
}

namespace glasssix::exposing::impl
{
    template<>
    struct abi<pedestrian::box_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "{6647E928-AB9E-4D82-BDFB-941B30A76FC9}" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL x1(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL y1(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL x2(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL y2(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL category(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL set_x1(abi_in_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL set_y1(abi_in_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL set_x2(abi_in_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL set_y2(abi_in_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL set_score(abi_in_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL set_category(abi_in_t<int> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, pedestrian::box_info> : interface_vtable_base<Derived, pedestrian::box_info>
    {

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

        virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().score()); });
        }
        virtual std::int32_t G6_ABI_CALL category(abi_out_t<int> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().category()); });
        }

        virtual std::int32_t G6_ABI_CALL set_x1(abi_in_t<int> input) noexcept override
        {
            return abi_safe_call([&]
                { this->self().set_x1(create_from_abi<int>(input)); });
        }

        virtual std::int32_t G6_ABI_CALL set_y1(abi_in_t<int> input) noexcept override
        {
            return abi_safe_call([&]
                { this->self().set_y1(create_from_abi<int>(input)); });
        }

        virtual std::int32_t G6_ABI_CALL set_x2(abi_in_t<int> input) noexcept override
        {
            return abi_safe_call([&]
                { this->self().set_x2(create_from_abi<int>(input)); });
        }

        virtual std::int32_t G6_ABI_CALL set_y2(abi_in_t<int> input) noexcept override
        {
            return abi_safe_call([&]
                { this->self().set_y2(create_from_abi<int>(input)); });
        }
        virtual std::int32_t G6_ABI_CALL set_score(abi_in_t<float> input) noexcept override
        {
            return abi_safe_call([&]
                { this->self().set_score(create_from_abi<float>(input)); });
        }
        virtual std::int32_t G6_ABI_CALL set_category(abi_in_t<int> input) noexcept override
        {
            return abi_safe_call([&]
                { this->self().set_category(create_from_abi<int>(input)); });
        }

    };

    template <>
    struct abi_adapter<pedestrian::box_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, pedestrian::box_info>
        {

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

			float score() const
            {
                float result = 0;

                return (check_abi_result(this->self_abi().score(put_abi(result))), result);
            }
			int category() const
            {
                int result = 0;

                return (check_abi_result(this->self_abi().category(put_abi(result))), result);
            }
            void set_x1(int input)
            {
                check_abi_result(this->self_abi().set_x1(get_abi(input)));
            }
            void set_y1(int input)
            {
                check_abi_result(this->self_abi().set_y1(get_abi(input)));
            }
            void set_x2(int input)
            {
                check_abi_result(this->self_abi().set_x2(get_abi(input)));
            }
            void set_y2(int input)
            {
                check_abi_result(this->self_abi().set_y2(get_abi(input)));
            }
            void set_score(float input)
            {
                check_abi_result(this->self_abi().set_score(get_abi(input)));
            }
            void set_category(int input)
            {
                check_abi_result(this->self_abi().set_category(get_abi(input)));
            }
        };
    };
}

namespace glasssix::pedestrian
{
    struct box_info : exposing::inherits<box_info>
    {
        using inherits::inherits;
    };
}
#endif