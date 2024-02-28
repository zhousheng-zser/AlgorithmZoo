#pragma once
#include "postprocessing_register.hpp"
#include <vector>

namespace glasssix::pump_weld {

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
			return !((countA > countB) ^ order);
			});
		return outRst;
	}

	static inline float sigmoid_x(float x)
	{
		return static_cast<float>(1.f / (1.f + exp(-x)));
	}

	static inline TensorSptr yolov8_complement_weld(std::vector<TensorSptr>& vec_ts_sorted)
	{
		// VF means VISUAL FIELD
		static constexpr int RAW_LOCATION_SIZE_ = 64; // include 4*BBoxlocaInfo(16 usually) + scores (64+N)
		static constexpr int SCORE_SIZE_ = 2; // include 4*BBoxlocaInfo(16 usually) + scores (64+N)
		static constexpr int INTEGR_ONNX_OUT_STD_INFO_NUM_ = SCORE_SIZE_ + 4;
		static constexpr int CUT_MODEL_VISUALFIELD_RAW_INFO_ = SCORE_SIZE_ + RAW_LOCATION_SIZE_; // include 4*BBoxlocaInfo(16 usually) + scores (64+N)
		struct BSize
		{
			int h;
			int w;
			int count() {
				return h * w;
			}
		};

		std::vector<BSize> blockSide; // blockSide maybe  { ..., 80, 40, 20 }; or means visual field size
		int INTEGR_ONNX_OUT_LINES = 0;
		for (auto& node : vec_ts_sorted) {
			auto shape = node->data_shape();
			CHECK_EQ(shape.size(), 4);
			CHECK_EQ(shape[0], 1);
			CHECK_EQ(shape[1], CUT_MODEL_VISUALFIELD_RAW_INFO_);
			blockSide.push_back({ shape[2],shape[3] });
			INTEGR_ONNX_OUT_LINES += shape[2] * shape[3];
		}

		auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{1, 1, INTEGR_ONNX_OUT_LINES, INTEGR_ONNX_OUT_STD_INFO_NUM_}, -1, memory::NCHW);
		float* top_data = top->mutable_cpu_data();
		size_t top_visual_field_counter = 0;

		for (int i = 0; i < vec_ts_sorted.size(); i++) {

			auto& Scaleblock = vec_ts_sorted[i];
			Scaleblock->reshape(std::vector<int>{1, 1, CUT_MODEL_VISUALFIELD_RAW_INFO_, blockSide[i].count()});
			Scaleblock = tensor_transpose_0132(Scaleblock); // 1, 1, 65, 6400(if) -> 1, 1, 6400, 65
			//YHC
			const int visual_field_nums = Scaleblock->data_shape()[2]; // 1, 1, 6400(if), 66 -> [2]=6400(if)
			const int per_raw_line_length = Scaleblock->data_shape()[3]; // 66
			CHECK_EQ(CUT_MODEL_VISUALFIELD_RAW_INFO_, per_raw_line_length);//per_raw_line_length should EQ CUT_MODEL_VISUALFIELD_RAW_INFO_

			for (int visual_field_idx = 0; visual_field_idx < visual_field_nums; visual_field_idx++) // loop 6400
			{
				//[NOTE!] vehicle yolov8 model cut out is special, score in front, and raw loaction behind !!! (other yolo8 score behind)
				//[NOTE!] 66 = 2 + 64 ,means socre(2) + raw location(4*16) 
				float* uintInfoLinePtrData_ = Scaleblock->mutable_cpu_data() + visual_field_idx * per_raw_line_length; // _ * 66
				float* uintInfoLinePtrData = uintInfoLinePtrData_ + SCORE_SIZE_; //raw loca

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

				raw_location[0] = 0.5 + visual_field_idx % blockSide[i].w - raw_location[0];
				raw_location[1] = 0.5 + visual_field_idx / blockSide[i].h - raw_location[1];

				raw_location[2] = 0.5 + visual_field_idx % blockSide[i].w + raw_location[2];
				raw_location[3] = 0.5 + visual_field_idx / blockSide[i].h + raw_location[3];

				float loaction_0 = (raw_location[2] + raw_location[0]) / 2;
				float loaction_1 = (raw_location[3] + raw_location[1]) / 2;

				float loaction_2 = raw_location[2] - raw_location[0];
				float loaction_3 = raw_location[3] - raw_location[1];

				loaction_0 = loaction_0 / blockSide[i].w; // Equivalent operation for * mul_{8,16,32} / div_{360} 
				loaction_1 = loaction_1 / blockSide[i].h;
				loaction_2 = loaction_2 / blockSide[i].w;
				loaction_3 = loaction_3 / blockSide[i].h;

				auto top_line_data = top_data + top_visual_field_counter * INTEGR_ONNX_OUT_STD_INFO_NUM_;
				top_line_data[0] = loaction_0;
				top_line_data[1] = loaction_1;
				top_line_data[2] = loaction_2;
				top_line_data[3] = loaction_3;
				for (int sc_i = 0; sc_i < SCORE_SIZE_; sc_i++) {
#ifdef BUILD_DEBUG_INFO
					int check_val = sigmoid_x(uintInfoLinePtrData_[sc_i]);
#endif // BUILD_DEBUG_INFO
					top_line_data[4 + sc_i] = sigmoid_x(uintInfoLinePtrData_[sc_i]);
				}

				top_visual_field_counter++;
			}
		}
		return top;
	}

	static std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> weld_concat(std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>>& input_tensor_map)
	{
		auto det_rst_vec = sort_yolo_rst(input_tensor_map);
		TensorSptr concat_tensor_ptr = yolov8_complement_weld(det_rst_vec);
		std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> postprocessing_rstmap;
		postprocessing_rstmap.try_emplace("inteyolo8", concat_tensor_ptr); // complement result 
		return postprocessing_rstmap;
	}

}