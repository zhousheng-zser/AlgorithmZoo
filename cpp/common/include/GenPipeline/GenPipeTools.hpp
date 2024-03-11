#pragma once
#ifndef _GENERAL_PIPELINE_TOOLS_HPP_
#define _GENERAL_PIPELINE_TOOLS_HPP_
#include <opencv2/opencv.hpp>
#include <type_traits>

// YOLO Image Detect Suite Definition

namespace GenPipTools {

	struct LetterInfo {
		int top_pad = 0;
		int left_pad = 0;
		float resize_scale = 1.f;
	};

	static inline cv::Mat letter_image(cv::Mat img, int hope_w, int hope_h, int& top_pad, int& left_pad, float& resize_scale, bool if_cvtColor = false)
	{
		top_pad = 0;
		left_pad = 0;
		resize_scale = 1.f;

		cv::Mat resize_img;
		int H = img.rows;
		int W = img.cols;

		// Skip resizing if hope_w or hope_h is invalid or if current dimensions match hope_h/w
		if (hope_w <= 0 || hope_h <= 0 || (H == hope_h && W == hope_w)) {
			resize_img = img;
			top_pad = 0;
			left_pad = 0;
		}
		else {
			float ratio_w = (float)W / (float)hope_w;
			float ratio_h = (float)H / (float)hope_h;
			if (ratio_w == ratio_h) {
				cv::resize(img, resize_img, cv::Size2i{ hope_w, hope_h });
				top_pad = 0;
				left_pad = 0;
			}
			else if (ratio_w > ratio_h) {
				int new_x = hope_w;
				int new_y = (int)(H / ratio_w);
				int pad1 = (int)((hope_h - new_y) / 2);
				int pad2 = hope_h - new_y - pad1;
				top_pad = pad1;
				left_pad = 0;
				resize_scale = ratio_w;
				cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
				cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
			}
			else {
				int new_y = hope_h;
				int new_x = (int)(W / ratio_h);
				int pad1 = (int)((hope_w - new_x) / 2);
				int pad2 = hope_w - new_x - pad1;
				top_pad = 0;
				left_pad = pad1;
				resize_scale = ratio_h;
				cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
				cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
			}
		}

		if (if_cvtColor)
			cv::cvtColor(resize_img, resize_img, cv::COLOR_BGR2RGB);

		return resize_img;
	}

	static inline cv::Mat letter_image(cv::Mat img, int hope_w, int hope_h, LetterInfo& letter_op, bool if_cvtColor = false) {
		return letter_image(img, hope_w, hope_h, letter_op.top_pad, letter_op.left_pad, letter_op.resize_scale, if_cvtColor);
	}

	static inline cv::Mat letter_image(cv::Mat img, int hope_w, int hope_h, bool if_cvtColor = false)
	{
		int pad_top_temp;
		int pad_lft_temp;
		float resize_scale;
		return letter_image(img, hope_w, hope_h, pad_top_temp, pad_lft_temp, resize_scale, if_cvtColor);
	}

	static inline cv::Mat safty_cut(cv::Mat& img, cv::Rect roi)
	{
		int width = roi.width;
		int height = roi.height;
		int x = roi.x;
		int y = roi.y;

		cv::Mat mat(height, width, img.type(), cv::Scalar(0));
		int _x = x;
		int _y = y;
		int _width = width;
		int _height = height;
		if (x < 0)
		{
			_x = 0;
			_width = width + x;
		}

		if (_x + _width > img.cols)
			_width = img.cols - _x;

		if (y < 0)
		{
			_y = 0;
			_height = height + y;
		}

		if (_y + _height > img.rows)
			_height = img.rows - _y;

		img(cv::Rect(_x, _y, _width, _height)).copyTo(mat(cv::Rect(_x - x, _y - y, _width, _height)));
		return mat;
	}


	class YoloBoxBase {
	public:
		float xmin;
		float ymin;
		float xmax;
		float ymax;
		float score;
		int cid;

	public:
		YoloBoxBase() = default;
		YoloBoxBase(const YoloBoxBase& other) = default;
		YoloBoxBase& operator=(const YoloBoxBase& other) = default;
		~YoloBoxBase() {};
		// constructor accept center_point + size(HW) trans to xyxy, so named YoloBoxBase
		YoloBoxBase(float cx, float cy, float w, float h, float the_score, int the_cid = -1) {
			xmin = cx - w / 2;
			xmax = cx + w / 2;
			ymin = cy - h / 2;
			ymax = cy + h / 2;
			score = the_score;
			cid = the_cid;
		}

		// constructor accept center_point + size(HW) trans to xyxy, so named YoloBoxBase
		YoloBoxBase(cv::Rect rect, float the_score, int the_cid = -1) {
			xmin = rect.x;
			xmax = rect.x + rect.width;
			ymin = rect.y;
			ymax = rect.y + rect.height;
			score = the_score;
			cid = the_cid;
		}

	public:
		/* ********************************* */
		/* Overloading the addition Operator */

		//virtual YoloBoxBase operator+(cv::Point p) const {
		//	YoloBoxBase result = *this;
		//	result.add(p.x, p.y);
		//	return result;
		//}

		//virtual YoloBoxBase& operator+=(cv::Point p) {
		//	add(p.x, p.y);
		//	return *this;
		//}

		virtual void add(cv::Point p) final {
			add(p.x, p.y);
		}

		virtual void add(int addToX, int addToY) final {
			xmin += addToX;
			ymin += addToY;
			xmax += addToX;
			ymax += addToY;
		}

		/* *************************************** */
		/* Overloading The Multiplication Operator */

		////return copy
		//virtual YoloBoxBase operator*(float ratio) const {
		//	YoloBoxBase result = *this;
		//	result.mul_ratio(ratio);
		//	return result;
		//}

		////return ref
		//virtual YoloBoxBase& operator*=(float ratio) {
		//	mul_ratio(ratio);
		//	return *this;
		//}

		virtual void mul_ratio(float ratio) final {
			xmin *= ratio;
			ymin *= ratio;
			xmax *= ratio;
			ymax *= ratio;
		}

	public:
		virtual bool overlaps(const YoloBoxBase& other) const final {
			return ((xmin <= other.xmax) && (xmax >= other.xmin)) && ((ymin <= other.ymax) && (ymax >= other.ymin));
		}

		virtual std::vector<cv::Point2f> points() const final {
			std::vector<cv::Point2f> rect_points{
				cv::Point2f(std::round(xmin),std::round(ymin)),
				cv::Point2f(std::round(xmin),std::round(ymax)),
				cv::Point2f(std::round(xmax),std::round(ymin)),
				cv::Point2f(std::round(xmax),std::round(ymax)) };
			return rect_points;
		}

		virtual cv::Rect get_rect() const final {
			return cv::Rect{
				cv::Point(std::round(xmin), std::round(ymin)),
				cv::Point(std::round(xmax), std::round(ymax)) };
		}

		virtual cv::Point2f get_center() const final {
			auto p1 = cv::Point2f(std::round(xmin), std::round(ymin));
			auto p2 = cv::Point2f(std::round(xmax), std::round(ymax));
			return (p1 + p2) / 2;
		}

		virtual float get_area() const final {
			return (xmax - xmin) * (ymax - ymin);
		}
	};

	// Map detected coords in processed letter_image to original coords.
	template<typename YoloBoxDerived>
	static inline void letter_map_origin_location(std::vector<YoloBoxDerived>& box_list, const LetterInfo& letter_op) {
		static_assert(std::is_base_of_v<YoloBoxBase, YoloBoxDerived>, "The element type must be derived from YoloBoxBase");
		for (auto& bbox : box_list) {
			bbox.add(-letter_op.left_pad, -letter_op.top_pad);
			bbox.mul_ratio(letter_op.resize_scale);
		}
	}

	template<typename YoloBoxDerived>
	static inline void nms_cpu(std::vector<YoloBoxDerived>& bboxes, float iou_thres) {
		static_assert(std::is_base_of_v<YoloBoxBase, YoloBoxDerived>, "The element type must be derived from YoloBoxBase");

		if (bboxes.empty()) return;
		std::sort(bboxes.begin(), bboxes.end(), [&](const YoloBoxDerived& b1, const YoloBoxDerived& b2) {return b1.score > b2.score; });
		std::vector<float> area(bboxes.size());
		for (int i = 0; i < bboxes.size(); ++i) {
			area[i] = (bboxes[i].xmax - bboxes[i].xmin + 1) * (bboxes[i].ymax - bboxes[i].ymin + 1);
		}
		for (int i = 0; i < bboxes.size(); ++i) {
			for (int j = i + 1; j < bboxes.size(); ) {
				float left = std::max(bboxes[i].xmin, bboxes[j].xmin);
				float right = std::min(bboxes[i].xmax, bboxes[j].xmax);
				float top = std::max(bboxes[i].ymin, bboxes[j].ymin);
				float bottom = std::min(bboxes[i].ymax, bboxes[j].ymax);
				float width = std::max(right - left + 1, 0.f);
				float height = std::max(bottom - top + 1, 0.f);
				float u_area = height * width;
				float iou = (u_area) / (area[i] + area[j] - u_area);
				if (iou >= iou_thres) {
					bboxes.erase(bboxes.begin() + j);
					area.erase(area.begin() + j);
				}
				else {
					++j;
				}
			}
		}
		if (bboxes.size() < 2) return;
		std::sort(bboxes.begin(), bboxes.end(), [&](const YoloBoxDerived& b1, const YoloBoxDerived& b2) {return b1.score > b2.score; });
	}


}

#endif //!_GENERAL_PIPELINE_TOOLS_HPP_