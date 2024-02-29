#pragma once
#ifndef _OBJBOXINFO_  
#define _OBJBOXINFO_

#include <opencv2/core.hpp>
#include "../posture/box_info.hpp"

namespace glasssix::pump_vesthelmet
{
	struct PostureInfo
	{
		std::int32_t xmin;
		std::int32_t ymin;
		std::int32_t xmax;
		std::int32_t ymax;
		float score;
		int category;
		std::vector<cv::Point> Kpoints;
		std::vector<float> Kpoints_score;
		//cv::Rect color_cut;

		PostureInfo(posture::box_info& b_info) {
			xmin = b_info.x1();
			xmax = b_info.x2();
			ymin = b_info.y1();
			ymax = b_info.y2();
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
		}

		cv::Rect get_vest_det_region() {
			std::vector<cv::Point> Kpoints_temp{
				Kpoints[0],
				Kpoints[5],
				Kpoints[6],
				Kpoints[7],
				Kpoints[8],
				Kpoints[11],
				Kpoints[12],
			};

			auto minmax_y = std::minmax_element(Kpoints_temp.begin(), Kpoints_temp.end(), [](cv::Point& a, cv::Point& b) {
				return a.y < b.y; });
			int top = minmax_y.first->y;
			int bottom = minmax_y.second->y;

			auto minmax_x = std::minmax_element(Kpoints_temp.begin(), Kpoints_temp.end(), [](cv::Point& a, cv::Point& b) {
				return a.x < b.x; });
			int left = minmax_x.first->x;
			int right = minmax_x.second->x;

			return cv::Rect{
				cv::Point(std::round(left), std::round(top)),
				cv::Point(std::round(right), std::round(bottom)) };
		}

		cv::Rect get_rect() {
			return cv::Rect{
				cv::Point(std::round(xmin), std::round(ymin)),
				cv::Point(std::round(xmax), std::round(ymax)) };
		}

		bool if_pump_vesthelmet_bodyerr() {
			CHECK_EQ(Kpoints_score.size(), Kpoints.size());
			int err_counter = 0;
			std::array<int, 4> check_idxs{ 5,6,11,12 };
			for (auto idx : check_idxs) {
				if (Kpoints_score[idx] < 0.8) {
					err_counter++;
				}
			}

			auto upperbody_img_area = get_vest_det_region().area();
			auto people_area = get_rect().area();
			return err_counter > 1 || (upperbody_img_area * 1.f) < (people_area * 1.f / 6);
		}

	};

	struct HeadInfo {
		float xmin;
		float ymin;
		float xmax;
		float ymax;
		float score;
		int cid = 0;

		HeadInfo(float cx, float cy, float w, float h, float the_score) {
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



	void headinfo_nms_cpu(std::vector<HeadInfo>& bboxes, float iou_thres) {
		if (bboxes.empty()) return;
		std::sort(bboxes.begin(), bboxes.end(), [&](HeadInfo b1, HeadInfo b2) {return b1.score > b2.score; });
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
		std::sort(bboxes.begin(), bboxes.end(), [&](HeadInfo b1, HeadInfo b2) {return b1.score > b2.score; });
	}

}
#endif //!_OBJBOXINFO_
