#pragma once
#include<vector>
#include<opencv2/opencv.hpp>
#include <Primitives/tensor.hpp>
#ifdef USE_RKNNAPI
//#if 0
#include "../../common/include/RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#endif



namespace glasssix::flame
{
	class RknnYolov8Wrapper {
	public:

		using TensorSptr = std::shared_ptr<glasssix::memory::tensor<float>>;

		RknnYolov8Wrapper(std::string racy, int device = -1, uint32_t flag = 0);

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
		std::unordered_map<std::string, TensorSptr> forward(cv::Mat letter_img)
#else
		std::unordered_map<std::string, TensorSptr> forward(TensorSptr input_tensor)
#endif
		{
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			auto det_rst_map = base_instance_->forward(letter_img.data, { 1, letter_img.rows, letter_img.cols, letter_img.channels() }, RKNN_TENSOR_NHWC);
#else
			auto det_rst_map = base_instance_->forward(input_tensor);
#endif
			auto det_rst_vec = sort_yolo_rst(det_rst_map);
			TensorSptr concat_tensor_ptr = flame_yolov8_complement(det_rst_vec);

			std::unordered_map<std::string, TensorSptr> result_map;
			result_map.try_emplace("concat_output", concat_tensor_ptr); // complement result 
			return result_map;
		}
		std::string version();

#ifdef BUILD_DEBUG_INFO
		void CosineSimilarity(TensorSptr A, TensorSptr B, int logLen = 0) {
			CHECK_EQ(A->count(), B->count());
			if (A->data_shape() != B->data_shape()) {
				std::cout << "CosineSimilarity Inputs Shape Not EQ!" << std::endl;;
				return;
			};
			int count = A->count();
			const float* AData = A->cpu_data();
			const float* BData = B->cpu_data();

			float sum0 = 0.f, sum1 = 0.f, sum2 = 0.f;
			std::cout << "CosineSimilarity [A,B] :" << std::endl;
			for (int i = 0; i < count; i++)
			{
				if (i < logLen)std::cout << "[" << std::fixed << std::setprecision(6) << AData[i] << "," << BData[i] << "] ";
				sum0 += AData[i] * BData[i];
				sum1 += AData[i] * AData[i];
				sum2 += BData[i] * BData[i];
			}

			float cos = sum0 / std::sqrt(sum1 * sum2);
			std::cout << "CosineSimilarity = " << cos << std::endl;
		};
#endif
	private:
		std::unique_ptr<rknnwrapper::rknn_wrapper> base_instance_;
		std::vector<TensorSptr> sort_yolo_rst(const std::unordered_map<std::string, TensorSptr>&);
		TensorSptr flame_yolov8_complement(std::vector<TensorSptr>&);
	};

	static inline float sigmoid_x(float x) {
		return static_cast<float>(1.f / (1.f + exp(-x)));
	}
}
