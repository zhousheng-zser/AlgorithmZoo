#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>
#include <opencv2/core/core.hpp>
#include "box_info.hpp"

namespace glasssix::vehicle
{
	struct ObjBox {
		float xmin;
		float ymin;
		float xmax;
		float ymax;
		float score;
		int cid = 0;

		ObjBox() = default;

		ObjBox(cv::Rect rect, float the_score) {
			xmin = rect.x;
			xmax = rect.x + rect.width;
			ymin = rect.y;
			ymax = rect.y + rect.height;
			score = the_score;
		}

		ObjBox(float cx, float cy, float w, float h, float the_score) {
			xmin = cx - w / 2;
			xmax = cx + w / 2;
			ymin = cy - h / 2;
			ymax = cy + h / 2;
			score = the_score;
		}

		void add(cv::Point2f point) {
			xmin += point.x;
			ymin += point.y;
			xmax += point.x;
			ymax += point.y;
		}
		void add(int x, int y) {
			xmin += x;
			ymin += y;
			xmax += x;
			ymax += y;
		}

		void mul_ratio(float ratio) {
			xmin = xmin * ratio;
			ymin = ymin * ratio;
			xmax = xmax * ratio;
			ymax = ymax * ratio;
		}

		std::vector<cv::Point2f> points() {
			std::vector<cv::Point2f> rect_points{
				cv::Point2f(std::round(xmin),std::round(ymin)),
				cv::Point2f(std::round(xmin),std::round(ymax)),
				cv::Point2f(std::round(xmax),std::round(ymin)),
				cv::Point2f(std::round(xmax),std::round(ymax)) };
			return rect_points;
		}

		cv::Rect get_rect() {
			return cv::Rect{
				cv::Point(std::round(xmin), std::round(ymin)),
				cv::Point(std::round(xmax), std::round(ymax)) };
		}

		cv::Point2f get_center() {
			auto p1 = cv::Point2f(std::round(xmin), std::round(ymin));
			auto p2 = cv::Point2f(std::round(xmax), std::round(ymax));
			return (p1 + p2) / 2;
		}

		float get_area() {
			return (xmax - xmin) * (ymax - ymin);
		}
	};

    class classify_code_internal
    {
    public:
        class impl;

        classify_code_internal(const classify_code_internal &) = delete;

        classify_code_internal &operator=(const classify_code_internal &) = delete;

        classify_code_internal(std::string_view model_directory, int device);

        virtual ~classify_code_internal();

        exposing::param_vector<vehicle::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string,float>& param_map_std);

        std::string version();

    private:
        std::unique_ptr<impl> impl_;
    };
}
