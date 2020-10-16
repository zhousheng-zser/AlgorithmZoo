#pragma once

#include "face_alignment.hpp"

#include <memory>

#include <abi/consumer.hpp>

namespace glasssix::romancia
{
	inline constexpr exposing::utf8_string_view romancia_face_alignment_qualified_name{ u8"g6.romancia.faceAlignment" };

	class face_alignment_internal;

	class face_alignment_impl : public exposing::implements<face_alignment_impl, face_alignment>, public exposing::make_external_qualified_name<romancia_face_alignment_qualified_name>
	{
	public:
		face_alignment_impl();
		~face_alignment_impl();

		void init(/*exposing::param_string mask_detector_model_path, */exposing::param_string antispoofing_model_path, std::int32_t device);

		exposing::param_string version() const;
		exposing::param_vector<exposing::param_vector<std::uint8_t>> get(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			exposing::param_vector<longinus::face_info> faces, std::int32_t order) const;
		double blur_detect(longinus::face_info face, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const;
		double antispoofing(longinus::face_info face, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const;
		double mask_detect(longinus::face_info face, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const;
	private:
		std::unique_ptr<face_alignment_internal> impl_;
	};
}
