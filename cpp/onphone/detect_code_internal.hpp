#ifndef __DETECT_CODE_INTERNAL_HPP__
#define __DETECT_CODE_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>
#include <opencv2/opencv.hpp>
#include "box_info.hpp"

#include "../posture/box_info.hpp"
#include "../head/box_info.hpp"

#include <GenPipeline/GenPipeTools.hpp>

namespace glasssix::onphone
{
	struct PhoneBox :public GenPipTools::YoloBoxBase {
	public:
		using YoloBoxBase::YoloBoxBase; //Inheriting Constructors
	};

	struct HeadInfo :public GenPipTools::YoloBoxBase {
	public:
		HeadInfo(head::box_info& hinfo) {
			xmin = hinfo.x1();
			ymin = hinfo.y1();
			xmax = hinfo.x2();
			ymax = hinfo.y2();
			score = hinfo.score();
		}

		cv::Rect DetPhoneRegion() {
			int width = xmax - xmin;
			int height = ymax - ymin;
			int area = width * height;
			int x1 = xmin - 0.25f * width;
			int y1 = ymin + 0.50f * height;
			return cv::Rect(x1, y1, width * 1.5, height * 0.75);
		}
	};

	struct box_info_internal
	{
		int x1;
		int y1;
		int x2;
		int y2;
		float category;
		float confidence;
		exposing::param_vector<std::int32_t> phonelocal_list;
		exposing::param_vector<float> phonescore_list;

		box_info_internal() {
			phonelocal_list = exposing::make_param_vector<std::int32_t>();
			phonescore_list = exposing::make_param_vector<float>();
		}

		void set(HeadInfo& obj) {
			x1 = obj.xmin;
			y1 = obj.ymin;
			x2 = obj.xmax;
			y2 = obj.ymax;
			confidence = obj.score;
		}
	};

    class detect_code_internal
    {
    public:
        class impl;

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
        detect_code_internal(std::string_view model_directory, int device);

        virtual ~detect_code_internal();

        detect_code_internal(const detect_code_internal&) = delete;
        detect_code_internal& operator=(const detect_code_internal&) = delete;

        std::string version();

		exposing::param_vector<onphone::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<head::box_info> head_info_list, std::map<std::string, float>& param_map) const;
        exposing::param_vector<onphone::box_info> exdetect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map) const;

    private:
        std::unique_ptr<impl> impl_;
    };
}
#endif