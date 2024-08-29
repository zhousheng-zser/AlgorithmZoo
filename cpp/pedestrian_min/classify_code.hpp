#ifndef _PEDESTRIAN_MIN_CLASSIFY_CODE_HPP_
#define _PEDESTRIAN_MIN_CLASSIFY_CODE_HPP_

#include "box_info.hpp"
#include <abi/consumer.hpp>
#include <algo_plugin_interface.hpp>

namespace glasssix::pedestrian_min
{
    struct classify_code;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<pedestrian_min::classify_code>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "6ACD7A6C-6EE0-0A47-1531-888A6123B65B" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> str_params) = 0;

            virtual std::int32_t G6_ABI_CALL execute(abi_in_t<param_hash_map<param_string, unknown_object>> input_params_map, abi_out_t<param_string> result) = 0;

            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, pedestrian_min::classify_code> : interface_vtable_base<Derived, pedestrian_min::classify_code>
    {
        virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> str_params) override
        {
            return abi_safe_call([&] { this->self().init(create_from_abi<param_string>(str_params)); });
        }

        virtual std::int32_t G6_ABI_CALL execute(abi_in_t<param_hash_map<param_string, unknown_object>> input_params_map, abi_out_t<param_string> result) override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().execute(create_from_abi<param_hash_map<param_string, unknown_object>>(input_params_map))); });
        }

        virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
        }
    };

    template <>
    struct abi_adapter<pedestrian_min::classify_code>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, pedestrian_min::classify_code>
        {
            void init(const param_string& str_params)
            {
                check_abi_result(this->self_abi().init(get_abi(str_params)));
            }

            param_string execute(const param_hash_map<param_string, unknown_object>& input_params_map)
            {
                param_string result{ nullptr };
                return (check_abi_result(this->self_abi().execute(get_abi(input_params_map), put_abi(result))), result);
            }


            param_string version() const
            {
                param_string result{ nullptr };

                return (check_abi_result(this->self_abi().version(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::pedestrian_min
{
    struct classify_code : exposing::inherits<classify_code, exposing::nessus::algo_plugin_interface>
    {
        using inherits::inherits;
    };
}

#endif
