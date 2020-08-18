#pragma once

#include "face_info.hpp"
#include "retina_net_native.hpp"

#include <abi/consumer.hpp>

namespace glasssix::longinus
{
	inline constexpr exposing::utf8_string_view longinus_face_info_qualified_name{ u8"g6.longinus.faceInfo" };

	class face_info_impl : public exposing::implements<face_info_impl, face_info>, public exposing::make_external_qualified_name<longinus_face_info_qualified_name>
	{
	public:
		face_info_impl();
		face_info_impl(face_info_internal& internal);
		~face_info_impl();

		std::int32_t x() const;
		std::int32_t y() const;
		std::int32_t width() const;
		std::int32_t height() const;
		float yaw() const;
		float pitch() const;
		float roll() const;
		float clarity() const;
		float confidence() const;
		exposing::param_vector<exposing::param_pair<float, float> > pts() const;

		void set_pts(exposing::param_vector<exposing::param_pair<float, float>> input);
		void set_yaw(float input);
		void set_pitch(float input);
		void set_roll(float input);
		void set_clarity(float input);
		void set_x(std::int32_t input);
		void set_y(std::int32_t input);
		void set_width(std::int32_t input);
		void set_height(std::int32_t input);
		void set_confidence(float input);

	private:
		face_info_internal internal_;
	};
}
