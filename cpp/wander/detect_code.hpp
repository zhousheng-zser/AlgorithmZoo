#ifndef _WANDER_DETECT_CODE_HPP_
#define _WANDER_DETECT_CODE_HPP_

#include "box_info.hpp"
#include <abi/consumer.hpp>
#include <algo_plugin_interface.hpp>
namespace glasssix::wander
{
    struct detect_code;
}



namespace glasssix::exposing::impl
{
    template <>
    struct abi<wander::detect_code>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "34AA76A2-524D-48AC-8A3F-F2510490F20F" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL init(
                abi_in_t<param_string> str_params) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL execute(abi_in_t<param_hash_map<param_string, unknown_object>> input_params_map, abi_out_t<param_string> result) = 0;
            virtual std::int32_t G6_ABI_CALL detect(
                abi_in_t<param_span<std::uint8_t>> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width,
                std::int32_t roi_x,
                std::int32_t roi_y,
                std::int32_t roi_width,
                std::int32_t roi_height,
                abi_in_t<exposing::param_hash_map<exposing::param_string, double>> param_map_abi,
                abi_in_t<exposing::param_vector<pedestrian::box_info>> pedestrain_info_abi,
                abi_out_t<exposing::param_vector<wander::box_info>> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL remove_library(  std::int32_t id, abi_out_t<param_string> result ) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL remove_person_by_index( std::int32_t device_id, std::int32_t id, abi_out_t<param_string> result ) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, wander::detect_code> : interface_vtable_base<Derived, wander::detect_code>
    {

        virtual std::int32_t G6_ABI_CALL init(
            abi_in_t<param_string> str_params) noexcept override
        {
            return abi_safe_call([&]
                { this->self().init(
                    create_from_abi<param_string>(str_params)); });
        }

        virtual std::int32_t G6_ABI_CALL execute(abi_in_t<param_hash_map<param_string, unknown_object>> input_params_map, abi_out_t<param_string> result)noexcept override
        {
            return abi_safe_call([&] { *result = detach_abi(this->self().execute(create_from_abi<param_hash_map<param_string, unknown_object>>(input_params_map))); });
        }

        virtual std::int32_t G6_ABI_CALL detect(abi_in_t<param_span<std::uint8_t>> bitmap,
            std::int32_t channels,
            std::int32_t height,
            std::int32_t width,
            std::int32_t roi_x,
            std::int32_t roi_y,
            std::int32_t roi_width,
            std::int32_t roi_height,
            abi_in_t<exposing::param_hash_map<exposing::param_string, double>> param_map_abi,
            abi_in_t<exposing::param_vector<pedestrian::box_info>> pedestrain_info_abi,
            abi_out_t<exposing::param_vector<wander::box_info>> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().detect(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, roi_x, roi_y, roi_width, roi_height,
                   create_from_abi<exposing::param_hash_map<exposing::param_string, double>>(param_map_abi), create_from_abi<exposing::param_vector<pedestrian::box_info>>(pedestrain_info_abi) )); });
        }

        virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call(
                [&]
                {
                    *result = detach_abi(this->self().version());
                }
                );
        }


        virtual std::int32_t G6_ABI_CALL remove_library(std::int32_t id, abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call(
                [&]
                {
                    *result = detach_abi(this->self().remove_library(id));
                }
                );
        }

        virtual std::int32_t G6_ABI_CALL remove_person_by_index(std::int32_t device_id,std::int32_t id, abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call(
                [&]
                {
                    *result = detach_abi(this->self().remove_person_by_index(device_id,id));
                }
                );
        }

    };

    template <>
    struct abi_adapter<wander::detect_code>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, wander::detect_code>
        {
            void init(
                const param_string& str_params) const
            {
                check_abi_result(this->self_abi().init(
                    get_abi(str_params)));
            }

            param_string execute(const param_hash_map<param_string, unknown_object>& input_params_map)
            {
                param_string result{ nullptr };
                return (check_abi_result(this->self_abi().execute(get_abi(input_params_map), put_abi(result))), result);
            }

            exposing::param_vector<wander::box_info> detect(
                param_span<std::uint8_t> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width,
                std::int32_t roi_x,
                std::int32_t roi_y,
                std::int32_t roi_width,
                std::int32_t roi_height,
                const exposing::param_hash_map<exposing::param_string, double>& param_map_abi,
                const exposing::param_vector<pedestrian::box_info>& pedestrain_info_abi) const
            {
                exposing::param_vector<wander::box_info> result{ nullptr };

                return (check_abi_result(
                    this->self_abi().detect(
                        get_abi(bitmap),
                        channels,
                        height,
                        width,
                        roi_x,
                        roi_y,
                        roi_width,
                        roi_height,
                        get_abi(param_map_abi),
                        get_abi(pedestrain_info_abi),
                        put_abi(result))
                ),
                result);
            }

            param_string version() const
            {
                param_string result{ nullptr };

                return (check_abi_result(this->self_abi().version(put_abi(result))), result);
            }


            param_string remove_library(std::int32_t id) const
            {
                param_string result{ nullptr };

                return (check_abi_result(this->self_abi().remove_library(id, put_abi(result))), result);
            }

            param_string remove_person_by_index(std::int32_t device_id,std::int32_t id) const
            {
                param_string result{ nullptr };

                return (check_abi_result(this->self_abi().remove_person_by_index(device_id, id, put_abi(result))), result);
            }

        };
    };
}

namespace glasssix::wander
{
    struct detect_code : exposing::inherits<detect_code,glasssix::exposing::nessus::algo_plugin_interface>
    {
        using inherits::inherits;
    };
}
#endif
