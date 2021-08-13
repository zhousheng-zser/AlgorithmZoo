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
			virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> racy_path, abi_in_t<param_string> tracker_racy_path, float nms, std::int32_t device) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> phai, abi_in_t<param_string> racy_path, abi_in_t<param_span<const param_string>> tracker_phai, abi_in_t<param_string> tracker_racy_path, float nms, std::int32_t device) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL detect(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t min_size, float threshold, std::int32_t order, bool do_attributing, abi_out_t<param_vector<longinus::face_info>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL single_trace(abi_in_t<longinus::face_info> face, abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order, abi_out_t<longinus::face_info> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL center_scale_align(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
				float scale, std::int32_t order, abi_out_t<param_vector< param_vector<std::uint8_t>>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, longinus::retina_net> : interface_vtable_base<Derived, longinus::retina_net>
	{
		virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> racy_path, abi_in_t<param_string> tracker_racy_path, float nms, std::int32_t device) noexcept override
		{
			return abi_safe_call([&] { this->self().init(create_from_abi<param_string>(racy_path), create_from_abi<param_string>(tracker_racy_path), nms, device); });
		}

		virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> phai, abi_in_t<param_string> racy_path, abi_in_t<param_span<const param_string>> tracker_phai, abi_in_t<param_string> tracker_racy_path, float nms, std::int32_t device) noexcept override
		{
			return abi_safe_call([&] { this->self().init(create_from_abi<param_span<const param_string>>(phai), create_from_abi<param_string>(racy_path), create_from_abi<param_span<const param_string>>(tracker_phai), create_from_abi<param_string>(tracker_racy_path), nms, device); });
		}

		virtual std::int32_t G6_ABI_CALL detect(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t min_size, float threshold, std::int32_t order, bool do_attributing, abi_out_t<param_vector<longinus::face_info>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().detect(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, min_size, threshold, order, do_attributing)); });
		}
		virtual std::int32_t G6_ABI_CALL single_trace(abi_in_t<longinus::face_info> face, abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order, abi_out_t<longinus::face_info> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().single_trace(create_from_abi<longinus::face_info>(face), create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order)); });
		}

		virtual std::int32_t G6_ABI_CALL center_scale_align(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			float scale, std::int32_t order, abi_out_t<param_vector< param_vector<std::uint8_t>>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().center_scale_align(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width,
				scale, order)); });
		}

		virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
		}
	};

	template<> struct abi_adapter<longinus::retina_net>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, longinus::retina_net>
		{
			void init(const param_string& racy_path, const param_string& tracker_racy_path, float nms = 0.4, std::int32_t device = -1) const
			{
				check_abi_result(this->self_abi().init(get_abi(racy_path), get_abi(tracker_racy_path), get_abi(nms), get_abi(device)));
			}

			void init(param_span<const param_string> phai, const param_string& racy_path, param_span<const param_string> tracker_phai, const param_string& tracker_racy_path, float nms = 0.4, std::int32_t device = -1) const
			{
				check_abi_result(this->self_abi().init(get_abi(phai), get_abi(racy_path), get_abi(tracker_phai), get_abi(tracker_racy_path), get_abi(nms), get_abi(device)));
			}

			param_vector<longinus::face_info> detect(param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t min_size, float threshold, std::int32_t order, bool do_attributing) const
			{
				param_vector<longinus::face_info> result{ nullptr };

				return (check_abi_result(this->self_abi().detect(get_abi(bitmap), get_abi(channels), get_abi(height), get_abi(width), get_abi(min_size), get_abi(threshold), get_abi(order), get_abi(do_attributing), put_abi(result))), result);
			}

			longinus::face_info single_trace(longinus::face_info face, param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
			{
				longinus::face_info result{nullptr};

				return (check_abi_result(this->self_abi().single_trace(get_abi(face), get_abi(bitmap), get_abi(channels), get_abi(height), get_abi(width), get_abi(order), put_abi(result))), result);
			}

			param_vector< param_vector<std::uint8_t>> center_scale_align(param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
				float scale, std::int32_t order) const
			{
				param_vector< param_vector<std::uint8_t>> result{ nullptr };
				return (check_abi_result(this->self_abi().center_scale_align(get_abi(bitmap), channels, height, width, scale, order, put_abi(result))), result);
			}

			param_string version() const
			{
				param_string result{ nullptr };

				return (check_abi_result(this->self_abi().version(put_abi(result))), result);
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
