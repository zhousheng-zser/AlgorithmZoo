#pragma once

#include "face_attribute_info.hpp"
#include "face_attributes_detector_impl.hpp"

namespace glasssix::face_attributes
{

	inline constexpr exposing::utf8_string_view face_attribute_info_qualified_name{ u8"g6.face_attributes.face_attribute_Info" };

	class face_attribute_info_impl : public exposing::implements<face_attribute_info_impl, face_attribute_info>, public exposing::make_external_qualified_name<face_attribute_info_qualified_name>
	{
	public:
		face_attribute_info_impl();
		face_attribute_info_impl(const face_attributes::face_attribute_info_internal& internal);
		~face_attribute_info_impl();

		int age() const;
		int gender() const;
		int glass() const;
		int mask() const;
		
	private:
		face_attribute_info_internal internal_;
	};
}
