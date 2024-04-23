#include "detect_code_internal.hpp"
#include <opencv2/opencv.hpp>
#include <GenPipeline/GenPipeline.hpp>
#include <GenPipeline/GenPipeTools.hpp>

namespace glasssix::fighting
{
	class detect_code_internal::impl
	{
	public:
		impl(std::string_view model_directory, int device, int batch) :BATCH_(batch)
		{
			std::string model_directory_ = exposing::to_narrow_string(model_directory);
			if (*model_directory_.rbegin() != '/') model_directory_ += '/';
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			std::string model_ext(".rknn");
#elif defined(USE_BMNN)
			std::string model_ext(".bmodel");
#else
			std::string model_ext(".onnx");
#endif
			if (BATCH_ == 10) {
				nonm_instance_ = std::make_unique<GenPipeline>(model_directory_ + "fight_10b.nnm" + model_ext, 0);// not normalization if rknn
				CROP_SIZE_H = 256;
				CROP_SIZE_W = 460;
			}
			else if (BATCH_ == 8) {
				instance_ = std::make_unique<GenPipeline>(model_directory_ + "fight_8b" + model_ext, 0);// not normalization if rknn
				CROP_SIZE_H = 256;
				CROP_SIZE_W = 256;
			}
			else
				throw exposing::abi_invalid_argument("fighting incorrect BATCH_ param");

			std::array<float, 3> means_v{ 123.675, 116.28, 103.53 };
			std::array<float, 3> stand_v{ 58.395, 57.12, 57.375 };

			std::vector<cv::Mat> std_channels;
			std::vector<cv::Mat> mean_channels;
			for (int i = 0; i < 3; i++) {
				/* Extract an individual channel. */
				cv::Mat std_channel(CROP_SIZE_H, CROP_SIZE_W, CV_32FC1, cv::Scalar(1.f / stand_v[i]));
				std_channels.push_back(std_channel);
				cv::Mat mean_channel(CROP_SIZE_H, CROP_SIZE_W, CV_32FC1, cv::Scalar(-means_v[i]));
				mean_channels.push_back(mean_channel);
			}
			cv::merge(std_channels, m_std);
			cv::merge(mean_channels, m_mean);

			f32ImgsArr.resize(BATCH_ * 3 * CROP_SIZE_H * CROP_SIZE_W);
		}

		float detect(exposing::param_span<std::uint8_t> bitmap, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map_std)
		{
			size_t bitmap_size = bitmap.size();
			CHECK_EQ(bitmap.size(), BATCH_ * height * width * 3);
			/* Split bitmap to img vector. */
			std::vector<cv::Mat> BatchImgs;
			for (int i = 0; i < BATCH_; i++) {
				cv::Mat InteImage(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data() + i * 3 * height * width));

				if (BATCH_ == 10) {
					cv::resize(InteImage, InteImage, cv::Size2i{ 460, 256 });
					cv::cvtColor(InteImage, InteImage, cv::COLOR_BGR2RGB);// dbg("cvtColor");
					//InteImage = letter_image_(InteImage, 460, 256);
				}
				BatchImgs.push_back(InteImage);
			}

			float fight_score = -1.f;
			if (BATCH_ == 10) {
				fight_score = detect_10B_handnormalization(BatchImgs);
			}
			//else if (BATCH_ == 8) {
			//	fight_score = detect_8B(BatchImgs);
			//}
			return fight_score;
		}

		float detect_10B_handnormalization(const std::vector<cv::Mat>& BatchImgs) {
			static constexpr int channel_ = 3; //RGB
			const int HWStep = CROP_SIZE_H * CROP_SIZE_W;

			/// Subsequent operations will not allocate new memory 
			/// but implicitly manipulate the memory space of f32ImgsArr.data 
			/// by using cv::Mat in an referencing external memory mode."
			for (int imgIdx = 0; imgIdx < BATCH_; imgIdx++) {
				cv::Mat sample_float(CROP_SIZE_H, CROP_SIZE_W, CV_32FC3);
				BatchImgs[imgIdx].convertTo(sample_float, CV_32FC3);
				cv::add(sample_float, m_mean, sample_float);	 // sample_float += m_mean
				cv::multiply(sample_float, m_std, sample_float); // sample_float *= m_std

				auto f32ImgsArr_img = f32ImgsArr.data() + imgIdx * 3 * HWStep;
				cv::Mat input_c0(CROP_SIZE_H, CROP_SIZE_W, CV_32FC1, f32ImgsArr_img + 0 * HWStep);
				cv::Mat input_c1(CROP_SIZE_H, CROP_SIZE_W, CV_32FC1, f32ImgsArr_img + 1 * HWStep);
				cv::Mat input_c2(CROP_SIZE_H, CROP_SIZE_W, CV_32FC1, f32ImgsArr_img + 2 * HWStep);
				std::array<cv::Mat, 3> input_channels{ input_c0 ,input_c1 ,input_c2 };
				cv::split(sample_float, input_channels); //eq split to f32ImgsArr
			}

#ifdef USE_RKNN2API // rknn net only accept HWC, a terrible trouble-maker! :-( 
			/* NOTE: Transposing will allocate new memory for the result. No changed f32ImgsArr.data()`s internal values */
			cv::Mat f32ImgsArr_TS_HW_C(BATCH_ * 3, HWStep, CV_32FC1, f32ImgsArr.data());
			cv::transpose(f32ImgsArr_TS_HW_C, f32ImgsArr_TS_HW_C); //{30, 256, 460} -> {256, 460, 30} 30:10*3
			auto ts_f32_rst_map = nonm_instance_->forward((float*)f32ImgsArr_TS_HW_C.data, { 1, 256, 460, 30 }, 1); //RKNN_TENSOR_NHWC is 1
#else
			auto ts_f32_rst_map = nonm_instance_->forward(f32ImgsArr.data(), { 1, 30, 256, 460 }, 0);
#endif
			auto det_scores = ts_f32_rst_map.begin()->second->cpu_data();
			return det_scores[0];
		}

		std::string version()
		{
			const std::string algo_module_version = "3.0.0";
			std::string nn_frame_version = instance_->version();
			return fmt::format(R"({ {"nn_frame_version":"{}", "algo_module_version" : "{}"} })", nn_frame_version, algo_module_version);
		}

		//std::array<cv::Mat, 3> threecrop(cv::Mat InteImage, int size) {
		//	std::array<cv::Mat, 3> rst;
		//	int H = InteImage.rows;
		//	int W = InteImage.cols;
		//	if (H == W) {
		//		cv::resize(InteImage, InteImage, { size,size });
		//		rst[0] = InteImage;
		//		rst[1] = InteImage;
		//		rst[2] = InteImage;
		//	}
		//	else {
		//		float reszie_ratio = size * 1.f / std::min(H, W);
		//		bool if_horizon = W >= H;
		//		CHECK_EQ(if_horizon, true); // lazy 2 write vertical image, meet call me -_-
		//		if (if_horizon) {
		//			int new_H = size;
		//			int new_W = W * reszie_ratio;
		//			cv::resize(InteImage, InteImage, { new_W,new_H }); //cv::Size{W,H}
		//			CHECK_GT(new_W, new_H);
		//			rst[0] = GenPipeTools::safty_cut(InteImage, cv::Rect(0, 0, size, size));
		//			rst[1] = GenPipeTools::safty_cut(InteImage, cv::Rect((new_W - size) / 2, 0, size, size));
		//			rst[2] = GenPipeTools::safty_cut(InteImage, cv::Rect(new_W - size, 0, size, size));
		//		}
		//	}
		//	return rst;
		//}

	private:
		std::unique_ptr<GenPipeline> nonm_instance_;
		std::unique_ptr<GenPipeline> instance_;
		int BATCH_;
		int CROP_SIZE_H;
		int CROP_SIZE_W;
		cv::Mat m_mean;
		cv::Mat m_std;
		std::vector<float> f32ImgsArr;
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
