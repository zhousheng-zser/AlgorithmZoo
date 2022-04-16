#pragma once

#include <abi/consumer.hpp>
#include <opencv2/opencv.hpp>

namespace glasssix::yolov5
{
    struct result_info_internal
    {
        exposing::param_vector<exposing::param_vector<float>> coordinates;
        exposing::param_vector<float> conf;
        exposing::param_vector<exposing::param_string> cls;
    };
}

namespace glasssix::yolov5
{
    struct result_info;
}

namespace glasssix::exposing::impl
{
    template <>
    struct abi<yolov5::result_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "{F14FD740-A24A-4235-83CC-1442EE5EE7F2}" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL coordinates(abi_out_t<param_vector<param_vector<float>>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL conf(abi_out_t<param_vector<float>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL cls(abi_out_t<param_vector<param_string>> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, yolov5::result_info> : interface_vtable_base<Derived, yolov5::result_info>
    {
        virtual std::int32_t G6_ABI_CALL coordinates(abi_out_t<param_vector<param_vector<float>>> result) noexcept override
        {
            return abi_safe_call(
                [&] {
                    *result = detach_abi(this->self().coordinates());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL conf(abi_out_t<param_vector<float>> result) noexcept override
        {
            return abi_safe_call(
                [&] {
                    *result = detach_abi(this->self().conf());
                }
            );
        }

        virtual std::int32_t G6_ABI_CALL cls(abi_out_t<param_vector<param_string>> result) noexcept override
        {
            return abi_safe_call(
                [&] {
                    *result = detach_abi(this->self().cls());
                }
            );
        }
    };

    template <>
    struct abi_adapter<yolov5::result_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, yolov5::result_info>
        {
            param_vector<param_vector<float>> coordinates() const
            {
                param_vector<param_vector<float>> result;
                return (check_abi_result(this->self_abi().coordinates(put_abi(result))), result);
            }

            param_vector<float> conf() const
            {
                param_vector<float> result;
                return (check_abi_result(this->self_abi().conf(put_abi(result))), result);
            }

            param_vector<param_string> cls() const
            {
                param_vector<param_string> result;
                return (check_abi_result(this->self_abi().cls(put_abi(result))), result);
            }

        };
    };
}

namespace glasssix::yolov5
{
    struct result_info : exposing::inherits<result_info>
    {
        using inherits::inherits;
    };
}
