#pragma once
#include<vector>
#include<opencv2/opencv.hpp>
#include <Primitives/tensor.hpp>
#include "Excalibur/pipeline.hpp"
#include <Primitives/tensor_conversions.hpp>
#include <abi/exceptions.hpp>

#ifdef USE_RKNNAPI
//#if 0
#include "../../common/include/RKNNWrapper/rknn_wrapper.hpp"
#define USE_RKNN
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#define USE_RKNN
#endif

//#include "dbg.h"
#include "ObjBox.hpp"
#include "img_preprocess.hpp"

namespace glasssix::playphone
{
	
	class RknnYolov8Pipline {
	public:

		using TensorSptr = std::shared_ptr<glasssix::memory::tensor<float>>;

		RknnYolov8Pipline(std::string model, int device = -1);

		std::unordered_map<std::string, TensorSptr> forward(cv::Mat img);

		std::vector<ObjBox> detect(cv::Mat image, cv::Point image_start, float conf_thres, float iou_thres);

		std::string version();

	private:
		std::vector<TensorSptr> sort_yolo_rst(const std::unordered_map<std::string, TensorSptr>&);
		TensorSptr yolov8_concat(std::vector<TensorSptr>&);

		enum class PipType { unknown, rknn, excalibur, onnx };
		PipType pipType_ = PipType::unknown;

		// switch pipline
		std::unique_ptr<excalibur::pipeline<float>> base_instance_exbr_;
#ifdef USE_RKNN
		std::unique_ptr<rknnwrapper::rknn_wrapper> base_instance_rknn_;
#endif // USE_RKNN

	};

	static inline float sigmoid_x(float x) {
		return static_cast<float>(1.f / (1.f + exp(-x)));
	}

	static inline RknnYolov8Pipline::TensorSptr tensor_transpose_0132(const RknnYolov8Pipline::TensorSptr& bottom) {
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
				for (int line = 0; line < height; line++) { //for 6
					top_ptr[i * height + line] = bottom_ptr[line * W_step + i];
				}
			}
		}
		return top;
	}

}
