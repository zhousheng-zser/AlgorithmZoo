#pragma once
#ifndef _POSTPROCESSING_
#define _POSTPROCESSING_

#include<vector>
#include<unordered_map>
#include<string>
#include<opencv2/opencv.hpp>
#include <Primitives/tensor.hpp>
//#include "numpyExtensor.hpp"
//#include "dbg.h"

namespace postprocessing
{
	using TensorSptr = std::shared_ptr<glasssix::memory::tensor<float>>;

	static inline float sigmoid_x(float x) {
		return static_cast<float>(1.f / (1.f + exp(-x)));
	}

	static inline TensorSptr tensor_transpose_0132(const TensorSptr& bottom) {
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


	// order=true count_sort{7,6,5,4..}; order=false count_sort{4,5,6..}
	static inline std::vector<TensorSptr> sort_yolo_rst(const std::unordered_map<std::string, TensorSptr>& result, bool order = true) {
		std::vector<TensorSptr> outRst;
		for (auto& out : result) {
			outRst.push_back(out.second);
		}
		std::sort(outRst.begin(), outRst.end(), [&order](const TensorSptr& A, const TensorSptr& B) {
			auto countA = A->count();
			auto countB = B->count();
			return !((countA > countB) ^ order); // order=true count_sort{7,6,5,4..}; order=false count_sort{4,5,6..}
			});
		return outRst;
	}


	//static inline TensorSptr yolov5_concat(std::vector<TensorSptr>& vec_ts_sorted) {
	//	static inline const float stride[3] = { 8, 16, 32 };
	//	static inline const float anchors[3][6] = {
	//	{10,13,  16,30,   33,23},    /* OP [120 160] FOR {ch1} {ch2} {ch3} */
	//	{30,61,  62,45,   59,119},   /*    [60  80 ] */
	//	{116,90, 156,198, 373,326}   /*    [30  40 ] */
	//	};
	//	// VF means VISUAL FIELD
	//	static constexpr int INTEGR_ONNX_OUT_STD_INFO_NUM_ = 6;
	//	static constexpr int CUT_MODEL_VISUALFIELD_RAW_INFO_ = 18; // 18 = 3*(4+1+1);  

	//	int INTEGR_ONNX_OUT_LINES = 0;
	//	std::vector<int> blockSide; // blockSide maybe  { 80, 40, 20 }; or means visual field size
	//	for (auto& node : vec_ts_sorted) {
	//		auto shape = node->data_shape();
	//		CHECK_EQ(shape.size(), 4);
	//		CHECK_EQ(shape[0], 1);
	//		CHECK_EQ(shape[1], CUT_MODEL_VISUALFIELD_RAW_INFO_);
	//		CHECK_EQ(shape[2], shape[3]); //20==20 40==40 80==80
	//		blockSide.push_back(shape[3]); //push_back 20, 40, 80
	//		INTEGR_ONNX_OUT_LINES += shape[2] * shape[3];
	//	}
	//	// ...
	//	auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{1, 1, INTEGR_ONNX_OUT_LINES, INTEGR_ONNX_OUT_STD_INFO_NUM_}, -1, memory::NCHW);
	//	return top;
	//}


	static inline TensorSptr yolov8_concat(std::vector<TensorSptr>& vec_ts_sorted)
	{
		// VF means VISUAL FIELD
		static constexpr int INTEGR_ONNX_OUT_STD_INFO_NUM_ = 5;
		static constexpr int CUT_MODEL_VISUALFIELD_RAW_INFO_ = 65; // include 4*BBoxlocaInfo(16 usually) + scores (64+N)

		int INTEGR_ONNX_OUT_LINES = 0;
		std::vector<int> blockSide; // blockSide maybe  { ..., 80, 40, 20 }; or means visual field size
		for (auto& node : vec_ts_sorted) {
			auto shape = node->data_shape();
			CHECK_EQ(shape.size(), 4);
			CHECK_EQ(shape[0], 1);
			CHECK_EQ(shape[1], CUT_MODEL_VISUALFIELD_RAW_INFO_);
			CHECK_EQ(shape[2], shape[3]);
			blockSide.push_back(shape[3]);
			INTEGR_ONNX_OUT_LINES += shape[2] * shape[3];
		}

		auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{1, 1, INTEGR_ONNX_OUT_LINES, INTEGR_ONNX_OUT_STD_INFO_NUM_}, -1, memory::NCHW);
		float* top_data = top->mutable_cpu_data();
		size_t top_visual_field_counter = 0;

		for (int i = 0; i < vec_ts_sorted.size(); i++) {

			auto& Scaleblock = vec_ts_sorted[i];
			Scaleblock->reshape(std::vector<int>{1, 1, CUT_MODEL_VISUALFIELD_RAW_INFO_, blockSide[i] * blockSide[i]});
			Scaleblock = tensor_transpose_0132(Scaleblock); // 1, 1, 65, 6400(if) -> 1, 1, 6400, 65
			//YHC
			const int visual_field_nums = Scaleblock->data_shape()[2]; // 1, 1, 6400(if), 65 -> [2]=6400(if)
			const int per_raw_line_length = Scaleblock->data_shape()[3]; // 65
			CHECK_EQ(CUT_MODEL_VISUALFIELD_RAW_INFO_, per_raw_line_length);//per_raw_line_length should EQ CUT_MODEL_VISUALFIELD_RAW_INFO_

			for (int visual_field_idx = 0; visual_field_idx < visual_field_nums; visual_field_idx++) // loop 6400
			{
				//[NOTE!] vehicle yolov8 model cut out is special, score in front, and raw loaction behind !!! (other yolo8 score behind)
				//[NOTE!] 65 = 1 + 64 ,means socre(1) + raw location(4*16) 
				float* uintInfoLinePtrData_ = Scaleblock->mutable_cpu_data() + visual_field_idx * per_raw_line_length;
				float* uintInfoLinePtrData = uintInfoLinePtrData_ + 1; //raw loca

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
				top_line_data[4] = sigmoid_x(uintInfoLinePtrData_[0]);

				top_visual_field_counter++;
			}
		}
		//npy::SAVE_TENSOR_TO_NUMPY(top, "D:/top.npy");

		return top;
	}
}
#endif //_POSTPROCESSING_