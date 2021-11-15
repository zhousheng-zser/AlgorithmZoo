#pragma once

#include <abi/consumer.hpp>
#include "../longinus/face_info.hpp"

namespace glasssix::damocles
{
	struct anti_spoofing;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<damocles::anti_spoofing>
	{
		using identity_type = type_identity_interface;

		static constexpr guid id{ "CA161DB5-85C1-4D22-A70C-08469734F283" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> FASMV2_racy_path, abi_in_t<param_string> land65_racy_path, std::int32_t device, bool use_int8) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> FASMV2_phai, abi_in_t<param_string> FASMV2_racy_path,
				abi_in_t<param_span<const param_string>> land65_phai, abi_in_t<param_string> land65_racy_path, std::int32_t device) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL spoofing_detect(abi_in_t<param_vector<longinus::face_info>> faces, abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
				std::int32_t order, abi_out_t<param_vector<param_vector<float>>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL presentation_attack_detect(std::int32_t action_cmd, abi_in_t<longinus::face_info> face, abi_in_t<param_span<std::uint8_t>> bitmap, 
				std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order, abi_out_t<bool> result) noexcept = 0;

			virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, damocles::anti_spoofing> : interface_vtable_base<Derived, damocles::anti_spoofing>
	{
		virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_string> FASMV2_racy_path, abi_in_t<param_string> land65_racy_path, std::int32_t device, bool use_int8) noexcept override
		{
			return abi_safe_call([&] { this->self().init(create_from_abi<param_string>(FASMV2_racy_path), create_from_abi<param_string>(land65_racy_path), device, use_int8); });
		}

		virtual std::int32_t G6_ABI_CALL init(abi_in_t<param_span<const param_string>> FASMV2_phai, abi_in_t<param_string> FASMV2_racy_path,
			abi_in_t<param_span<const param_string>> land65_phai, abi_in_t<param_string> land65_racy_path, std::int32_t device) noexcept override
		{
			return abi_safe_call([&] { this->self().init(create_from_abi<param_span<const param_string>>(FASMV2_phai), create_from_abi<param_string>(FASMV2_racy_path), create_from_abi<param_span<const param_string>>(land65_phai), create_from_abi<param_string>(land65_racy_path), device); });
		}

		virtual std::int32_t G6_ABI_CALL spoofing_detect(abi_in_t<param_vector<longinus::face_info>> faces, abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			std::int32_t order, abi_out_t<param_vector<param_vector<float>>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().spoofing_detect(create_from_abi<param_vector<longinus::face_info>>(faces), create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order)); });
		}

		virtual std::int32_t G6_ABI_CALL presentation_attack_detect(std::int32_t action_cmd, abi_in_t<longinus::face_info> face, abi_in_t<param_span<std::uint8_t>> bitmap,
			std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order, abi_out_t<bool> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().presentation_attack_detect(action_cmd, create_from_abi<longinus::face_info>(face), create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order)); });
		}

		virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
		}
	};

	template<> struct abi_adapter<damocles::anti_spoofing>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, damocles::anti_spoofing>
		{
			void init(const param_string& FASMV2_racy_path, const param_string& land65_racy_path, std::int32_t device, bool use_int8) const
			{
				check_abi_result(this->self_abi().init(get_abi(FASMV2_racy_path), get_abi(land65_racy_path), get_abi(device), get_abi(use_int8)));
			}

			void init(param_span<const param_string> FASMV2_phai, const param_string& FASMV2_racy_path, param_span<const param_string> land65_phai, const param_string& land65_racy_path, std::int32_t device) const
			{
				check_abi_result(this->self_abi().init(get_abi(FASMV2_phai), get_abi(FASMV2_racy_path), get_abi(land65_phai), get_abi(land65_racy_path), get_abi(device)));
			}

			param_vector<param_vector<float>> spoofing_detect(const param_vector<longinus::face_info>& faces, param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
			{
				param_vector<param_vector<float>> result{ nullptr };
				return (check_abi_result(this->self_abi().spoofing_detect(get_abi(faces), get_abi(bitmap), channels, height, width, order, put_abi(result))), result);
			}

			bool presentation_attack_detect(std::int32_t action_cmd, const longinus::face_info& face, param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
			{
				bool result = false;
				return (check_abi_result(this->self_abi().presentation_attack_detect(action_cmd, get_abi(face), get_abi(bitmap), channels, height, width, order, put_abi(result))), result);
			}

			param_string version() const
			{
				param_string result{ nullptr };

				return (check_abi_result(this->self_abi().version(put_abi(result))), result);
			}
		};
	};
}

namespace glasssix::damocles
{
	struct anti_spoofing : exposing::inherits<anti_spoofing>
	{
		using inherits::inherits;
	};
}
