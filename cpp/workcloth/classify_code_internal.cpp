#include <iostream>
#include <cmath>
#include "general.hpp"

#include "classify_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "../posture/detect_code.hpp"

#include <opencv2/opencv.hpp>

#include <abi/param_vector.hpp>
#include <Primitives/fmt/format.h>
#include <utility>


#include <GenPipeline/GenPipeline.hpp>
#include <GenPipeline/GenPipeTools.hpp>

//YHC
//#include "dbg.h"

namespace glasssix::workcloth
{
	class classify_code_internal::impl
	{
	public:
		impl() noexcept {}
		impl(std::string_view model_directory, int device) :impl()
		{
			std::string model_directory_ = exposing::to_narrow_string(model_directory);
			if (*model_directory_.rbegin() != '/') model_directory_ += '/';

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			classify_instance_ = std::make_unique<GenPipeline>(model_directory_ + "workcloth_cls.rknn", 0);
#elif defined(USE_BMNN)
			classify_instance_ = std::make_unique<GenPipeline>(model_directory_ + "workcloth_cls.bmodel", 0);
#else
			classify_instance_ = std::make_unique<GenPipeline>(model_directory_ + "workcloth_cls.onnx", 0);
#endif
			constexpr float stand = 1.f / 255;
			classify_instance_->manual_possible_normalization({ 0,0,0 }, { stand,stand,stand });
		}

		exposing::param_vector<workcloth::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list_raw, std::map<std::string, float>& param_map, 
			std::unordered_map<int, std::vector<cv::Scalar>>& color_hsv_cfg)
		{
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}
			CHECK_EQ(channels, 3);
			CHECK_EQ(bitmap.size(), channels * height * width);
			if (roi_x < 0 || roi_x > width || roi_y > height || roi_y < 0 || roi_height < 0 || (roi_height + roi_y) >height || roi_width < 0 || (roi_width + roi_x) > width)
			{
				throw exposing::abi_invalid_argument("incorrect roi in phone");
			}

			cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
			cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));

			std::vector<box_info_internal> results;
			auto result = exposing::make_param_vector<box_info>();

			std::vector<PostureInfo> posture_info_list;

			for (auto pinfo : posture_info_list_raw) {
				//dbg(pinfo.score());

				PostureInfo postureInfo{ pinfo };
				posture_info_list.push_back(postureInfo);
			}

			body_nms_cpu(posture_info_list, 0.5);

			run_workcloth2(results, image, posture_info_list, param_map, color_hsv_cfg);

			for (auto& i : results)
			{
				result.push_back(exposing::make_as_first<box_info_impl>(i));
			}
			return result;
		}

		std::string version()
		{
			const std::string algo_module_version = "3.0.0";
			std::string nn_frame_version = classify_instance_->version();
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
		}

	private:

		static inline void body_nms_cpu(std::vector<PostureInfo>& bboxes, float iou_thres) {
			if (bboxes.empty()) return;
			std::sort(bboxes.begin(), bboxes.end(), [&](PostureInfo b1, PostureInfo b2) {return b1.score > b2.score; });
			std::vector<float> area(bboxes.size());
			for (int i = 0; i < bboxes.size(); ++i) {
				area[i] = bboxes[i].iou_body_nms.area();
			}
			for (int i = 0; i < bboxes.size(); ++i) {
				for (int j = i + 1; j < bboxes.size(); ) {
					float left = std::max(bboxes[i].x1, bboxes[j].x1);
					float right = std::min(bboxes[i].x2, bboxes[j].x2);
					float top = std::max(bboxes[i].y1, bboxes[j].y1);
					float bottom = std::min(bboxes[i].y2, bboxes[j].y2);
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
			std::sort(bboxes.begin(), bboxes.end(), [&](PostureInfo b1, PostureInfo b2) {return b1.score > b2.score; });
		}

		static inline bool rect_modify(cv::Rect& rec_region, int W, int H) {
			rec_region.x = std::max(0, rec_region.x);
			rec_region.y = std::max(0, rec_region.y);

			rec_region.width = std::min(W - rec_region.x - 1, rec_region.width);
			rec_region.height = std::min(H - rec_region.y - 1, rec_region.height);
			if (rec_region.width < 1 || rec_region.height < 1) {
				return true; // is invalid rect : true
			}
			else {
				return false;
			}
		}

		enum class Color {
			black = 0, grey, white, red, orange, yellow, green, cyan, blue, purple
		};

		static inline int calculate_singglehsv_method(cv::Mat image, Color mode, const std::vector<cv::Scalar>& color_hsv_cfg_val ) {
			cv::Mat color_mask;
			cv::Mat hsv_img;
			cv::cvtColor(image, hsv_img, cv::COLOR_BGR2HSV);
			if (mode != Color::red) {
				cv::Scalar hsv_lower = color_hsv_cfg_val[0];
				cv::Scalar hsv_upper = color_hsv_cfg_val[1];
				cv::inRange(hsv_img, hsv_lower, hsv_upper, color_mask);
			}
			else {
				// detect red mode
				cv::Mat mask1, mask2;
				cv::Scalar red_lower1 = color_hsv_cfg_val[0];
				cv::Scalar red_upper1 = color_hsv_cfg_val[1];
				cv::Scalar red_lower2 = color_hsv_cfg_val[2];
				cv::Scalar red_upper2 = color_hsv_cfg_val[3];
				cv::inRange(hsv_img, red_lower1, red_upper1, mask1);
				cv::inRange(hsv_img, red_lower2, red_upper2, mask2);
				color_mask = mask1 + mask2;
			}
			int color_pixels = cv::countNonZero(color_mask);
			return color_pixels;
		}

		void run_workcloth2(std::vector<box_info_internal>& results, cv::Mat& image, std::vector<PostureInfo>& persons, std::map<std::string, float>& param_map,
			std::unordered_map<int, std::vector<cv::Scalar>>& color_hsv_cfg)
		{
			float W = image.cols;
			float H = image.rows;

			int rc_img_side_minthres = 6;
			float points_score_thres = param_map.count("points_score_thres") ? param_map["points_score_thres"] : 0.95f;
			//float points_num_thres = param_map.count("points_num_thres") ? param_map["points_num_thres"] : 0.55f;

			// cv::Mat draw_image = image.clone();

			for (int pidx = 0; pidx < persons.size(); pidx++)
			{
				auto& person = persons[pidx];
				// dbg(person.score);

				box_info_internal in_box_info;
				std::vector<float> Kpoints_vali_set{ person.Kpoints_score[5],person.Kpoints_score[6],person.Kpoints_score[11],person.Kpoints_score[12],person.Kpoints_score[7],person.Kpoints_score[8] };
				// find no-rotate-react inclu upperbody_keys
				std::vector<cv::Point> upperbody_keys(person.Kpoints.begin() + 5, person.Kpoints.begin() + 13);
				auto ho = std::minmax_element(upperbody_keys.begin(), upperbody_keys.end(), [](cv::Point& p1, cv::Point& p2) {return p1.x > p2.x; });
				auto ve = std::minmax_element(upperbody_keys.begin(), upperbody_keys.end(), [](cv::Point& p1, cv::Point& p2) {return p1.y > p2.y; });
				auto upperbody_img_area = std::abs(ho.first - ho.second) * std::abs(ve.first - ve.second);

				static constexpr float upperbody_invalid_thres = 1.f / 6;
				bool upperbody_invalid = (upperbody_img_area * 1.f / person.person_bbox.area()) < upperbody_invalid_thres;

				int invalid_kpoints_counter = 0;

				for (auto p_score : Kpoints_vali_set) {
					if (p_score < points_score_thres) invalid_kpoints_counter++;
				}

				bool bodyishard = invalid_kpoints_counter > 2;

				// dbg(bodyishard);

				if (bodyishard || rect_modify(person.cls_cut, W, H) || rect_modify(person.color_cut, W, H)) continue; // bodyishard

				cv::Mat cls_image = GenPipeTools::safty_cut(image, person.cls_cut);
				cv::Mat blob = GenPipeTools::letter_image(cls_image, 112, 112, true);
				auto network_result = classify_instance_->forward(blob);
				auto tensor_out = network_result.begin()->second;

				auto classify_result = std::make_pair(tensor_out->cpu_data()[0], tensor_out->cpu_data()[1]);

				// Kpoints[5]: top right, [6]: top left, [12]: bottom right, [11]: bottom left
				cv::Point color_center = person.Kpoints[5] + person.Kpoints[6] + person.Kpoints[12] + person.Kpoints[11];
				color_center /= 4;
				cv::Point color_left = (color_center + person.Kpoints[6]) / 2;
				cv::Point color_right = (color_center + person.Kpoints[5]) / 2;

				cv::Rect rc_img_left(person.Kpoints[6], color_left);
				cv::Rect rc_img_center(
					color_center.x- person.color_cut.width/4, 
					color_center.y - person.color_cut.height / 4,
					person.color_cut.width / 2,
					person.color_cut.height / 2
				);
				cv::Rect rc_img_right(person.Kpoints[5], color_right);


				bool rc_img_area_over_min = 
					rc_img_center.width < rc_img_side_minthres ||
					rc_img_center.height < rc_img_side_minthres ||
					rc_img_left.width < rc_img_side_minthres ||
					rc_img_left.height < rc_img_side_minthres ||
					rc_img_right.width < rc_img_side_minthres ||
					rc_img_right.height < rc_img_side_minthres ||
					rc_img_center.width < rc_img_side_minthres ||
					rc_img_center.height < rc_img_side_minthres;

				if (rc_img_area_over_min) {
					//dbg(rc_img_area_over_min);
					//dbg(person.color_cut);
					//dbg(rc_img_center);
					//dbg(rc_img_left);
					//dbg(rc_img_right);
					continue;
				}

				cv::Mat color_img_center = GenPipeTools::safty_cut(image, rc_img_center);
				cv::Mat color_img_left = GenPipeTools::safty_cut(image, rc_img_left);
				cv::Mat color_img_right = GenPipeTools::safty_cut(image, rc_img_right);
				// Origin Main ROI CUT
				cv::Mat color_image = GenPipeTools::safty_cut(image, person.color_cut);

				auto color_ratios_abi = exposing::make_param_vector<float>();

				auto push_hsv_ratio = [&](cv::Mat region_img) {
					for (int i = 0; i < 10; i++) {
						int _color_pixels = calculate_singglehsv_method(region_img, static_cast<Color>(i),color_hsv_cfg[i]);
						int _total_pixels = region_img.cols * region_img.rows;
						float ratio = static_cast<float>(_color_pixels) / static_cast<float>(_total_pixels);
						color_ratios_abi.push_back(ratio);
					}
				};

				push_hsv_ratio(color_image);
				push_hsv_ratio(color_img_center);
				push_hsv_ratio(color_img_left);
				push_hsv_ratio(color_img_right);

				in_box_info.is_sleeve = classify_result.first < classify_result.second;
				in_box_info.color_ratios = color_ratios_abi;
				in_box_info.x1 = person.x1;
				in_box_info.y1 = person.y1;
				in_box_info.x2 = person.x2;
				in_box_info.y2 = person.y2;

				results.push_back(in_box_info);
			}
			// cv::imwrite("/home/firefly/yhc/bdh.png", draw_image);
		}


	private:
		std::unique_ptr<GenPipeline> classify_instance_;
	};

	classify_code_internal::classify_code_internal(std::string_view model_directory, int device)
		: impl_{ std::make_unique<impl>(model_directory, device) }
	{
	}

	classify_code_internal::~classify_code_internal() = default;

	std::string classify_code_internal::version()
	{
		return impl_->version();
	}

	exposing::param_vector<workcloth::box_info> classify_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map, std::unordered_map<int, std::vector<cv::Scalar>>& color_hsv_cfg) const
	{
		return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, posture_info_list, param_map, color_hsv_cfg);
	}
}