#pragma once


#include "../longinus/face_info.hpp"
#include "face_attributes_detector.hpp"


#include <memory>

#include <abi/consumer.hpp>

namespace glasssix::face_attributes
{
	inline constexpr exposing::utf8_string_view face_attributes_detector_qualified_name{ u8"g6.face_attributes.face_attributes_detector" };

	struct face_attribute_info_internal
	{
		int gender;	//0 : 女 1 : 男
		int age;	//0 : <15 1 : 15< <35 2 : 35< <55 3 : 55<
		int mask;	//0 : 不戴口罩 1 : 戴口罩
		int glass;	//0 : 不戴眼镜 1 : 戴眼镜
	};

	class face_attributes_detector_impl : public exposing::implements<face_attributes_detector_impl, face_attributes_detector>, public exposing::make_external_qualified_name<face_attributes_detector_qualified_name>
	{
	public:
	
		class impl;
		face_attributes_detector_impl();
		~face_attributes_detector_impl();

		void init(const exposing::param_string& str_params);
		exposing::param_string version() const;
		exposing::param_string execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map);

		exposing::param_vector<face_attribute_info> detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const;
	private:
		std::unique_ptr<impl> impl_;
	};
}
