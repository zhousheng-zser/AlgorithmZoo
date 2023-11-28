#include <iostream>
#include <cmath>
#include "hardcode.hpp"
#include "general.hpp"

#include "classify_code_internal.hpp"
#include "box_info_impl.hpp"
#include <Excalibur/pipeline.hpp>
#include <Primitives/tensor_conversions.hpp>
#include "logger.hpp"

#include "../posture/detect_code.hpp"

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#ifdef BUILD_DEBUG_INFO
#include <opencv2/highgui/highgui.hpp>
#endif // BUILD_DEBUG_INFO


#include <abi/param_vector.hpp>
#include <Primitives/fmt/format.h>
#include <utility>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif


#ifdef USE_RKNNAPI
//#if 0
#include "../../common/include/RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#endif

//YHC
//#include "dbg.h"

namespace glasssix::workcloth
{
	class classify_code_internal::impl
	{
	public:
		impl(int device) noexcept : device_{ device } {}
		impl(std::string_view model_directory, int device)
			: impl(device)
		{
			model_directory_ = std::string(model_directory);
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			classify_instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("workcloth_cls"), std::string(model_directory) + "/" + "workcloth_cls" + ".rknn", device);
#endif
			posture_instance_ = glasssix::exposing::make_exported_interface<posture::detect_code>(model_directory, device,1);
			posture_param_abi = exposing::make_param_hash_map<exposing::param_string, float>();
			posture_param_abi.add_or_update("conf_thres", 0.70f);
			posture_param_abi.add_or_update("nms_thres", 0.70f);
		}

		exposing::param_vector<workcloth::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
		{
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}
			CHECK_EQ(channels, 3);
			CHECK_EQ(bitmap.size(), channels * height * width);

			cv::Mat image2(height, width, CV_8UC3);
			cv::Mat image(cv::Size(width, height), CV_8UC3);
			std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * channels * height * width);
			if (roi_x < 0 || roi_x > width || roi_y > height || roi_y < 0 || roi_height < 0 || (roi_height + roi_y) >height || roi_width < 0 || (roi_width + roi_x) > width)
			{
				throw exposing::abi_invalid_argument("incorrect roi in phone");
			}

			cv::Rect roi_rect{ roi_x, roi_y, roi_width, roi_height };

			std::vector<box_info_internal> results;
			auto result = exposing::make_param_vector<box_info>();

			exposing::param_vector<posture::box_info> posture_info_list_raw = posture_instance_.detect(bitmap, channels, height, width, 0, 0, width, height, posture_param_abi);
			std::vector<PostureInfo> posture_info_list;
			// std::vector<PostureInfo> persons_info;

			//dbg(posture_info_list_raw.size());
			for (auto pinfo : posture_info_list_raw) {
				//dbg(pinfo.score());

				PostureInfo postureInfo{ pinfo };
				posture_info_list.push_back(postureInfo);
			}

			body_nms_cpu(posture_info_list, 0.5);

			//run_workcloth(results, image, persons_info, param_map);
			run_workcloth2(results, image, posture_info_list, param_map);


			for (auto& i : results)
			{
				result.push_back(exposing::make_as_first<box_info_impl>(i));
			}
			return result;
		}

		std::string version()
		{
			const std::string algo_module_version = "2.7.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			//#if 0
			std::string nn_frame_version = classify_instance_->version();
#else
			std::string nn_frame_version = excalibur::pipeline<float>::version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
		}

	private:

		void body_nms_cpu(std::vector<PostureInfo>& bboxes, float iou_thres) {
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

		inline cv::Mat safty_cut(cv::Mat& img, cv::Rect roi)
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

		bool rect_modify(cv::Rect& rec_region, int W, int H) {
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

		cv::Mat preprocess(cv::Mat img, int hope_w = 640, int hope_h = 640)
		{
			int H = img.rows;
			int W = img.cols;
			float ratio_w = (float)W / (float)hope_w;
			float ratio_h = (float)H / (float)hope_h;
			cv::Mat resize_img;
			if (ratio_w == ratio_h)
				cv::resize(img, resize_img, cv::Size2i{ hope_w, hope_h });
			else if (ratio_w > ratio_h) {
				int new_x = hope_w;
				int new_y = (int)(H / ratio_w);
				int pad1 = (int)((hope_h - new_y) / 2);
				int pad2 = hope_h - new_y - pad1;
				cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
				cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
			}
			else {
				int new_y = hope_h;
				int new_x = (int)(W / ratio_h);
				int pad1 = (int)((hope_w - new_x) / 2);
				int pad2 = hope_w - new_x - pad1;
				cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
				cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 127,127,127 });
			}
			return resize_img;
		}


		std::pair<float, float> run_classify(cv::Mat& image)
		{
			cv::Mat blob = preprocess(image, 112, 112);
			cv::cvtColor(blob, blob, cv::COLOR_BGR2RGB);

			std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forwards;
			auto network_result = classify_instance_->forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);
			for (auto& out : network_result) {
				forwards.push_back(out.second);
			}

			const float* data_ptr = forwards[0]->cpu_data();
			return std::make_pair(data_ptr[0], data_ptr[1]);
		}

		enum class Color {
			black = 0, grey, white, red, orange, yellow, green, cyan, blue, purple
		};

		std::map<Color, std::pair<cv::Scalar, cv::Scalar>> color_hsv_cfg{
		{Color::black,{cv::Scalar{0, 0, 0},cv::Scalar{180, 255, 45}}},
		{Color::grey,{cv::Scalar{0, 0, 46},cv::Scalar{180, 42, 220}}},
		{Color::white,{cv::Scalar{0, 0, 221},cv::Scalar{180, 30, 255}}},

		{Color::orange,{cv::Scalar{11, 43, 46},cv::Scalar{25, 255, 255}}},
		{Color::yellow,{cv::Scalar{26, 43, 46},cv::Scalar{34, 255, 255}}},

		{Color::green,{cv::Scalar{35, 43, 46},cv::Scalar{77, 255, 255}}},
		{Color::cyan,{cv::Scalar{78, 43, 46},cv::Scalar{99, 255, 255}}},
		{Color::blue,{cv::Scalar{100, 43, 46},cv::Scalar{124, 255, 255}}},
		{Color::purple,{cv::Scalar{125, 43, 46},cv::Scalar{155, 255, 255}}},
		};

		int calculate_singglehsv_method(cv::Mat image, Color mode) {
			cv::Mat color_mask;
			cv::Mat hsv_img;
			cv::cvtColor(image, hsv_img, cv::COLOR_BGR2HSV);
			if (mode != Color::red) {
				cv::Scalar hsv_lower = color_hsv_cfg.at(mode).first;
				cv::Scalar hsv_upper = color_hsv_cfg.at(mode).second;

				cv::inRange(hsv_img, hsv_lower, hsv_upper, color_mask);
			}
			else {
				// detect red mode
				cv::Mat mask1;
				cv::Scalar red_lower1 = cv::Scalar{ 0, 43, 46 };
				cv::Scalar red_upper1 = cv::Scalar{ 10, 255, 255 };
				cv::inRange(hsv_img, red_lower1, red_upper1, mask1);

				cv::Mat mask2;
				cv::Scalar red_lower2 = cv::Scalar{ 156, 43, 46 };
				cv::Scalar red_upper2 = cv::Scalar{ 180, 255, 255 };
				cv::inRange(hsv_img, red_lower2, red_upper2, mask2);
				color_mask = mask1 + mask2;
			}

			int color_pixels = cv::countNonZero(color_mask);
			return color_pixels;
		}

		void run_workcloth(std::vector<box_info_internal>& results, cv::Mat& image, std::vector<PostureInfo>& persons, std::map<std::string, float>& param_map)
		{
			float W = image.cols;
			float H = image.rows;

			float points_score_thres = param_map.count("points_score_thres") ? param_map["points_score_thres"] : 0.9f;
			float points_num_thres = param_map.count("points_num_thres") ? param_map["points_num_thres"] : 0.45f;

			for (auto& person : persons)
			{
				// auto& person=persons[2];
				box_info_internal in_box_info;

				// dbg(person.Kpoints_score);
				// dbg(person.Kpoints_score[5]);
				// dbg(person.Kpoints_score[6]);
				// dbg(person.Kpoints_score[11]);
				// dbg(person.Kpoints_score[12]);

				std::vector<float> Kpoints_vali_set{ person.Kpoints_score[5],person.Kpoints_score[6],person.Kpoints_score[11],person.Kpoints_score[12] };
				int effect_kpoints_counter = 0;

				for (auto p_score : Kpoints_vali_set) {
					if (p_score > points_score_thres) effect_kpoints_counter++;
				}

				bool bodyishard = ((float)effect_kpoints_counter / Kpoints_vali_set.size()) < points_num_thres;
				// dbg(bodyishard);
				if (bodyishard || rect_modify(person.cls_cut, W, H) || rect_modify(person.color_cut, W, H)) continue; // bodyishard

				cv::Mat cls_image = safty_cut(image, person.cls_cut);
				auto classify_result = run_classify(cls_image);

				person.color_cut.width *= 0.5;
				person.color_cut.height *= 0.5;
				person.color_cut.x += person.color_cut.width * 0.5;
				person.color_cut.y += person.color_cut.height * 0.5;

				cv::Mat color_image = safty_cut(image, person.color_cut);
				int total_pixels = color_image.rows * color_image.cols;
				auto color_ratios_abi = exposing::make_param_vector<float>();
				for (int i = 0; i < 10; i++) {
					float ratio = (float)calculate_singglehsv_method(color_image, static_cast<Color>(i)) / total_pixels;
					color_ratios_abi.push_back(ratio);
				}

				// for (auto kp : person.Kpoints) {
				//     cv::circle(image, kp, 3, { 255,255,255}, 3);
				// }
				// cv::circle(image, person.Kpoints[5], 3, { 0,0,150}, 3);
				// cv::circle(image, person.Kpoints[6], 3, { 0,0,180}, 3);
				// cv::circle(image, person.Kpoints[11], 3, { 0,0,210}, 3);
				// cv::circle(image, person.Kpoints[12], 3, { 0,0,250}, 3);

				in_box_info.is_sleeve = classify_result.first < classify_result.second;
				in_box_info.color_ratios = color_ratios_abi;
				in_box_info.x1 = person.x1;
				in_box_info.y1 = person.y1;
				in_box_info.x2 = person.x2;
				in_box_info.y2 = person.y2;

				results.push_back(in_box_info);
			}
				// cv::imwrite("/home/firefly/yhc/bdh.png",image);
		}

		void run_workcloth2(std::vector<box_info_internal>& results, cv::Mat& image, std::vector<PostureInfo>& persons, std::map<std::string, float>& param_map)
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
				// dbg(Kpoints_vali_set);
				// dbg(person.color_cut.tl());

				int invalid_kpoints_counter = 0;

				for (auto p_score : Kpoints_vali_set) {
					if (p_score < points_score_thres) invalid_kpoints_counter++;
				}

				bool bodyishard = invalid_kpoints_counter > 2;

				// dbg(bodyishard);

				if (bodyishard || rect_modify(person.cls_cut, W, H) || rect_modify(person.color_cut, W, H)) continue; // bodyishard

				cv::Mat cls_image = safty_cut(image, person.cls_cut);
				auto classify_result = run_classify(cls_image);

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

				cv::Mat color_img_center = safty_cut(image, rc_img_center);
				cv::Mat color_img_left = safty_cut(image, rc_img_left);
				cv::Mat color_img_right = safty_cut(image, rc_img_right);
				// Origin Main ROI CUT
				cv::Mat color_image = safty_cut(image, person.color_cut);

				auto color_ratios_abi = exposing::make_param_vector<float>();

				auto push_hsv_ratio = [&](cv::Mat region_img) {
					for (int i = 0; i < 10; i++) {
						int _color_pixels = calculate_singglehsv_method(region_img, static_cast<Color>(i));
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

				// cv::rectangle(draw_image, rc_img_center, { 0,170,0 }, 2);
				// cv::rectangle(draw_image, rc_img_left, { 170,170,0 }, 2);
				// cv::rectangle(draw_image, rc_img_right, { 0,170,170 }, 2);
				// cv::rectangle(draw_image, person.color_cut, { 0,0,0 }, 2);
				// for (auto kp : person.Kpoints) {
				// 	cv::circle(draw_image, kp, 2, { 255,255,255 }, 2);
				// }
				// cv::circle(draw_image, person.Kpoints[5], 3, { 0,0,150}, 3);
				// cv::circle(draw_image, person.Kpoints[6], 3, { 0,0,180}, 3);
				// cv::circle(draw_image, person.Kpoints[11], 3, { 0,0,210}, 3);
				// cv::circle(draw_image, person.Kpoints[12], 3, { 0,0,250}, 3);
				// cv::circle(draw_image, person.Kpoints[7], 2, { 0,255,0}, 2);
				// cv::circle(draw_image, person.Kpoints[8], 2, { 0,255,0}, 2);

			}
			// cv::imwrite("/home/firefly/yhc/bdh.png", draw_image);
		}


	private:
		exposing::param_hash_map<exposing::param_string, float> posture_param_abi;
		std::string model_directory_;
		int device_;

		posture::detect_code posture_instance_;

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
		//#if 0
		std::unique_ptr<rknnwrapper::rknn_wrapper> classify_instance_;
#else
		std::unique_ptr<excalibur::pipeline<float>> classify_instance_;
#endif
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

	exposing::param_vector<workcloth::box_info> classify_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
	{
		return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
	}
}