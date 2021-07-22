#pragma once

#include "face_info.hpp"
#include "retina_net_internal.hpp"

#include <abi/consumer.hpp>

namespace glasssix::longinus
{
	inline constexpr exposing::utf8_string_view longinus_face_info_qualified_name{ u8"g6.longinus.faceInfo" };

	class face_info_impl : public exposing::implements<face_info_impl, face_info>, public exposing::make_external_qualified_name<longinus_face_info_qualified_name>
	{
	public:
		face_info_impl();
		face_info_impl(const face_info_internal& internal);
		~face_info_impl();

		float x() const;
		float y() const;
		float width() const;
		float height() const;
		float yaw() const;
		float pitch() const;
		float roll() const;
		float clarity() const;
		float confidence() const;
		float has_mask() const;
		std::int32_t is_alive() const;
		std::int32_t glass_index() const;
		std::int32_t mask_index() const;
		exposing::param_vector<exposing::param_pair<float, float> > pts() const;

		void set_pts(exposing::param_vector<exposing::param_pair<float, float>> input);
		void set_yaw(float input);
		void set_pitch(float input);
		void set_roll(float input);
		void set_clarity(float input);
		void set_x(float input);
		void set_y(float input);
		void set_width(float input);
		void set_height(float input);
		void set_confidence(float input);
		void set_has_mask(float input);
		void set_is_alive(std::int32_t);
		void set_glass_index(std::int32_t);
		void set_mask_index(std::int32_t);

	private:
		face_info_internal internal_;
	};
}
