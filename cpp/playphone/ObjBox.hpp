#pragma once
#include<vector>
#include<opencv2/core.hpp>
#include "../posture/box_info.hpp"
//#include "dbg.h"
#include <GenPipeline/GenPipeTools.hpp>

namespace glasssix::playphone
{
	struct PhoneBox :public GenPipTools::YoloBoxBase {
	public:
		using YoloBoxBase::YoloBoxBase; //Inheriting Constructors
	};

	struct PostureInfo :public GenPipTools::YoloBoxBase {
	public:
		using YoloBoxBase::YoloBoxBase; //Inheriting Constructors

		std::vector<cv::Point> Kpoints;
		std::vector<float> Kpoints_score;
		cv::Rect origin_image_border;

		static constexpr float vaild_face_thres = 0.1f;
		static constexpr float vaild_hand_thres = 0.1f;

		PostureInfo(posture::box_info& b_info) {
			xmin = b_info.x1();
			xmax = b_info.x2();
			ymin = b_info.y1();
			ymax = b_info.y2();
			score = b_info.score();
			cid = b_info.category();

			auto key_points = b_info.key_points(); // 3 elems peer group : x, y, score
			for (size_t i = 0; i < (int)key_points.size() / 3; i++) {
				cv::Point key_p;
				key_p.x = key_points[i * 3];
				key_p.y = key_points[i * 3 + 1];
				Kpoints_score.push_back(key_points[i * 3 + 2]);
				Kpoints.push_back(key_p);
			}

		}

		bool if_hand_close_nose(const float& hand_nose_thresh) {
			cv::Point nose_pt = Kpoints[0];
			const cv::Point& rgt_wrist_pt = Kpoints[9];
			const cv::Point& lft_wrist_pt = Kpoints[10];
			auto dis_nose_rgtwrist = cv::norm(nose_pt - rgt_wrist_pt);
			auto dis_nose_lftwrist = cv::norm(nose_pt - lft_wrist_pt);
			auto hand_nose_dis = std::min(dis_nose_rgtwrist, dis_nose_lftwrist);
			return hand_nose_dis <= hand_nose_thresh;
		}

		void set_origin_image_border(int x, int y, int width, int height) {
			origin_image_border = cv::Rect(x, y, width, height);
		}

		cv::Rect get_playphone_det_region() {
			int people_width = xmax - xmin;
			int people_height = ymax - ymin;

			constexpr float expand_people_ratio = 0.15f;
			int left = xmin - people_width * expand_people_ratio;
			int right = xmax + people_width * expand_people_ratio;
			int top = ymin - people_height * expand_people_ratio;
			//int bottom = ymax + people_height * expand_people_ratio;

			int bottom = std::max({ Kpoints[11].y, Kpoints[12].y ,Kpoints[13].y, Kpoints[14].y });
			cv::Rect uprexpand{
				cv::Point(std::round(left), std::round(top)),
				cv::Point(std::round(right), std::round(bottom)) };

			cv::Rect intersect = uprexpand & origin_image_border; //border limit
			return intersect;
		}

		std::vector<cv::Rect> get_playphone_hands_region() {
			auto upperbody_img_rect = get_playphone_det_region();
			int handbox_len = std::max(upperbody_img_rect.width, upperbody_img_rect.height) * 0.1f;

			auto centerRect = [](cv::Point center, int W, int H) {
				cv::Point offset(W / 2, H / 2);
				return cv::Rect(center - offset, center + offset);
			};

			auto hand_point = [](cv::Point elbow, cv::Point wrist, float extend_ratio = 0.6) {
				int hand_x = wrist.x + (wrist.x - elbow.x) * extend_ratio;
				int hand_y = wrist.y + (wrist.y - elbow.y) * extend_ratio;
				return cv::Point{ hand_x, hand_y };
			};

			std::vector<cv::Rect> hands_region;
			if (Kpoints_score[9] >= vaild_hand_thres) {
				cv::Rect hand_rgt = centerRect(hand_point(Kpoints[7], Kpoints[9]), handbox_len, handbox_len);
				hands_region.push_back(hand_rgt);
			}
			if (Kpoints_score[10] >= vaild_hand_thres) {
				cv::Rect hand_lft = centerRect(hand_point(Kpoints[8], Kpoints[10]), handbox_len, handbox_len);
				hands_region.push_back(hand_lft);
			}
			return hands_region;
		}

		size_t invaild_face_kpnum() {
			size_t invaild_num = 0;
			for (int i = 0; i < 3; i++) {
				if (Kpoints_score[i] < vaild_face_thres) invaild_num++;
			}
			return invaild_num;
		}

		size_t invaild_hand_kpnum() {
			size_t invaild_num = 0;
			if (Kpoints_score[9] < vaild_hand_thres) invaild_num++;
			if (Kpoints_score[10] < vaild_hand_thres) invaild_num++;
			return invaild_num;
		}


	};
}
