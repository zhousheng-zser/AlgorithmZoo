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

        static constexpr guid id{"{4E399FFA-49E0-41B4-B72E-A4EFC3204349}"};

        struct type : abi_unknown_object
        {  
            virtual std::int32_t G6_ABI_CALL location(abi_out_t<param_vector<float>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL strinfos(abi_out_t<param_vector<param_string>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL aligned_images(abi_out_t<param_vector<std::uint8_t>> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, plate::box_info> : interface_vtable_base<Derived, plate::box_info>
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

        virtual std::int32_t G6_ABI_CALL aligned_images(abi_out_t<param_vector<std::uint8_t>> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().aligned_images()); });
        }
    };

        template <>
    struct abi_adapter<plate::box_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, plate::box_info>
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

            param_vector<std::uint8_t> aligned_images() const
            {
                param_vector<std::uint8_t> result;

                return (check_abi_result(this->self_abi().aligned_images(put_abi(result))), result);
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