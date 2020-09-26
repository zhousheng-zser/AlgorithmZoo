#pragma once

#include "face_info.hpp"

#include <abi/consumer.hpp>

namespace glasssix::longinus
{
	struct retina_net;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<longinus::retina_net>
	{
		using identity_type = type_identity_interface;

		static constexpr guid id{ "725D32DB-75BA-46CF-8EB6-5F052816A5E6" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> racy_path, float nms, std::int32_t device) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> phai, abi_in_t<param_string> racy_path, float nms, std::int32_t device) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL get(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t min_size, float threshold, std::int32_t order, abi_out_t<param_vector<longinus::face_info>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, longinus::retina_net> : interface_vtable_base<Derived, longinus::retina_net>
	{
		virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> racy_path, float nms, std::int32_t device) noexcept override
		{
			return abi_safe_call([&] { this->self().init(create_from_abi<param_string>(racy_path), nms, device); });
		}

		virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> phai, abi_in_t<param_string> racy_path, float nms, std::int32_t device) noexcept override
		{
			return abi_safe_call([&] { this->self().init(create_from_abi<param_span<const param_string>>(phai), create_from_abi<param_string>(racy_path), nms, device); });
		}

		virtual std::int32_t G6_ABI_CALL get(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t min_size, float threshold, std::int32_t order, abi_out_t<param_vector<longinus::face_info>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().get(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, min_size, threshold, order)); });
		}

		virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
		}

		virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> phai_path, abi_in_t<param_string> racy_path, float nms, std::int32_t device) noexcept override
		{
			return abi_safe_call([&] { this->self().init(create_from_abi<param_string>(phai_path), create_from_abi<param_string>(racy_path), nms, device); });
		}
	};

	template<> struct abi_adapter<longinus::retina_net>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, longinus::retina_net>
		{
			void init(param_string racy_path, float nms = 0.4, std::int32_t device = -1) const
			{
				check_abi_result(this->self_abi().init(get_abi(racy_path), get_abi(nms), get_abi(device)));
			}

			void init(param_span<const param_string> phai, param_string racy_path, float nms = 0.4, std::int32_t device = -1) const
			{
				check_abi_result(this->self_abi().init(get_abi(phai), get_abi(racy_path), get_abi(nms), get_abi(device)));
			}

			param_vector<longinus::face_info> get(param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t min_size, float threshold, std::int32_t order) const
			{
				param_vector<longinus::face_info> result{ nullptr };

				return (check_abi_result(this->self_abi().get(get_abi(bitmap), get_abi(channels), get_abi(height), get_abi(width), get_abi(min_size), get_abi(threshold), get_abi(order), put_abi(result))), result);
			}

			param_string version() const
			{
				param_string result{ nullptr };

				return (check_abi_result(this->self_abi().version(put_abi(result))), result);
			}

			void init(param_string phai_path, param_string racy_path, float nms = 0.4, std::int32_t device = -1) const
			{
				check_abi_result(this->self_abi().init(get_abi(phai_path), get_abi(racy_path), get_abi(nms), get_abi(device)));
			}
		};
	};
}

namespace glasssix::longinus
{
	struct retina_net : exposing::inherits<retina_net>
	{
		using inherits::inherits;
	};
}
