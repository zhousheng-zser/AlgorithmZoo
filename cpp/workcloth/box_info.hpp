#ifndef _WORKCLOTH_BOX_INFO_HPP_
#define _WORKCLOTH_BOX_INFO_HPP_

#include <abi/consumer.hpp>

namespace glasssix::workcloth
{
    struct box_info;
}

namespace glasssix::exposing::impl
{
    template<>
    struct abi<workcloth::box_info>
    {
        using identity_type = type_identity_interface;

        static constexpr guid id{ "{53A02D2A-7348-4EE7-A6A9-6A6D11535649}" };

        struct type : abi_unknown_object
        {
            virtual std::int32_t G6_ABI_CALL x1(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL y1(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL x2(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL y2(abi_out_t<int> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL up_rgb(abi_out_t<param_vector<int>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL lw_rgb(abi_out_t<param_vector<int>> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL up_strange(abi_out_t<bool> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL lw_strange(abi_out_t<bool> result) noexcept = 0;
            virtual std::int32_t G6_ABI_CALL score(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL category(abi_out_t<int> result) noexcept = 0;
        };
    };

    template <typename Derived>
    struct interface_vtable<Derived, workcloth::box_info> : interface_vtable_base<Derived, workcloth::box_info>
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

        virtual std::int32_t G6_ABI_CALL up_rgb(abi_out_t<param_vector<int>> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().up_rgb()); });
        }

        virtual std::int32_t G6_ABI_CALL lw_rgb(abi_out_t<param_vector<int>> result) noexcept override
        {
            return abi_safe_call([&]
                { *result = detach_abi(this->self().lw_rgb()); });
        }

        virtual std::int32_t G6_ABI_CALL up_strange(abi_out_t<bool> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().up_strange()); });
        }

        virtual std::int32_t G6_ABI_CALL lw_strange(abi_out_t<bool> result) noexcept override
        {
            return abi_safe_call([&]
                                 { *result = detach_abi(this->self().lw_strange()); });
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

    };

    template <>
    struct abi_adapter<workcloth::box_info>
    {
        template <typename Derived>
        struct type : enable_self_abi_awareness<Derived, workcloth::box_info>
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

            param_vector<int> up_rgb() const
            {
                param_vector<int> result;

                return (check_abi_result(this->self_abi().up_rgb(put_abi(result))), result);
            }

            param_vector<int> lw_rgb() const
            {
                param_vector<int> result;

                return (check_abi_result(this->self_abi().lw_rgb(put_abi(result))), result);
            }

			bool up_strange() const
            {
                bool result = 0;

                return (check_abi_result(this->self_abi().up_strange(put_abi(result))), result);
            }

			bool lw_strange() const
            {
                bool result = 0;

                return (check_abi_result(this->self_abi().lw_strange(put_abi(result))), result);
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
        };
    };
}

namespace glasssix::workcloth
{
    struct box_info : exposing::inherits<box_info>
    {
        using inherits::inherits;
    };
}
#endif