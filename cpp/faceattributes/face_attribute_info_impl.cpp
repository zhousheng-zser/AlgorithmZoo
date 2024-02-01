#include "face_attribute_info_impl.hpp"

namespace glasssix::face_attributes
{
	face_attribute_info_impl::face_attribute_info_impl()
	{
	}

	face_attribute_info_impl::face_attribute_info_impl(const face_attributes::face_attribute_info_internal& internal) : internal_(internal)
	{
	}
	face_attribute_info_impl::~face_attribute_info_impl()
	{
	}

	int face_attribute_info_impl::age() const
	{
		return internal_.age;
	}
	
	int face_attribute_info_impl::gender() const
	{
		return internal_.gender;
	}

	int face_attribute_info_impl::glass() const
	{
		return internal_.glass;
	}

	int face_attribute_info_impl::mask() const
	{
		return internal_.mask;
	}		
}
