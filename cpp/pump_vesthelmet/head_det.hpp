#pragma once
#ifndef _HEADDET_  
#define _HEADDET_
#include <vector>
#include "obj_box_info.hpp"

namespace glasssix::pump_vesthelmet
{
	static std::shared_ptr<glasssix::memory::tensor<float>> sp_concat_tensor(std::shared_ptr<glasssix::memory::tensor<float>>& A, std::shared_ptr<glasssix::memory::tensor<float>>& B) {
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

	static std::shared_ptr<glasssix::memory::tensor<float>> tensor_transpose_0132(const std::shared_ptr<glasssix::memory::tensor<float>>& bottom) {
		int num = bottom->num();
		int channels = bottom->channels();
		int height = bottom->height();
		int width = bottom->width();
		//CHECK_EQ(bottom->channels(), D * C);
		auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{num, channels, width, height}, -1, glasssix::memory::NCHW);

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

	static std::shared_ptr<glasssix::memory::tensor<float>> yolov8_complement(std::vector<std::shared_ptr<glasssix::memory::tensor<float>>>& vec_ts_rstSort)
	{
		// VF means VISUAL FIELD
		static constexpr int INTEGR_ONNX_OUT_STD_INFO_NUM_ = 5;
		static constexpr int CUT_MODEL_VISUALFIELD_RAW_INFO_ = 65; // include 4*BBoxlocaInfo(16 usually) + scores (64+N)

		int INTEGR_ONNX_OUT_LINES = 0;
		std::vector<int> blockSide; // blockSide maybe  { ..., 80, 40, 20 }; or means visual field size
		for (auto& node : vec_ts_rstSort) {
			auto shape = node->data_shape();
			CHECK_EQ(shape.size(), 4);
			CHECK_EQ(shape[0], 1);
			CHECK_EQ(shape[1], CUT_MODEL_VISUALFIELD_RAW_INFO_);
			CHECK_EQ(shape[2], shape[3]);
			blockSide.push_back(shape[3]);
			INTEGR_ONNX_OUT_LINES += shape[2] * shape[3];
		}

		auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{1, 1, INTEGR_ONNX_OUT_LINES, INTEGR_ONNX_OUT_STD_INFO_NUM_}, -1, glasssix::memory::NCHW);
		float* top_data = top->mutable_cpu_data();
		size_t top_visual_field_counter = 0;

		for (int i = 0; i < vec_ts_rstSort.size(); i++) {

			auto& Scaleblock = vec_ts_rstSort[i];
			Scaleblock->reshape(std::vector<int>{1, 1, CUT_MODEL_VISUALFIELD_RAW_INFO_, blockSide[i] * blockSide[i]});
			Scaleblock = tensor_transpose_0132(Scaleblock); // 1, 1, 65, 6400(if) -> 1, 1, 6400, 65
			//YHC
			const int visual_field_nums = Scaleblock->data_shape()[2]; // 1, 1, 6400(if), 65 -> [2]=6400(if)
			const int per_raw_line_length = Scaleblock->data_shape()[3]; // 65
			CHECK_EQ(CUT_MODEL_VISUALFIELD_RAW_INFO_, per_raw_line_length);//per_raw_line_length should EQ CUT_MODEL_VISUALFIELD_RAW_INFO_

			for (int visual_field_idx = 0; visual_field_idx < visual_field_nums; visual_field_idx++) // loop 6400 |
			{
				float* uintInfoLinePtrData = Scaleblock->mutable_cpu_data() + visual_field_idx * per_raw_line_length;
				// {65 = 64 + 2}, {64 = 16 * 4}, {16 * 4 conv 16 -> 4}, 4 means raw location

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
				// <score>
				// score[0], score[1] = [64+0], [64+1]...
				uintInfoLinePtrData[64] = sigmoid_x(uintInfoLinePtrData[64]);
				// </score>

				raw_location[0] = 0.5 + visual_field_idx % blockSide[i] - raw_location[0];
				raw_location[1] = 0.5 + visual_field_idx / blockSide[i] - raw_location[1];

				raw_location[2] = 0.5 + visual_field_idx % blockSide[i] + raw_location[2];
				raw_location[3] = 0.5 + visual_field_idx / blockSide[i] + raw_location[3];

				float loaction_0 = (raw_location[2] + raw_location[0]) / 2;
				float loaction_1 = (raw_location[3] + raw_location[1]) / 2;

				float loaction_2 = raw_location[2] - raw_location[0];
				float loaction_3 = raw_location[3] - raw_location[1];

				loaction_0 = loaction_0 / blockSide[i]; // Equivalent operation for * mul_{8,16,32} / div_{360} 
				loaction_1 = loaction_1 / blockSide[i];
				loaction_2 = loaction_2 / blockSide[i];
				loaction_3 = loaction_3 / blockSide[i];

				auto top_line_data = top_data + top_visual_field_counter * INTEGR_ONNX_OUT_STD_INFO_NUM_;
				top_line_data[0] = loaction_0;
				top_line_data[1] = loaction_1;
				top_line_data[2] = loaction_2;
				top_line_data[3] = loaction_3;
				top_line_data[4] = uintInfoLinePtrData[64];

				top_visual_field_counter++;
			}
		}
		return top;
	}


	static inline std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> sort_yolo_rst(const std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& result, bool order = true) {
		std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> outRst;
		for (auto& out : result) {
			outRst.push_back(out.second);
		}
		std::sort(outRst.begin(), outRst.end(), [&order](const std::shared_ptr<glasssix::memory::tensor<float>>& A, const std::shared_ptr<glasssix::memory::tensor<float>>& B) {
			auto countA = A->count();
			auto countB = B->count();
			return !((countA > countB) ^ order);
			});
		return outRst;
	}

}//namesapce glasssix::pump_vesthelmet
#endif //!_HEADDET_