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



namespace glasssix::onphone
{
	class RknnYolov8Wrapper {
	public:

		using TensorSptr = std::shared_ptr<glasssix::memory::tensor<float>>;

		RknnYolov8Wrapper(std::string racy, int device = -1, uint32_t flag = 0);

		std::unordered_map<std::string, TensorSptr> forward(cv::Mat letter_img)
		{
			auto det_rst_map = base_instance_->forward(letter_img.data, { 1, letter_img.rows, letter_img.cols, letter_img.channels() }, RKNN_TENSOR_NHWC);
			auto det_rst_vec = sort_yolo_rst(det_rst_map);
			TensorSptr concat_tensor_ptr = yolov8_complement(det_rst_vec);

			std::unordered_map<std::string, TensorSptr> result_map;
			result_map.try_emplace("concat_output", concat_tensor_ptr); // complement result 
			return result_map;
		}
		std::string version();

	private:
		std::unique_ptr<rknnwrapper::rknn_wrapper> base_instance_;
		std::vector<TensorSptr> sort_yolo_rst(const std::unordered_map<std::string, TensorSptr>&);
		TensorSptr yolov8_complement(std::vector<TensorSptr>&);
	};

	static inline float sigmoid_x(float x) {
		return static_cast<float>(1.f / (1.f + exp(-x)));
	}
}
