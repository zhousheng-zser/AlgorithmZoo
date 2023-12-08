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

			if (det_rst_vec.size() == 6) {
				det_rst_vec[0] = sp_concat_tensor(det_rst_vec[0], det_rst_vec[3]);
				det_rst_vec[1] = sp_concat_tensor(det_rst_vec[1], det_rst_vec[4]);
				det_rst_vec[2] = sp_concat_tensor(det_rst_vec[2], det_rst_vec[5]);
			}

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

		TensorSptr sp_concat_tensor(TensorSptr& A, TensorSptr& B) {
			auto A_shape = A->data_shape();
			auto B_shape = B->data_shape();
			CHECK_EQ(A_shape[2], B_shape[2]);
			CHECK_EQ(A_shape[3], B_shape[3]);
			CHECK_EQ(A_shape[2], A_shape[3]);
			CHECK_GT(A_shape[1], 1);
			CHECK_EQ(B_shape[1], 1);
			int sideLength = A_shape[2];

			auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{1, A_shape[1] + B_shape[1], sideLength, sideLength}, -1, memory::NCHW);
			CHECK_EQ(top->count(), A->count() + B->count());

			std::copy(A->mutable_cpu_data(), A->mutable_cpu_data() + A->count(), top->mutable_cpu_data());
			std::copy(B->mutable_cpu_data(), B->mutable_cpu_data() + B->count(), top->mutable_cpu_data() + A->count());

			return top;
		}

	};

	static inline float sigmoid_x(float x) {
		return static_cast<float>(1.f / (1.f + exp(-x)));
	}
}
