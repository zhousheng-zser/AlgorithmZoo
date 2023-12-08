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

#include <Primitives/tensor_conversions.hpp>


#ifdef BUILD_DEBUG_INFO
#include <opencv2/highgui/highgui.hpp>



#define GetShowRatio(visual_img) std::min(float(1920.f / visual_img.cols), float(1080.f / visual_img.rows)) * 0.75
#define ShowResize(visual_img, showRatio) cv::resize(visual_img, visual_img, cv::Size(), showRatio, showRatio);
#endif // BUILD_DEBUG_INFO

namespace glasssix::flame
{
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

	struct FlameBox {
		float xmin;
		float ymin;
		float xmax;
		float ymax;
		float score;
		int cid = 0;

		FlameBox(float cx, float cy, float w, float h, float the_score) {
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

        exposing::param_vector<flame::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const;

    private:
        std::unique_ptr<impl> impl_;
    };

	static inline std::shared_ptr<glasssix::memory::tensor<float>> tensor_transpose_0132(const std::shared_ptr<glasssix::memory::tensor<float>>& bottom) {
		int num = bottom->num();
		int channels = bottom->channels();
		int height = bottom->height();
		int width = bottom->width();
		//CHECK_EQ(bottom->channels(), D * C);
		auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{num, channels, width, height}, -1, memory::NCHW);

		int W_step = width; //8400
		int countb = bottom->count();

		for (int nc = 0; nc < num; nc++) {
			const float* bottom_ptr = bottom->cpu_data() + countb * nc; // bottom_ptr -> D * HW
			float* top_ptr = top->mutable_cpu_data() + countb * nc; // top_ptr -> HW * D

			for (int i = 0; i < W_step; i++) { //for 8400
				for (int line = 0; line < height; line++) { //for 6
					top_ptr[i * height + line] = bottom_ptr[line * W_step + i];
				}
			}
		}
		return top;
	}

}
#endif