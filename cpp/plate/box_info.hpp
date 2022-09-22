#ifndef _PLATE_BOX_INFO_HPP_
#define _PLATE_BOX_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::plate
{
    struct box_info;
}

namespace glasssix::exposing::impl
{
    template<>
    struct abi<plate::box_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "{4E399FFA-49E0-41B4-B72E-A4EFC3204349}" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL x(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL y(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL width(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL height(abi_out_t<float> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL strinfos(abi_out_t<param_vector<param_string>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL aligned_images(abi_out_t<param_vector<std::uint8_t>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL confidence(abi_out_t<float> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL set_x(abi_in_t<float> input) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL set_y(abi_in_t<float> input) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL set_width(abi_in_t<float> input) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL set_height(abi_in_t<float> input) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL set_confidence(abi_in_t<float> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, plate::box_info> : interface_vtable_base<Derived, plate::box_info>
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

        virtual std::int32_t G6_ABI_CALL strinfos(abi_out_t<param_vector<param_string>> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().strinfos()); });
        }

        virtual std::int32_t G6_ABI_CALL aligned_images(abi_out_t<param_vector<std::uint8_t>> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().aligned_images()); });
        }

        virtual std::int32_t G6_ABI_CALL confidence(abi_out_t<float> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().confidence()); });
        }

        virtual std::int32_t G6_ABI_CALL set_x(abi_in_t<float> input) noexcept override
        {
            return abi_safe_call([&]
                { this->self().set_x(create_from_abi<float>(input)); });
        }
        virtual std::int32_t G6_ABI_CALL set_y(abi_in_t<float> input) noexcept override
        {
            return abi_safe_call([&]
                { this->self().set_y(create_from_abi<float>(input)); });
        }
        virtual std::int32_t G6_ABI_CALL set_width(abi_in_t<float> input) noexcept override
        {
            return abi_safe_call([&]
                { this->self().set_width(create_from_abi<float>(input)); });
        }
        virtual std::int32_t G6_ABI_CALL set_height(abi_in_t<float> input) noexcept override
        {
            return abi_safe_call([&]
                { this->self().set_height(create_from_abi<float>(input)); });
        }

        virtual std::int32_t G6_ABI_CALL set_confidence(abi_in_t<float> input) noexcept override
        {
            return abi_safe_call([&]
                { this->self().set_confidence(create_from_abi<float>(input)); });
        }

    };

    template <>
    struct abi_adapter<plate::box_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, plate::box_info>
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

            param_vector<param_string> strinfos() const
            {
                param_vector<param_string> result;

                return (check_abi_result(this->self_abi().strinfos(put_abi(result))), result);
            }

            param_vector<std::uint8_t> aligned_images() const
            {
                param_vector<std::uint8_t> result;

                return (check_abi_result(this->self_abi().aligned_images(put_abi(result))), result);
            }

            float confidence() const
            {
                float result = 0.f;

                return (check_abi_result(this->self_abi().confidence(put_abi(result))), result);
            }

            void set_x(float input)
            {
                check_abi_result(this->self_abi().set_x(get_abi(input)));
            }
            void set_y(float input)
            {
                check_abi_result(this->self_abi().set_y(get_abi(input)));
            }
            void set_width(float input)
            {
                check_abi_result(this->self_abi().set_width(get_abi(input)));
            }
            void set_height(float input)
            {
                check_abi_result(this->self_abi().set_height(get_abi(input)));
            }
            void set_confidence(float input)
            {
                check_abi_result(this->self_abi().set_confidence(get_abi(input)));
            }
        };
    };
}

namespace glasssix::plate
{
    struct box_info : exposing::inherits<box_info>
    {
        using inherits::inherits;
    };
}
#endif