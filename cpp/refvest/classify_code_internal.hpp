#ifndef __CLASSIFY_CODE_INTERNAL_HPP__
#define __CLASSIFY_CODE_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include "box_info.hpp"

#include "../posture/box_info.hpp"

#include <opencv2/opencv.hpp>

namespace glasssix::refvest
{
    struct PostureInfo
    {
        PostureInfo(posture::box_info& b_info) {
            x1 = b_info.x1();
            x2 = b_info.x2();
            y1 = b_info.y1();
            y2 = b_info.y2();
            score = b_info.score();
            category = b_info.category();

            auto key_points = b_info.key_points();
            for (size_t i = 0; i < (int)key_points.size() / 3; i++) {
				std::pair<cv::Point, float> key_p;
				key_p.first.x = key_points[i * 3];
                key_p.first.y = key_points[i * 3 + 1];
                key_p.second = key_points[i * 3 + 2];
                Kpoints.push_back(key_p);
            }
        }

        cv::Rect get_rect() {
            return cv::Rect{
                cv::Point(std::round(x1), std::round(y1)),
                cv::Point(std::round(x2), std::round(y2)) };
        }

		std::int32_t x1;
		std::int32_t y1;
		std::int32_t x2;
		std::int32_t y2;
		float score;
		int category;
        std::vector<std::pair<cv::Point, float>> Kpoints;
    };

    struct box_info_internal
    {
        int x1;
        int y1;
        int x2;
        int y2;
        float score;
        int category;
        exposing::param_string version;
    };

    class classify_code_internal
    {
    public:
        class impl;

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
        classify_code_internal(std::string_view model_directory, int device);

        virtual ~classify_code_internal();

        classify_code_internal(const classify_code_internal&) = delete;
        classify_code_internal& operator=(const classify_code_internal&) = delete;

        std::string version();

        exposing::param_vector<refvest::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const;

    private:
        std::unique_ptr<impl> impl_;
    };
}
#endif