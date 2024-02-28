#pragma once

#include <map>
#include "pumptop_helmet_detector.hpp"


#include <memory>

#include <abi/consumer.hpp>

namespace glasssix::pumptop_helmet
{
	inline constexpr exposing::utf8_string_view pumptop_helmet_detector_qualified_name{ u8"g6.pumptop_helmet.pumptop_helmet_detector" };

    struct pumptop_helmet_info_internal
    {
        std::int32_t x1;
        std::int32_t y1;
        std::int32_t x2;
        std::int32_t y2;
        int category;
		/*
			{0: 'head', 1: 'helmet', 2: 'no'}
		*/
        float score;//人头置信度
        float helmet_score;//人头分类置信度
    };
	class pumptop_helmet_detector_impl : public exposing::implements<pumptop_helmet_detector_impl, pumptop_helmet_detector>, public exposing::make_external_qualified_name<pumptop_helmet_detector_qualified_name>
	{
	public:

		class impl;
		pumptop_helmet_detector_impl();
		~pumptop_helmet_detector_impl();

		void init(const exposing::param_string& models_directory, std::int32_t device);

		exposing::param_string version() const;
		exposing::param_vector<pumptop_helmet_info> detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, const exposing::param_hash_map<exposing::param_string,float>& param_map) const;
	private:
		std::unique_ptr<impl> impl_;
	};
}
