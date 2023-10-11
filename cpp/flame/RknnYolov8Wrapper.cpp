#include "RknnYolov8Wrapper.hpp"
#include "numpyExtensor.hpp"
#include <algorithm>
#include <numeric>
#include <math.h>
#include "detect_code_internal.hpp"

namespace glasssix::flame
{
	void softmax(float* input, int len) {
		float total = 0.f;
		for (int i = 0; i < len; i++) {
			total += exp(input[i]);
			input[i] = exp(input[i]);
		}
		for (int i = 0; i < len; i++) {
			input[i] = input[i] / total;
		}
	}

	RknnYolov8Wrapper::TensorSptr RknnYolov8Wrapper::flame_yolov8_complement(std::vector<TensorSptr>& vec_ts_rstSort)
	{
		static constexpr int blockSide[3] = { 80, 40, 20 }; //ScaleSteps[3][2] = { {80, 80}, {40, 40}, {20, 20} };
		CHECK_EQ(3, vec_ts_rstSort.size());

		auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{1, 1, 8400, 6}, -1, memory::NCHW);
		float* top_data = top->mutable_cpu_data();
		size_t top_line_counter = 0;

		for (int i = 0; i < 3; i++) {

			auto& Scaleblock = vec_ts_rstSort[i];
			Scaleblock->reshape(std::vector<int>{1, 1, 66, blockSide[i] * blockSide[i]});
			Scaleblock = tensor_transpose_0132(Scaleblock); // 1, 1, 66, 6400 -> 1, 1, 6400, 66

			int line_num = Scaleblock->data_shape()[2]; // 6400 + 1600 + 400 = 8400
			int per_line_length = Scaleblock->data_shape()[3]; // 66
			for (int line = 0; line < line_num; line++) // loop 6400 |
			{
				float* uintInfoLinePtrData = Scaleblock->mutable_cpu_data() + line * per_line_length;
				// {66 = 64 + 2}, {64 = 16 * 4}, {16 * 4 conv 16 -> 4}, 4 means raw location

				float raw_location[4] = { 0.f,0.f,0.f,0.f };
				float sotfmax_total[4] = { 0.f,0.f,0.f,0.f };

				// softmax
				for (int exp_i = 0; exp_i < 64; exp_i++) // per conv_group kernel = {0,1,2,3,...,15}, len 16
				{
					uintInfoLinePtrData[exp_i] = exp(uintInfoLinePtrData[exp_i]);
					sotfmax_total[exp_i / 16] += uintInfoLinePtrData[exp_i];
				}

				// convolution with after-softmax
				for (int exp_i = 0; exp_i < 64; exp_i++) // per conv_group kernel = {0,1,2,3,...,15}, len 16
				{
					// exp_i / 16 : loop div 0...0, 1..1, 2..2, 3..3 per length 16
					// exp_i % 16 : loop mul 0,1,2..15,  0,1,2..15,  0,1,2..15,  0,1,2..15
					uintInfoLinePtrData[exp_i] /= sotfmax_total[exp_i / 16]; // after-softmax operation
					raw_location[exp_i / 16] += uintInfoLinePtrData[exp_i] * (exp_i % 16);// convolution
				}

				uintInfoLinePtrData[64] = sigmoid_x(uintInfoLinePtrData[64]);
				uintInfoLinePtrData[65] = sigmoid_x(uintInfoLinePtrData[65]);

				raw_location[0] = 0.5 + line % blockSide[i] - raw_location[0];
				raw_location[1] = 0.5 + line / blockSide[i] - raw_location[1];

				raw_location[2] = 0.5 + line % blockSide[i] + raw_location[2];
				raw_location[3] = 0.5 + line / blockSide[i] + raw_location[3];

				float loaction_0 = (raw_location[2] + raw_location[0]) / 2;
				float loaction_1 = (raw_location[3] + raw_location[1]) / 2;

				float loaction_2 = raw_location[2] - raw_location[0];
				float loaction_3 = raw_location[3] - raw_location[1];

				loaction_0 = loaction_0 / blockSide[i]; // Equivalent operation for * mul_{8,16,32} / div_{360} 
				loaction_1 = loaction_1 / blockSide[i];
				loaction_2 = loaction_2 / blockSide[i];
				loaction_3 = loaction_3 / blockSide[i];

				auto top_line_data = top_data + top_line_counter * 6;
				top_line_data[0] = loaction_0;
				top_line_data[1] = loaction_1;
				top_line_data[2] = loaction_2;
				top_line_data[3] = loaction_3;
				top_line_data[4] = uintInfoLinePtrData[64];
				top_line_data[5] = uintInfoLinePtrData[65];
				top_line_counter++;
			}
		}
		return top;
	}

	std::string RknnYolov8Wrapper::version() {
		return "RknnYolov8Wrapper";
	}

	RknnYolov8Wrapper::RknnYolov8Wrapper(std::string racy, int device, uint32_t flag) {
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
		std::vector<std::string> rkn_phai;
		base_instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(rkn_phai, racy, device, flag);
#else
		base_instance_ = std::make_unique<onnxrt::pipline>(racy);
#endif
	}

	std::vector<RknnYolov8Wrapper::TensorSptr> RknnYolov8Wrapper::sort_yolo_rst(const std::unordered_map<std::string, TensorSptr>& result) {
		std::vector<TensorSptr> outRst;
		for (auto& out : result) {
			outRst.push_back(out.second);
		}
		std::sort(outRst.begin(), outRst.end(), [](const TensorSptr& A, const TensorSptr& B) {
			auto countA = A->count();
			auto countB = B->count();
			return countA > countB;
			});
		return outRst;
	}

}