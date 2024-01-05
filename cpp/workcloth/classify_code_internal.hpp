#ifndef __CLASSIFY_CODE_INTERNAL_HPP__
#define __CLASSIFY_CODE_INTERNAL_HPP__

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
// #include "dbg.h"

namespace glasssix::workcloth
{
	struct PostureInfo
	{
		PostureInfo(posture::box_info& b_info) {
			//x1 = b_info.x1();
			//x2 = b_info.x2();
			//y1 = b_info.y1();
			//y2 = b_info.y2();
			score = b_info.score();
			category = b_info.category();

			auto key_points = b_info.key_points(); // 3 elems peer group : x, y, score
			for (size_t i = 0; i < (int)key_points.size() / 3; i++) {
				cv::Point key_p;
				key_p.x = key_points[i * 3];
				key_p.y = key_points[i * 3 + 1];
				Kpoints_score.push_back(key_points[i * 3 + 2]);
				Kpoints.push_back(key_p);
			}

			std::vector<cv::Point> cls_Kpoints{ Kpoints[5],Kpoints[6],Kpoints[7],Kpoints[8],Kpoints[9],Kpoints[10],Kpoints[11],Kpoints[12] };
			auto cls_rect = cv::minAreaRect(cls_Kpoints);
			cls_cut = cls_rect.boundingRect();
			int cls_H = cls_cut.height;
			int cls_W = cls_cut.width;
			cls_cut.y -= cls_H * 0.05;
			cls_cut.x -= cls_W * 0.15;
			cls_cut.height = cls_H * 1.05;
			cls_cut.width = cls_W * 1.3;

			auto myboundingRect = [](std::vector<cv::Point>& points_list) {
				cv::Point top_left{ 99999,99999 };
				cv::Point bottom_right{ 0,0 };
				for (auto p : points_list) {
					if (p.x < top_left.x)top_left.x = p.x;
					if (p.y < top_left.y)top_left.y = p.y;
					if (p.x > bottom_right.x)bottom_right.x = p.x;
					if (p.y > bottom_right.y)bottom_right.y = p.y;
				}
				return cv::Rect(top_left, bottom_right);
			};
			std::vector<cv::Point> color_Kpoints{ Kpoints[5],Kpoints[6],Kpoints[11],Kpoints[12] };
			color_cut = myboundingRect(color_Kpoints);

			cls_Kpoints.push_back(Kpoints[13]);
			cls_Kpoints.push_back(Kpoints[14]);
			iou_body_nms = myboundingRect(cls_Kpoints);

			x1 = iou_body_nms.x;
			y1 = iou_body_nms.y;
			x2 = iou_body_nms.x + iou_body_nms.width;
			y2 = iou_body_nms.y + iou_body_nms.height;
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
		std::vector<cv::Point> Kpoints;
		std::vector<float> Kpoints_score;
		cv::Rect cls_cut;
		cv::Rect color_cut;
		cv::Rect iou_body_nms;
	};

	struct box_info_internal
	{
		int x1;
		int y1;
		int x2;
		int y2;
		int is_sleeve;
		exposing::param_vector<float> color_ratios; //color ratio list
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

		exposing::param_vector<workcloth::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map, std::unordered_map<int, std::vector<cv::Scalar>>& color_hsv_cfg) const;

	private:
		std::unique_ptr<impl> impl_;
	};
}
#endif