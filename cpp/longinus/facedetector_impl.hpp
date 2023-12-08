#pragma once

#include "facedetector.hpp"
#include <memory>

#include <abi/consumer.hpp>

namespace glasssix::longinus
{
	inline constexpr exposing::utf8_string_view longinus_facedetector_qualified_name{ u8"g6.longinus.facedetector" };

	class facedetector_base;

	class facedetector_impl : public exposing::implements<facedetector_impl, facedetector>, public exposing::make_external_qualified_name<longinus_facedetector_qualified_name>
	{
	public:
		facedetector_impl();
		~facedetector_impl();

		void init(const exposing::param_string& models_directory, int algo_type, int model_type, float nms_threshold, std::int32_t device);
		
		exposing::param_string version() const;
		exposing::param_vector<face_info> detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t min_size, float threshold, std::int32_t order, bool do_attributing) const;
		face_info single_trace(face_info face, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const;
		exposing::param_vector<exposing::param_vector<std::uint8_t>> center_scale_align(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			float scale, std::int32_t order) const;
	private:
		int algo_type_;
		std::unique_ptr<facedetector_base> impl_;
	};
}
