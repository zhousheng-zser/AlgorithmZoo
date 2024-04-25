#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>
#include "box_info.hpp"
#include <GenPipeline/GenPipeTools.hpp>


namespace glasssix::fighting
{
    struct PersonBBox :public GenPipTools::YoloBoxBase {
    public:
        using YoloBoxBase::YoloBoxBase; //Inheriting Constructors
        int frame_id;
    };

	struct BoxInfoInternal
	{
		int x1;
		int y1;
		int x2;
		int y2;
		float score;
		int category = -1;

		BoxInfoInternal(cv::Rect rect, float sc, float isFightgThres) {
			x1 = rect.x;
			x2 = rect.x + rect.width;
			y1 = rect.y;
			y2 = rect.y + rect.height;
			score = sc;
			category = sc > isFightgThres;
		}

		cv::Rect get_rect() const {
			return cv::Rect{
				cv::Point(std::round(x1), std::round(y1)),
				cv::Point(std::round(x2), std::round(y2)) };
		}

		void add(int add_x, int add_y) {
			x1 += add_x;
			x2 += add_x;
			y1 += add_y;
			y2 += add_y;
		}
	};

    class detect_code_internal
    {
    public:
        class impl;

        detect_code_internal(const detect_code_internal &) = delete;

        detect_code_internal &operator=(const detect_code_internal &) = delete;

        detect_code_internal(std::string_view model_directory, int device, int batch);

        virtual ~detect_code_internal();

		exposing::param_vector<fighting::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string,float>& param_map_std);

        std::string version();

    private:
        std::unique_ptr<impl> impl_;
    };
}
