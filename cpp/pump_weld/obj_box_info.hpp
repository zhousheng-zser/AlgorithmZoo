#pragma once
#ifndef _OBJBOXINFO_  
#define _OBJBOXINFO_

#include <opencv2/core.hpp>

namespace glasssix::pump_weld
{
	struct ObjBox {
		float xmin;
		float ymin;
		float xmax;
		float ymax;
		float score;
		int cid = 0;

		ObjBox(float cx, float cy, float w, float h, float the_score, bool the_cid) {
			xmin = cx - w / 2;
			xmax = cx + w / 2;
			ymin = cy - h / 2;
			ymax = cy + h / 2;
			score = the_score;
			cid = the_cid;
		}

		ObjBox(cv::Rect rect, float the_score, bool the_cid) {
			xmin = rect.x;
			xmax = rect.x + rect.width;
			ymin = rect.y;
			ymax = rect.y + rect.height;
			score = the_score;
			cid = the_cid;
		}

		bool overlaps(const ObjBox& other) const {
			return ((xmin <= other.xmax) && (xmax >= other.xmin)) && ((ymin <= other.ymax) && (ymax >= other.ymin));
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

		std::vector<cv::Point2f> points() const {
			std::vector<cv::Point2f> rect_points{
				cv::Point2f(std::round(xmin),std::round(ymin)),
				cv::Point2f(std::round(xmin),std::round(ymax)),
				cv::Point2f(std::round(xmax),std::round(ymin)),
				cv::Point2f(std::round(xmax),std::round(ymax)) };
			return rect_points;
		}

		cv::Rect get_rect() const {
			return cv::Rect{
				cv::Point(std::round(xmin), std::round(ymin)),
				cv::Point(std::round(xmax), std::round(ymax)) };
		}

		cv::Point2f get_center() const {
			auto p1 = cv::Point2f(std::round(xmin), std::round(ymin));
			auto p2 = cv::Point2f(std::round(xmax), std::round(ymax));
			return (p1 + p2) / 2;
		}

		float get_area() const {
			return (xmax - xmin) * (ymax - ymin);
		}
	};


	void obj_box_nms_cpu(std::vector<ObjBox>& bboxes, float iou_thres) {
		if (bboxes.empty()) return;
		std::sort(bboxes.begin(), bboxes.end(), [&](ObjBox b1, ObjBox b2) {return b1.score > b2.score; });
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
		std::sort(bboxes.begin(), bboxes.end(), [&](ObjBox b1, ObjBox b2) {return b1.score > b2.score; });
	}

}
#endif //!_OBJBOXINFO_
