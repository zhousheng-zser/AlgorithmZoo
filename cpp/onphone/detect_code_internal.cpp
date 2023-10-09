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
#else
#include "onxrt.hpp"
#endif

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>

#include "dbg.h"
#include "numpyExtensor.hpp"

#ifdef BUILD_DEBUG_INFO
#include <opencv2/highgui/highgui.hpp>
#include "dbg.h"
#include "numpyExtensor.hpp"

#define GetShowRatio(visual_img) std::min(float(1920.f / visual_img.cols), float(1080.f / visual_img.rows)) * 0.75
#define ShowResize(visual_img, showRatio) cv::resize(visual_img, visual_img, cv::Size(), showRatio, showRatio);
#endif // BUILD_DEBUG_INFO


namespace glasssix::onphone
{
	class detect_code_internal::impl
	{
	public:
		impl(const exposing::param_string model_directory, int device = -1)
			: device_(device)
		{
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			detect_instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("onphone"), std::string(model_directory) + "/" + "onphone_det" + ".rknn", device);
			classi_instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("onphone"), std::string(model_directory) + "/" + "onphone_cls" + ".rknn", device);
#else
			detect_instance_ = std::make_unique<onnxrt::pipline>(exposing::to_narrow_string(model_directory) + "/" + "head_yolov8s_best.onnx");
			classi_instance_ = std::make_unique<onnxrt::pipline>(exposing::to_narrow_string(model_directory) + "/" + "efficientnet.onnx");
#endif
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

			cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width)).clone();

			std::vector<box_info_internal> detect_results;
			run_detect(detect_results, cropped_image, roi_x, roi_y, roi_width, roi_height, param_map);

			auto result = exposing::make_param_vector<onphone::box_info>();
			for (auto& i : detect_results) {
				result.push_back(exposing::make_as_first<box_info_impl>(i));
			}
			return result;
		}

		std::string version()
		{
			const std::string algo_module_version = "2.0.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			//#if 0
			std::string nn_frame_version = detect_instance_->version();
#else
			std::string nn_frame_version = detect_instance_->version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
		}

	private:
		std::vector<HeadBox> head_detect(cv::Mat image) {
			std::vector<HeadBox> head_list;

			int reShapeSide = 640;
			auto letter_img = imgPreProcess(image, reShapeSide, reShapeSide);
			float mapping_ratio = static_cast<float>(std::max(image.cols, image.rows)) / reShapeSide;

			cv::cvtColor(letter_img, letter_img, cv::COLOR_BGR2RGB);

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			auto det_rst_map = detect_instance_->forward(letter_img.data, { 1, letter_img.rows, letter_img.cols, letter_img.channels() }, RKNN_TENSOR_NHWC);
			auto det_rst = det_rst_map.begin()->second;
			auto FlameRknnOutN = det_rst->num();
			det_rst->reshape(std::vector<int>{FlameRknnOutN, 1, 5, 8400});
#else
			auto input_tensor = matConverTensor(letter_img);
			// normalization
			auto data = input_tensor->mutable_cpu_data();
			for (int i = 0; i < input_tensor->count(); i++) {
				data[i] = data[i] / 255;
			}
			auto det_rst_map = detect_instance_->forward(input_tensor);
			auto det_rst = det_rst_map.begin()->second;
#endif
			// transepose
			auto tensor_out = tensor_transpose_0132(det_rst);

			int targetnum = tensor_out->height();
			int infonum = tensor_out->width();
			for (size_t idx = 0; idx < targetnum; idx++) {
				float* pdata = tensor_out->mutable_cpu_data() + idx * infonum;
				float conf = pdata[4];
				if (conf > 0.7) {
					//dbg(conf);
					HeadBox headbox(pdata[0], pdata[1], pdata[2], pdata[3], conf);
					head_list.push_back(headbox);
				}
			}

			int pad = std::abs(image.cols - image.rows) / 2;
			bool is_vertical_pad = image.cols > image.rows;

			for (auto& bbox : head_list) {
				bbox.mul_ratio(mapping_ratio);
				if (is_vertical_pad) {
					bbox.ymin -= pad;
					bbox.ymax -= pad;
				}
				else {
					bbox.xmin -= pad;
					bbox.xmax -= pad;
				}
			}

			std::vector<HeadBox> effect_head_list;
			for (auto& head : head_list) {
				auto region = head.get_rect();
				if (region.width >= 50 || region.height >= 50) effect_head_list.push_back(head);
			}

			nms_cpu(effect_head_list, 0.75);

			return effect_head_list;
		}

		float classify(HeadBox& headbox, cv::Mat image) {
			auto det_phone_region = headbox.DetPhoneRegion();
			cv::Mat cls_img = safty_cut(image, det_phone_region);
			auto letter_img = imgPreProcess(cls_img, 224, 112);

			cv::cvtColor(letter_img, letter_img, cv::COLOR_BGR2RGB);
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			auto det_rst_map = classi_instance_->forward(letter_img.data, { 1, letter_img.rows, letter_img.cols, letter_img.channels() }, RKNN_TENSOR_NHWC);
#else
			auto input_tensor = matConverTensor(letter_img);
			const std::array<float, 3> cls_mean{ 123.675, 116.28, 103.53 };
			const std::array<float, 3> cls_std{ 58.395, 57.12, 57.375 };
			int HWStep = input_tensor->width() * input_tensor->height();
			for (int c = 0; c < input_tensor->channels(); c++) {
				auto cpdata = input_tensor->mutable_cpu_data() + c * HWStep;
				for (int i = 0; i < HWStep; i++) cpdata[i] = (cpdata[i] - cls_mean[c]) / cls_std[c];
			}
			auto det_rst_map = classi_instance_->forward(input_tensor);
#endif
			auto tensor_out = det_rst_map.begin()->second;
			return tensor_out->cpu_data()[0];
		}

		void run_detect(std::vector<box_info_internal>& results, cv::Mat& image, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
		{
			std::vector<HeadBox> head_list = head_detect(image);

			//cv::Mat run_detect_visual = image.clone();
			for (auto& head : head_list) {
				//cv::rectangle(run_detect_visual, head.get_rect(), { 0,0,240 }, 5);
				//cv::rectangle(run_detect_visual, head.DetPhoneRegion(), { 0,240,0 }, 5);				
				float cls_score = classify(head, image);

				box_info_internal in_box_info;
				in_box_info.category = cls_score;
				in_box_info.confidence = head.score;
				cv::Rect det_phone_region = head.DetPhoneRegion();
				in_box_info.x1 = det_phone_region.x;
				in_box_info.y1 = det_phone_region.y;
				in_box_info.x2 = det_phone_region.x + det_phone_region.width;
				in_box_info.y2 = det_phone_region.y + det_phone_region.height;
				results.push_back(in_box_info);
			}

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

		void nms_cpu(std::vector<HeadBox>& bboxes, float iou_thres) {
			if (bboxes.empty()) return;
			std::sort(bboxes.begin(), bboxes.end(), [&](HeadBox b1, HeadBox b2) {return b1.score > b2.score; });
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
			std::sort(bboxes.begin(), bboxes.end(), [&](HeadBox b1, HeadBox b2) {return b1.score > b2.score; });
		}

		std::shared_ptr<glasssix::memory::tensor<float>> matConverTensor(cv::Mat input_image) {
			std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, input_image.rows, input_image.cols, 3}, -1, glasssix::memory::NHWC));
			std::copy(input_image.data, input_image.data + input_image.step[0] * input_image.rows, input_tensor_u8->mutable_cpu_data());
			input_tensor_u8->convert_order();
			auto input_tensor = input_tensor_u8 | glasssix::memory::tensor_convert_to<float>;
			return input_tensor;
		}

		std::shared_ptr<glasssix::memory::tensor<float>> tensor_transpose_0132(const std::shared_ptr<glasssix::memory::tensor<float>>& bottom) {
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
					for (int line = 0; line < height; line++) { //for 5
						top_ptr[i * height + line] = bottom_ptr[line * W_step + i];
					}
				}
			}
			return top;
		}


	private:
		std::string model_directory_;
		int device_;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
		std::unique_ptr<rknnwrapper::rknn_wrapper> detect_instance_;
		std::unique_ptr<rknnwrapper::rknn_wrapper> classi_instance_;
#else
		std::unique_ptr<onnxrt::pipline> detect_instance_;
		std::unique_ptr<onnxrt::pipline> classi_instance_;
#endif
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
