#include "detect_code_internal.hpp"

#include <algorithm>
#include <numeric>

#include <Excalibur/pipeline.hpp>
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include "Primitives/tensor_conversions.hpp"
#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_resize.hpp"
#include "Excalibur/operation_rgb2gray.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#ifdef USE_RKNNAPI
//#if 0
#include "../../common/include/RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#endif

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

namespace glasssix::fighting
{
	class detect_code_internal::impl
	{
	public:
		impl(std::string_view model_directory, int device, int batch) :BATCH_(batch)
		{
			std::vector<std::string> empty_hold;
			if (BATCH_ == 10) {
				instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(empty_hold, std::string(model_directory) + "/" + "fight_10b" + ".rknn", device);
				CROP_SIZE_H = 256;
				CROP_SIZE_W = 460;
			}
			else if (BATCH_ == 8) {
				instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(empty_hold, std::string(model_directory) + "/" + "fight_8b" + ".rknn", device);
				CROP_SIZE_H = 256;
				CROP_SIZE_W = 256;
			}
			else
				throw exposing::abi_invalid_argument("incorrect BATCH_ param");
		}

		float detect(exposing::param_span<std::uint8_t> bitmap, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map_std)
		{
			size_t bitmap_size = bitmap.size();
			CHECK_EQ(bitmap.size(), BATCH_ * height * width * 3);
			// split bitmap to img vector
			std::vector<cv::Mat> BatchImgs;
			for (int i = 0; i < BATCH_; i++) {
				cv::Mat InteImage(cv::Size(width, height), CV_8UC3);
				std::memcpy(InteImage.data, bitmap.data() + height * width * 3 * i, sizeof(uint8_t) * height * width * 3);

				if (BATCH_ == 10) {
					InteImage = letter_image_(InteImage, 460, 256);
				}
				BatchImgs.push_back(InteImage);
			}

			float fight_score = -1.f;
			if (BATCH_ == 10) {
				fight_score = detect_10B(BatchImgs);
			}
			else if (BATCH_ == 8) {
				fight_score = detect_8B(BatchImgs);
			}
			return fight_score;
		}

		float detect_10B(const std::vector<cv::Mat>& BatchImgs) {
			// three crop img and sort out
			CHECK_EQ(BATCH_, 10);
			static constexpr int channel_ = 3; //RGB

			const int HWstep = CROP_SIZE_H * CROP_SIZE_W;
			cv::Mat inputMat(cv::Size(HWstep, BATCH_ * channel_), CV_8UC1);

			int inpuCopyFlag = 0;
			CropsPushTensor(BatchImgs, inputMat, inpuCopyFlag, HWstep);
			cv::transpose(inputMat, inputMat);

			if (!inputMat.isContinuous()) inputMat = inputMat.clone();
			auto det_rst_map = instance_->forward(inputMat.data, { 1, CROP_SIZE_H, CROP_SIZE_W, BATCH_ * channel_ }, RKNN_TENSOR_NHWC);
			auto det_scores = det_rst_map.begin()->second->cpu_data();
			return det_scores[0];
		}

		float detect_8B(std::vector<cv::Mat>& BatchImgs) {
			CHECK_EQ(CROP_SIZE_H, CROP_SIZE_W);
			const int CROP_SIZE = CROP_SIZE_H;
			// three crop img and sort out
			std::vector<cv::Mat> headCrops;
			std::vector<cv::Mat> mediCrops;
			std::vector<cv::Mat> tailCrops;
			for (auto& integImg : BatchImgs) {
				auto [c_head, c_medi, c_tail] = threecrop(integImg, CROP_SIZE_H);
				headCrops.push_back(c_head);
				mediCrops.push_back(c_medi);
				tailCrops.push_back(c_tail);
			}

			int inpuCopyFlag = 0;
			cv::Mat inputMat(cv::Size(CROP_SIZE * CROP_SIZE, BATCH_ * 9), CV_8UC1);//I dont know, but its right

			CropsPushTensor(headCrops, inputMat, inpuCopyFlag, CROP_SIZE * CROP_SIZE);
			CropsPushTensor(tailCrops, inputMat, inpuCopyFlag, CROP_SIZE * CROP_SIZE);
			CropsPushTensor(mediCrops, inputMat, inpuCopyFlag, CROP_SIZE * CROP_SIZE);
			cv::transpose(inputMat, inputMat);

			if (!inputMat.isContinuous()) inputMat = inputMat.clone();
			auto det_rst_map = instance_->forward(inputMat.data, { 1, CROP_SIZE, CROP_SIZE, 9 * BATCH_ }, RKNN_TENSOR_NHWC);
			auto det_scores = det_rst_map.begin()->second->cpu_data();
			return det_scores[0];
		}

		// Crops trans BGRBGR.. -> RR..GG..BB..
		void CropsPushTensor(const std::vector<cv::Mat>& Crops, cv::Mat& inputMat_dst, int& inpuCopyFlag, int cropHWStep) {
			for (const auto& crop : Crops) {
				std::vector<cv::Mat> channels;//split channels[0]:R channels[1]:G channels[2]:B
				split(crop, channels);
				for (int c = 0; c < 3; c++) {
					std::copy(channels[c].data, channels[c].data + cropHWStep, inputMat_dst.data + inpuCopyFlag);
					inpuCopyFlag += cropHWStep;
				}
			}
		}

		std::string version()
		{
			const std::string algo_module_version = "2.0.0";
			std::string nn_frame_version = instance_->version();
			return fmt::format(R"({ {"nn_frame_version":"{}", "algo_module_version" : "{}"} })", nn_frame_version, algo_module_version);
		}

		std::array<cv::Mat, 3> threecrop(cv::Mat InteImage, int size) {
			std::array<cv::Mat, 3> rst;
			int H = InteImage.rows;
			int W = InteImage.cols;
			if (H == W) {
				cv::resize(InteImage, InteImage, { size,size });
				rst[0] = InteImage;
				rst[1] = InteImage;
				rst[2] = InteImage;
			}
			else {
				float reszie_ratio = size * 1.f / std::min(H, W);
				bool if_horizon = W >= H;
				CHECK_EQ(if_horizon, true); // lazy 2 write vertical image, meet call me -_-
				if (if_horizon) {
					int new_H = size;
					int new_W = W * reszie_ratio;
					cv::resize(InteImage, InteImage, { new_W,new_H }); //cv::Size{W,H}

					CHECK_GT(new_W, new_H);
					rst[0] = safty_cut(InteImage, cv::Rect(0, 0, size, size));
					rst[1] = safty_cut(InteImage, cv::Rect((new_W - size) / 2, 0, size, size));
					rst[2] = safty_cut(InteImage, cv::Rect(new_W - size, 0, size, size));
				}
			}

			return rst;
		}

		static inline cv::Mat letter_image_(cv::Mat img, int hope_w, int hope_h)
		{
			int H = img.rows;
			int W = img.cols;

			if (H == hope_h && W == hope_w) return img;

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

		cv::Mat safty_cut(cv::Mat& img, cv::Rect roi)
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

	private:
		std::unique_ptr<rknnwrapper::rknn_wrapper> instance_;
		int BATCH_;
		int CROP_SIZE_H;
		int CROP_SIZE_W;
	};

	detect_code_internal::detect_code_internal(std::string_view model_directory, int device, int BATCH_)
		: impl_{ std::make_unique<impl>(model_directory, device, BATCH_) }
	{
	}

	detect_code_internal::~detect_code_internal()
	{
	}

	float detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map_std)
	{
		return impl_->detect(bitmap, height, width, roi_x, roi_y, roi_width, roi_height, param_map_std);
	}

	std::string detect_code_internal::version()
	{
		return impl_->version();
	}
}
