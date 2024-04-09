#include <iostream>
#include <cmath>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"

#include "../head/detect_code.hpp"
#include "../posture/detect_code.hpp"

#include <opencv2/opencv.hpp>

#include <GenPipeline/GenPipeTools.hpp>
#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include "../genpipeline/market/yolov8_GEN.hpp"


namespace glasssix::onphone
{
	class detect_code_internal::impl
	{
	public:
		impl()noexcept {}

		impl(std::string_view model_directory, int device) :impl()
		{
			std::string model_dir = exposing::to_narrow_string(model_directory);
			if (*model_dir.rbegin() != '/') model_dir += '/';
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			iopipe_phone_det_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "onphone_v8_cut.rknn", 0);
#elif defined(USE_BMNN)
			iopipe_phone_det_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "onphone_v8_cut.bmodel", 0);
#else
			iopipe_phone_det_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "onphone_v8_cut.onnx", 0);
#endif
			iopipe_phone_det_->manual_possible_normalization(0, 1.f / 255);
			iopipe_phone_det_->set_postprocessing(yolov8_GEN<1, 1>);
		}

		exposing::param_vector<onphone::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height,
			exposing::param_vector<head::box_info> head_info_list_raw, std::map<std::string, float>& param_map)
		{
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}
			CHECK_EQ(channels, 3);
			CHECK_EQ(bitmap.size(), channels * height * width);
			cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));

			std::vector<HeadInfo> head_list;
			for (auto hinfo : head_info_list_raw) {
				head_list.push_back(hinfo);
			}

			std::vector<box_info_internal> detect_results;

			for (auto& head : head_list) {
				std::vector<PhoneBox> phone_list = phone_detect(head, image, param_map); // head loacte origin image before roi-cut in head_detect, shouldnt to roi_cut now.

				box_info_internal head_box_info;
				head_box_info.set(head);
				if (phone_list.empty()) {
					head_box_info.category = 0.f;
				}
				else {
					head_box_info.category = 1.f;
					for (auto& phoneBox : phone_list) {
						head_box_info.phonelocal_list.push_back(phoneBox.xmin);
						head_box_info.phonelocal_list.push_back(phoneBox.ymin);
						head_box_info.phonelocal_list.push_back(phoneBox.xmax);
						head_box_info.phonelocal_list.push_back(phoneBox.ymax);
						head_box_info.phonescore_list.push_back(phoneBox.score);
					}
				}

				detect_results.push_back(head_box_info);
			}

			auto result = exposing::make_param_vector<onphone::box_info>();
			for (auto& i : detect_results) {
				result.push_back(exposing::make_as_first<box_info_impl>(i));
			}
			return result;
		}

		exposing::param_vector<onphone::box_info> exdetect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height,
			exposing::param_vector<posture::box_info> posture_info_list_raw, std::map<std::string, float>& param_map)
		{
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}
			CHECK_EQ(channels, 3);
			CHECK_EQ(bitmap.size(), channels * height * width);
			cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));

			struct onPhoneMan
			{
				cv::Rect phoneDetRegion;
				cv::Rect manRegion;
				float score;
			};

			std::vector<onPhoneMan> man_list;
			// posture_info_list_raw 2 man_list
			for (auto pinfo : posture_info_list_raw) {
				std::pair<cv::Rect, float> man;
				auto xmin = pinfo.x1();
				auto ymin = pinfo.y1();
				auto det_w = pinfo.x2() - pinfo.x1();

				struct KpointsInfo {
					cv::Point pt;
					float score;
				};

				std::vector<KpointsInfo> Kpoints;
				auto key_points = pinfo.key_points(); // 3 elems peer group : x, y, score
				for (size_t i = 0; i < (int)key_points.size() / 3; i++) {
					cv::Point key_p(key_points[i * 3], key_points[i * 3 + 1]);
					Kpoints.push_back({ key_p, key_points[i * 3 + 2] });
				}
				int DetRegionBottom = std::max(Kpoints[9].pt.y, Kpoints[10].pt.y);
				int det_h = DetRegionBottom - pinfo.y1();
				int man_h = pinfo.y2() - pinfo.y1();

				cv::Rect manRegion(xmin, ymin, det_w, man_h);
				cv::Rect phoneDetRegion(xmin, ymin, det_w, det_h);

				float score = pinfo.score();
				man_list.push_back({ phoneDetRegion,manRegion, score });
			}

			std::vector<box_info_internal> detect_results;

			for (auto& man : man_list)
			{
				/* head loacte origin image before roi - cut in head_detect, shouldnt to roi_cut now. */
				std::vector<PhoneBox> phone_list = phone_detect_exhib(man.phoneDetRegion, image, param_map);

				box_info_internal rst_box_info;
				rst_box_info.phonelocal_list = exposing::make_param_vector<std::int32_t>();
				rst_box_info.phonescore_list = exposing::make_param_vector<float>();

				rst_box_info.x1 = man.phoneDetRegion.x;
				rst_box_info.y1 = man.phoneDetRegion.y;
				rst_box_info.x2 = man.phoneDetRegion.x + man.phoneDetRegion.width;
				rst_box_info.y2 = man.phoneDetRegion.y + man.phoneDetRegion.height;
				rst_box_info.confidence = man.score;

				if (phone_list.empty()) {
					rst_box_info.category = 0.f;
				}
				else {
					rst_box_info.category = 1.f;
					for (auto& phoneBox : phone_list) {
						rst_box_info.phonelocal_list.push_back(phoneBox.xmin);
						rst_box_info.phonelocal_list.push_back(phoneBox.ymin);
						rst_box_info.phonelocal_list.push_back(phoneBox.xmax);
						rst_box_info.phonelocal_list.push_back(phoneBox.ymax);
						rst_box_info.phonescore_list.push_back(phoneBox.score);
					}
				}

				detect_results.push_back(rst_box_info);
			}

			auto result = exposing::make_param_vector<onphone::box_info>();
			for (auto& i : detect_results) {
				result.push_back(exposing::make_as_first<box_info_impl>(i));
			}
			return result;
		}

		std::vector<PhoneBox> phone_detect(HeadInfo& headbox, cv::Mat image, std::map<std::string,float>& param_map)
		{
			float phone_conf_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.4f;
			float phone_nms_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.45f;
			float phone_distance_thres = param_map.count("phone_distance_thres") ? param_map["phone_distance_thres"] : 1.0f;

			std::vector<PhoneBox> phone_list;

			const int width = headbox.xmax - headbox.xmin;
			const int height = headbox.ymax - headbox.ymin;

			const int area = width * height;
			cv::Rect det_phone_region;

			if (area <= 500) {
				return phone_list;
			}
			else if (area > 500 && area < 8000) {
				det_phone_region = cv::Rect(headbox.xmin - width, headbox.ymin - height * 0.25f, width * 3, height * 1.5);
			}
			else {
				det_phone_region = cv::Rect(headbox.xmin - width * 0.5f, headbox.ymin - height * 0.25f, width * 2, height * 1.5);
			}

			cv::Mat phone_region_img = GenPipTools::safty_cut(image, det_phone_region);

			constexpr int letter_hw = 320;
			GenPipTools::LetterInfo letter_op;
			auto letter_img = GenPipTools::letter_image(phone_region_img, letter_hw, letter_hw, letter_op, true);
			auto det_rst_map = iopipe_phone_det_->forward(letter_img);
			auto tensor_out = det_rst_map.begin()->second;

			int targetnum = tensor_out->height();
			int infonum = tensor_out->width();
			for (size_t idx = 0; idx < targetnum; idx++) {
				float* pdata = tensor_out->mutable_cpu_data() + idx * infonum;
				float conf = pdata[4];
				if (conf > phone_conf_thres)
				{
					PhoneBox phonebox(pdata[0] * letter_hw, pdata[1] * letter_hw, pdata[2] * letter_hw, pdata[3] * letter_hw, conf);
					phone_list.push_back(phonebox);
				}
			}
			GenPipTools::nms_cpu(phone_list, phone_nms_thres);
			GenPipTools::letter_map_origin_location(phone_list, letter_op);

			std::vector<PhoneBox> phone_list_fliter_sort;
			for (auto& phonebox : phone_list) {
				// relocate phonebox mapping origin sdk input image
				auto start_position = det_phone_region.tl();
				phonebox.add(start_position.x, start_position.y);

				float iou_head_phone = get_iou(phonebox, headbox);
				float area_ratio = phonebox.get_area() / headbox.get_area();

				auto headboxRect = headbox.get_rect();
				float phone_head_distance = get_points_distance(headbox.get_center(), phonebox.get_center());
				float phone_head_relative_distance = (phone_head_distance * 2) / (headboxRect.width + headboxRect.height);

				if (iou_head_phone >= 0.025 && area_ratio > 0.08 && phone_head_relative_distance < phone_distance_thres) {
					phone_list_fliter_sort.push_back(phonebox);
				}
			}
			std::sort(phone_list_fliter_sort.begin(), phone_list_fliter_sort.end(),
				[&](PhoneBox b1, PhoneBox b2)
				{
					auto head_center = headbox.get_center();
					auto b1_center = b1.get_center();
					auto b2_center = b2.get_center();
					float b1_head_distance = get_points_distance(head_center, b1_center);
					float b2_head_distance = get_points_distance(head_center, b2_center);
					return b1_head_distance < b2_head_distance;
				}
			);

			//YHC
			//cv::imwrite("/home/glasssix/yhc/onphone.jpg", image);

			return phone_list_fliter_sort;
		}

		std::vector<PhoneBox> phone_detect_exhib(cv::Rect det_region, cv::Mat image, std::map<std::string, float>& param_map) {
			float phone_conf_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.4f;
			float phone_nms_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.45f;
			std::vector<PhoneBox> phone_list;

			cv::Mat phone_region_img = GenPipTools::safty_cut(image, det_region);
			constexpr int letter_hw = 320;
			GenPipTools::LetterInfo letter_op;
			auto letter_img = GenPipTools::letter_image(phone_region_img, letter_hw, letter_hw, letter_op, true);
			auto det_rst_map = iopipe_phone_det_->forward(letter_img);
			auto tensor_out = det_rst_map.begin()->second;

			int targetnum = tensor_out->height();
			int infonum = tensor_out->width();
			for (size_t idx = 0; idx < targetnum; idx++) {
				float* pdata = tensor_out->mutable_cpu_data() + idx * infonum;
				float conf = pdata[4];
				if (conf > phone_conf_thres)
				{
					PhoneBox phonebox(pdata[0] * letter_hw, pdata[1] * letter_hw, pdata[2] * letter_hw, pdata[3] * letter_hw, conf);
					phone_list.push_back(phonebox);
				}
			}
			GenPipTools::nms_cpu(phone_list, phone_nms_thres);
			GenPipTools::letter_map_origin_location(phone_list, letter_op);

			for (auto& ph : phone_list) {
				ph.add(det_region.tl()); //mapping start
			}
			return phone_list;
		}

		float get_points_distance(cv::Point2f pointO, cv::Point2f pointA)
		{
			float distance;
			distance = powf((pointO.x - pointA.x), 2) + powf((pointO.y - pointA.y), 2);
			distance = sqrtf(distance);
			return distance;
		}

		float get_iou(PhoneBox& b1, HeadInfo& b2) {
			float left = std::max(b1.xmin, b2.xmin);
			float right = std::min(b1.xmax, b2.xmax);
			float top = std::max(b1.ymin, b2.ymin);
			float bottom = std::min(b1.ymax, b2.ymax);

			float width = std::max(right - left + 1, 0.f);
			float height = std::max(bottom - top + 1, 0.f);
			float u_area = height * width;
			float iou = (u_area) / (b1.get_area() + b2.get_area() - u_area);
			return iou;
		}

		std::string version()
		{
			const std::string algo_module_version = "4.0.0";
			std::string nn_frame_version = iopipe_phone_det_->version();
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
		}

	private:
		std::shared_ptr<PrePostProcessGenPipeline> iopipe_phone_det_;
	};

	detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
		: impl_{ std::make_unique<impl>(model_directory, device) }
	{
	}

	detect_code_internal::~detect_code_internal() = default;

	exposing::param_vector<onphone::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<head::box_info> head_info_list, std::map<std::string, float>& param_map) const
	{
		return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, head_info_list, param_map);
	}

	exposing::param_vector<onphone::box_info> detect_code_internal::exdetect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map) const
	{
		return impl_->exdetect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, posture_info_list, param_map);
	}

	std::string detect_code_internal::version()
	{
		return impl_->version();
	}
}
