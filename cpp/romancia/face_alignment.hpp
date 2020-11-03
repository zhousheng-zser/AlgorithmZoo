#pragma once

#include <abi/consumer.hpp>

#include "../longinus/face_info.hpp"

namespace glasssix::romancia
{
	struct face_alignment;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<romancia::face_alignment>
	{
		using identity_type = type_identity_interface;
		
		static constexpr guid id{ "63617281-7151-4134-8B43-ADE11A97177B" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t G6_ABI_CALL init(/*abi_in_t<param_string> mask_detector_model_path, */abi_in_t<param_string> antispoofing_model_path, std::int32_t device) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL get(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
				abi_in_t<exposing::param_vector<longinus::face_info>> faces, std::int32_t order, abi_out_t<param_vector< param_vector<std::uint8_t>>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL blur_detect(abi_in_t<longinus::face_info> face, abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
				std::int32_t order, abi_out_t<double> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL antispoofing(abi_in_t<longinus::face_info> face, abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
				std::int32_t order, abi_out_t<bool> result) noexcept = 0;
			//virtual std::int32_t G6_ABI_CALL mask_detect(abi_in_t<longinus::face_info> face, abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			//	std::int32_t order, abi_out_t<bool> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL mask_detect(abi_in_t<longinus::face_info> face, abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
				std::int32_t order, abi_out_t<double> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, romancia::face_alignment> : interface_vtable_base<Derived, romancia::face_alignment>
	{
		virtual std::int32_t G6_ABI_CALL init(/*abi_in_t<param_string> mask_detector_model_path, */abi_in_t<param_string> antispoofing_model_path, std::int32_t device) noexcept override
		{
			return abi_safe_call([&] { this->self().init(/*create_from_abi<param_string>(mask_detector_model_path), */create_from_abi<param_string>(antispoofing_model_path), device); });
		}

		virtual std::int32_t G6_ABI_CALL get(abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			abi_in_t<exposing::param_vector<longinus::face_info>> faces, std::int32_t order, abi_out_t<param_vector< param_vector<std::uint8_t>>> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().get(create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, 
				create_from_abi<exposing::param_vector<longinus::face_info>>(faces), order)); });
		}

		virtual std::int32_t G6_ABI_CALL blur_detect(abi_in_t<longinus::face_info> face, abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			std::int32_t order, abi_out_t<double> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().blur_detect(create_from_abi<longinus::face_info>(face), create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order)); });
		}
		virtual std::int32_t G6_ABI_CALL antispoofing(abi_in_t<longinus::face_info> face, abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			std::int32_t order, abi_out_t<bool> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().antispoofing(create_from_abi<longinus::face_info>(face), create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order)); });
		}
		//virtual std::int32_t G6_ABI_CALL mask_detect(abi_in_t<longinus::face_info> face, abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
		//	std::int32_t order, abi_out_t<bool> result) noexcept override
		//{
		//	return abi_safe_call([&] { *result = detach_abi(this->self().mask_detect(create_from_abi<longinus::face_info>(face), create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order)); });
		//}

		virtual std::int32_t G6_ABI_CALL mask_detect(abi_in_t<longinus::face_info> face, abi_in_t<param_span<std::uint8_t>> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			std::int32_t order, abi_out_t<double> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().mask_detect(create_from_abi<longinus::face_info>(face), create_from_abi<param_span<std::uint8_t>>(bitmap), channels, height, width, order)); });
		}

		virtual std::int32_t G6_ABI_CALL version(abi_out_t<param_string> result) noexcept override
		{
			return abi_safe_call([&] { *result = detach_abi(this->self().version()); });
		}
	};

	template<> struct abi_adapter<romancia::face_alignment>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, romancia::face_alignment>
		{
			void init(/*param_string mask_detector_model_path, */exposing::param_string antispoofing_model_path, std::int32_t device = -1) const
			{
				check_abi_result(this->self_abi().init(/*get_abi(mask_detector_model_path), */get_abi(antispoofing_model_path), get_abi(device)));
			}

			param_string version() const
			{
				param_string result{ nullptr };

				return (check_abi_result(this->self_abi().version(put_abi(result))), result);
			}

			param_vector< param_vector<std::uint8_t>> get(param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
				exposing::param_vector<longinus::face_info> faces, std::int32_t order) const
			{
				param_vector< param_vector<std::uint8_t>> result{ nullptr };
				return (check_abi_result(this->self_abi().get(get_abi(bitmap), channels, height, width, get_abi(faces), order, put_abi(result))), result);
			}

			double blur_detect(longinus::face_info face, param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
			{
				double result;
				return (check_abi_result(this->self_abi().blur_detect(get_abi(face), get_abi(bitmap), channels, height, width, order, put_abi(result))), result);
			}

			bool antispoofing(longinus::face_info face, param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
			{
				bool result;
				return (check_abi_result(this->self_abi().antispoofing(get_abi(face), get_abi(bitmap), channels, height, width, order, put_abi(result))), result);
			}

			//bool mask_detect(longinus::face_info face, param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
			//{
			//	bool result;
			//	return (check_abi_result(this->self_abi().mask_detect(get_abi(face), get_abi(bitmap), channels, height, width, order, put_abi(result))), result);
			//}

			double mask_detect(longinus::face_info face, param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
			{
				double result;
				return (check_abi_result(this->self_abi().mask_detect(get_abi(face), get_abi(bitmap), channels, height, width, order, put_abi(result))), result);
			}
		};
	};
}

namespace glasssix::romancia
{
	struct face_alignment : exposing::inherits<face_alignment>
	{
		using inherits::inherits;
	};
}
