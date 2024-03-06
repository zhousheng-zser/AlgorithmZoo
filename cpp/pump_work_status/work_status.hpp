#pragma once

#include <abi/consumer.hpp>

namespace glasssix::pump_work_status
{
    struct work_status;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<pump_work_status::work_status>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{"646EB399-3782-40AA-8915-7349263095E0"};

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL init(std::int32_t device) noexcept = 0;
            
            virtual std::int32_t G6_ABI_CALL status(
                abi_in_t<param_span<std::uint8_t>> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width,
                exposing::param_vector<std::int32_t> rois,
                abi_in_t<exposing::param_hash_map<exposing::param_string, float>> param_map_abi,
                abi_out_t<std::int32_t> result) noexcept = 0;

            virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;

        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, pump_work_status::work_status> : interface_vtable_base<Derived, pump_work_status::work_status>
    {

         virtual std::int32_t G6_ABI_CALL status(abi_in_t<param_span<std::uint8_t>> bitmap,
            std::int32_t channels,
            std::int32_t height,
            std::int32_t width,
            exposing::param_vector<int> rois,
            abi_in_t<exposing::param_hash_map<exposing::param_string, float>> param_map_abi,
            abi_out_t<std::int32_t> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().status(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, rois,
                   create_from_abi<exposing::param_hash_map<exposing::param_string, float>>(param_map_abi))); });
        }


        virtual std::int32_t G6_ABI_CALL init( std::int32_t device) noexcept override
        {
            return abi_safe_call([&]
                                 { this->self().init(device); });
        }
        

        virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().version()); });
        }
    };

    template <>
    struct abi_adapter<pump_work_status::work_status>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, pump_work_status::work_status>
        {
           
             std::int32_t status(
                param_span<std::uint8_t> bitmap,
                std::int32_t channels,
                std::int32_t height,
                std::int32_t width,
                exposing::param_vector<std::int32_t> rois,
                const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const
            {
                std::int32_t result;
                return (check_abi_result(
                    this->self_abi().status(
                        get_abi(bitmap),
                        channels,
                        height,
                        width,
                        rois,
                        get_abi(param_map_abi),
                        put_abi(result))
                ),
                result);
            }


            void init(std::int32_t device) const
            {
                check_abi_result(this->self_abi().init( get_abi(device)));
            }
           
            param_string version() const
            {
                param_string result{nullptr};

                return (check_abi_result(this->self_abi().version(put_abi(result))), result);
            }
        };
    };
}

namespace glasssix::pump_work_status
{
    struct work_status : exposing::inherits<work_status>
    {
        using inherits::inherits;
    };
}
