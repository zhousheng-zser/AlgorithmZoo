#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>
#include <unordered_map>
#include <Primitives/tensor_conversions.hpp>
#ifdef USE_RKNNAPI
//#if 0
#include "../../common/include/RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#endif

#include "../head/detect_code.hpp"

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>

#include "Yolov8CutWrapper.hpp"


#ifdef BUILD_DEBUG_INFO
#include <opencv2/highgui/highgui.hpp>

#define GetShowRatio(visual_img) std::min(float(1920.f / visual_img.cols), float(1080.f / visual_img.rows)) * 0.75
#define ShowResize(visual_img, showRatio) cv::resize(visual_img, visual_img, cv::Size(), showRatio, showRatio);
#endif // BUILD_DEBUG_INFO


namespace glasssix::onphone
{
	class detect_code_internal::impl
	{
	public:
		impl(int device) noexcept : device_{ device } {}
		impl(std::string_view model_directory, int device)
			: impl(device)
		{
			model_directory_ = std::string(model_directory);

			phone_instance = std::make_unique<RknnYolov8Wrapper>(exposing::to_narrow_string(model_directory) + "/" + "onphone_v8_cut" + ".rknn", device);


			head_instance_ = glasssix::exposing::make_exported_interface<head::detect_code>(model_directory, device);
		}


		exposing::param_vector<onphone::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
		{
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}
			CHECK_EQ(channels, 3);
			CHECK_EQ(bitmap.size(), channels * height * width);

			cv::Mat image(cv::Size(width, height), CV_8UC3);
			std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * channels * height * width);

			if (roi_x<0 || roi_x>width || roi_y > height || roi_y < 0 || roi_height<0 || (roi_height + roi_y) >height || roi_width<0 || (roi_width + roi_x) > width)
			{
				throw exposing::abi_invalid_argument("incorrect roi in onphone");
			}


			float head_conf_thres = param_map.count("head_conf_thres") ? param_map["head_conf_thres"] : 0.3f;
			float head_nms_thres = param_map.count("head_nms_thres") ? param_map["head_nms_thres"] : 0.6f;
			auto head_param_abi = exposing::make_param_hash_map<exposing::param_string, float>();
			head_param_abi.add_or_update("conf_thres", head_conf_thres);
			head_param_abi.add_or_update("nms_thres", head_nms_thres);
			exposing::param_vector<head::box_info> head_info_list_raw = head_instance_.detect(bitmap, channels, height, width, 0, 0, width, height, head_param_abi);

			std::vector<ObjBox> head_list;
			for (auto hinfo : head_info_list_raw) {
				ObjBox headInfo;
				//dbg(pinfo.score());
				headInfo.xmin = hinfo.x1();
				headInfo.ymin = hinfo.y1();
				headInfo.xmax = hinfo.x2();
				headInfo.ymax = hinfo.y2();
				headInfo.score = hinfo.score();
				head_list.push_back(headInfo);
			}

			std::vector<box_info_internal> detect_results;
			cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width)).clone();

			for (auto& head : head_list) {
				//cv::rectangle(run_detect_visual, head.get_rect(), { 0,0,240 }, 5);
				//cv::rectangle(run_detect_visual, head.DetPhoneRegion(), { 0,240,0 }, 5);
				std::vector<ObjBox> phone_list = phone_detect(head, cropped_image, param_map);

				box_info_internal head_box_info;
				head_box_info.phonelocal_list = exposing::make_param_vector<std::int32_t>();
				head_box_info.phonescore_list = exposing::make_param_vector<float>();
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

		std::string version()
		{
			const std::string algo_module_version = "3.0.0";

			std::string nn_frame_version = phone_instance->version();

			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
		}

	private:
		std::vector<ObjBox> phone_detect(ObjBox& headbox, cv::Mat image, std::map<std::string,float>& param_map)
		{
			const float phone_vivid_conf_ratio = 1.25f; // 1.25 = 0.5 / 0.4
			float phone_conf_thres = param_map.count("phone_conf_thres") ? param_map["phone_conf_thres"] : 0.4f;
			float phone_nms_thres = param_map.count("phone_nms_thres") ? param_map["phone_nms_thres"] : 0.45f;
			float phone_distance_thres = param_map.count("phone_distance_thres") ? param_map["phone_distance_thres"] : 1.0f;

			std::vector<ObjBox> phone_list;

			int width = headbox.xmax - headbox.xmin;
			int height = headbox.ymax - headbox.ymin;

			int area = width * height;
			auto center = headbox.get_center();
			int cx = center.x;
			int cy = center.y;
			cv::Rect det_phone_region;

			if (area <= 500) {
				return phone_list;
			}
			else if (area > 500 && area < 8000) {
				det_phone_region = cv::Rect(cx - width * 0.75f, cy - height * 0.375f, width * 3, height * 1.5);
			}
			else {
				det_phone_region = cv::Rect(cx - width * 0.5f, cy - height * 0.375f, width * 2, height * 1.5);
				phone_conf_thres = std::min(0.9f, phone_conf_thres * phone_vivid_conf_ratio);
			}

			const int reShapeSide = 640;
			cv::Mat cls_img = safty_cut(image, det_phone_region);
			auto letter_img = imgPreProcess(cls_img, reShapeSide, reShapeSide);

			cv::cvtColor(letter_img, letter_img, cv::COLOR_BGR2RGB);

			auto det_rst_map = phone_instance->forward(letter_img);
			auto tensor_out = det_rst_map.begin()->second;

			int targetnum = tensor_out->height();
			int infonum = tensor_out->width();
			for (size_t idx = 0; idx < targetnum; idx++) {
				float* pdata = tensor_out->mutable_cpu_data() + idx * infonum;
				float conf = pdata[4];

				if (conf > phone_conf_thres)
				{
					ObjBox phonebox(pdata[0] * 640, pdata[1] * 640, pdata[2] * 640, pdata[3] * 640, conf);
					phone_list.push_back(phonebox);
				}
			}


			int pad = std::abs(cls_img.cols - cls_img.rows) / 2;
			bool is_vertical_pad = cls_img.cols > cls_img.rows;
			float mapping_ratio = static_cast<float>(std::max(cls_img.cols, cls_img.rows)) / reShapeSide;

			for (auto& phonebox : phone_list) {
				phonebox.mul_ratio(mapping_ratio);
				if (is_vertical_pad) {
					phonebox.ymin -= pad;
					phonebox.ymax -= pad;
				}
				else {
					phonebox.xmin -= pad;
					phonebox.xmax -= pad;
				}
			}

			nms_cpu(phone_list, phone_nms_thres);

			std::vector<ObjBox> phone_list_fliter_sort;
			for (auto& phonebox : phone_list) {
				// relocate phonebox mapping origin sdk input image

				auto start_position = det_phone_region.tl();
				phonebox.add(start_position.x, start_position.y);

				float iou_head_phone = get_iou(phonebox, headbox);
				float area_ratio = phonebox.get_area() / headbox.get_area();

				auto headboxRect = headbox.get_rect();
				float phone_head_distance = get_points_distance(headbox.get_center(), phonebox.get_center());
				float phone_head_relative_distance = (phone_head_distance * 2) / (headboxRect.width + headboxRect.height);

				////YHC
				//cv::rectangle(image, headbox.get_rect(), cv::Scalar{ 0,255,0 }, 3);
				//cv::rectangle(image, phonebox.get_rect(), cv::Scalar{ 0,0,255 }, 3);

				//dbg(iou_head_phone);
				//dbg(area_ratio);
				//dbg(phone_head_relative_distance);
				//dbg(iou_head_phone >= 0.025);
				//dbg(area_ratio > 0.1);
				//dbg(phone_head_relative_distance < phone_distance_thres);

				if (iou_head_phone >= 0.025 && area_ratio > 0.1 && phone_head_relative_distance < phone_distance_thres) {
					phone_list_fliter_sort.push_back(phonebox);
				}
			}
			std::sort(phone_list_fliter_sort.begin(), phone_list_fliter_sort.end(),
				[&](ObjBox b1, ObjBox b2)
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


		cv::Mat imgPreProcess(cv::Mat img, int hope_w = 640, int hope_h = 640)
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


		float get_points_distance(cv::Point2f pointO, cv::Point2f pointA)
		{
			float distance;
			distance = powf((pointO.x - pointA.x), 2) + powf((pointO.y - pointA.y), 2);
			distance = sqrtf(distance);
			return distance;
		}

		float get_iou(ObjBox& b1, ObjBox& b2) {
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

		void nms_cpu(std::vector<ObjBox>& bboxes, float iou_thres) {
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

		std::shared_ptr<glasssix::memory::tensor<float>> matConverTensor(cv::Mat input_image) {
			std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, input_image.rows, input_image.cols, 3}, -1, glasssix::memory::NHWC));
			std::copy(input_image.data, input_image.data + input_image.step[0] * input_image.rows, input_tensor_u8->mutable_cpu_data());
			input_tensor_u8->convert_order();
			auto input_tensor = input_tensor_u8 | glasssix::memory::tensor_convert_to<float>;
			return input_tensor;
		}

	private:
		std::string model_directory_;
		int device_;
		//std::unique_ptr<rknnwrapper::rknn_wrapper> detect_instance_;

		head::detect_code head_instance_;
		//std::unique_ptr<rknnwrapper::rknn_wrapper> phone_instance;
		std::unique_ptr<RknnYolov8Wrapper> phone_instance;
	};

	detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
		: impl_{ std::make_unique<impl>(model_directory, device) }
	{
	}

	detect_code_internal::~detect_code_internal() = default;

	exposing::param_vector<onphone::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
	{
		return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
	}

	std::string detect_code_internal::version()
	{
		return impl_->version();
	}
}
