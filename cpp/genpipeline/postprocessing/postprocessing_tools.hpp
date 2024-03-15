/*
	postprocessing_tools.hpp :
	helper functions set for Postprocessor implement
*/

#pragma once;
#include <Primitives/tensor.hpp>
//#include "numpy_extensor/numpyExtensor.hpp"

using TensorSptr = std::shared_ptr<glasssix::memory::tensor<float>>;

namespace PostprocessingTools {

	// order=true count_sort{7,6,5,4..}; order=false count_sort{4,5,6..}
	static inline std::vector<TensorSptr> sort_yolo_rst(const std::unordered_map<std::string, TensorSptr>& result, bool order = true) {
		std::vector<TensorSptr> outRst;
		for (auto& out : result) {
			outRst.push_back(out.second);
		}
		std::sort(outRst.begin(), outRst.end(), [&order](const TensorSptr& A, const TensorSptr& B) {
			auto countA = A->count();
			auto countB = B->count();
			return !((countA > countB) ^ order);
			});
		return outRst;
	}

	static inline TensorSptr sp_concat_tensor(TensorSptr& A, TensorSptr& B) {
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

	static inline std::shared_ptr<glasssix::memory::tensor<float>> tensor_transpose_0132(const std::shared_ptr<glasssix::memory::tensor<float>>& bottom) {
		int num = bottom->num();
		int channels = bottom->channels();
		int height = bottom->height();
		int width = bottom->width();
		//CHECK_EQ(bottom->channels(), D * C);
		auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{num, channels, width, height}, -1, memory::NCHW);

		int W_step = width;
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

	static inline float sigmoid_x(float x)
	{
		return static_cast<float>(1.f / (1.f + exp(-x)));
	}

	static inline void Softmax(float* data, int num)
	{
		double L2_Sum = 0.f;
		for (size_t i = 0; i < num; i++)
		{
			data[i] = (exp(data[i]));
			L2_Sum += data[i];
		}
		for (size_t i = 0; i < num; i++)
		{
			data[i] = data[i] / L2_Sum;
		}
	}

}