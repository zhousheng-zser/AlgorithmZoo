#pragma once
#include "postprocessing_tools.hpp"
#include <vector>

//编译时能确认的后处理函数，直接从此处获取
//运行时才绑定的后处理，则由由该hpp对应的cpp中的后处理封装类注册

namespace {
	/// native code
	/// SOCRE_ORDER: 0 means raw out = [score + raw_location], 1 means raw out = [raw_location + score]
	/// SCORE_LEN: (category) scores num
	template<int SCORE_LEN, bool SOCRE_ORDER>
	static inline TensorSptr yolov8_gen_complement_(std::vector<TensorSptr>& tensor_vector_sorted)
	{
		// VF means VISUAL FIELD
		static constexpr int RAW_LOCATION_SIZE_ = 64;
		static constexpr int SCORE_SIZE_ = SCORE_LEN;
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
		for (auto& node : tensor_vector_sorted) {
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

		for (int i = 0; i < tensor_vector_sorted.size(); i++) {
			auto& Scaleblock_origin = tensor_vector_sorted[i];
			Scaleblock_origin->reshape(std::vector<int>{1, 1, CUT_MODEL_VISUALFIELD_RAW_INFO_, blockSide[i].count()});
			auto Scaleblock = tensor_transpose_0132(Scaleblock_origin); // 1, 1, 65, 6400(if) -> 1, 1, 6400, 65

			const int visual_field_nums = Scaleblock->data_shape()[2]; // 1, 1, 6400(if), 66 -> [2]=6400(if)
			const int per_raw_line_length = Scaleblock->data_shape()[3]; // 66
			CHECK_EQ(CUT_MODEL_VISUALFIELD_RAW_INFO_, per_raw_line_length);//per_raw_line_length should EQ CUT_MODEL_VISUALFIELD_RAW_INFO_

			for (int visual_field_idx = 0; visual_field_idx < visual_field_nums; visual_field_idx++) // loop 6400
			{
				//[NOTE!] vehicle yolov8 model cut out is special, score in front, and raw loaction behind !!! (other yolo8 score behind)
				//[NOTE!] 66 = 2 + 64 ,means socre(2) + raw location(4*16) 
				float* uintInfoLinePtrData_ = Scaleblock->mutable_cpu_data() + visual_field_idx * per_raw_line_length; // _ * 66

				//SOCRE_ORDER 0: score + location
				//SOCRE_ORDER 1: location + score
				float* uintInfoLinePtrData_raw_loca = SOCRE_ORDER == 0 ? uintInfoLinePtrData_ + SCORE_SIZE_ : uintInfoLinePtrData_;
				float* uintInfoLinePtrData_raw_score = SOCRE_ORDER == 0 ? uintInfoLinePtrData_ : uintInfoLinePtrData_ + RAW_LOCATION_SIZE_;

				float raw_location[4] = { 0.f,0.f,0.f,0.f };
				float softmax_total[4] = { 0.f,0.f,0.f,0.f };

				// softmax
				for (int exp_i = 0; exp_i < 64; exp_i++) // per conv_group kernel = {0,1,2,3,...,15}, len 16
				{
					uintInfoLinePtrData_raw_loca[exp_i] = exp(uintInfoLinePtrData_raw_loca[exp_i]);
					softmax_total[exp_i / 16] += uintInfoLinePtrData_raw_loca[exp_i];
				}

				// convolution with after-softmax
				for (int exp_i = 0; exp_i < 64; exp_i++) // per conv_group kernel = {0,1,2,3,...,15}, len 16
				{
					// exp_i / 16 : loop div 0...0, 1..1, 2..2, 3..3 per length 16
					// exp_i % 16 : loop mul 0,1,2..15,  0,1,2..15,  0,1,2..15,  0,1,2..15
					uintInfoLinePtrData_raw_loca[exp_i] /= softmax_total[exp_i / 16]; // after-softmax operation
					raw_location[exp_i / 16] += uintInfoLinePtrData_raw_loca[exp_i] * (exp_i % 16);// convolution
				}

				raw_location[0] = 0.5 + visual_field_idx % blockSide[i].w - raw_location[0];
				raw_location[1] = 0.5 + visual_field_idx / blockSide[i].h - raw_location[1];
				raw_location[2] = 0.5 + visual_field_idx % blockSide[i].w + raw_location[2];
				raw_location[3] = 0.5 + visual_field_idx / blockSide[i].h + raw_location[3];

				float loaction_0 = (raw_location[2] + raw_location[0]) / 2;
				float loaction_1 = (raw_location[3] + raw_location[1]) / 2;
				float loaction_2 = raw_location[2] - raw_location[0];
				float loaction_3 = raw_location[3] - raw_location[1];

				auto top_line_data = top_data + top_visual_field_counter * INTEGR_ONNX_OUT_STD_INFO_NUM_;
				top_line_data[0] = loaction_0 / blockSide[i].w;
				top_line_data[1] = loaction_1 / blockSide[i].h;
				top_line_data[2] = loaction_2 / blockSide[i].w;
				top_line_data[3] = loaction_3 / blockSide[i].h;

				for (int sc_i = 0; sc_i < SCORE_SIZE_; sc_i++) {
					top_line_data[4 + sc_i] = sigmoid_x(uintInfoLinePtrData_raw_score[sc_i]);
				}
				top_visual_field_counter++;
			}
		}
		return top;
	}
}

// SOCRE_ORDER: 0 means raw out = [score + raw_location], 1 means raw out = [raw_location + score]
// using template for packing function type (ref PostprocessingFunction = std::function<tensorMap(tensorMap)>) conveniently
template<int SCORE_LEN = 1, bool SOCRE_ORDER = true>
static inline std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> yolov8_GEN(std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& input_tensor_map)
{
	auto det_rst_vec = sort_yolo_rst(input_tensor_map);
	TensorSptr concat_tensor_ptr = yolov8_gen_complement_<SCORE_LEN, SOCRE_ORDER>(det_rst_vec);

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> postprocessing_rstmap;
	postprocessing_rstmap.try_emplace("concat_yolo8", concat_tensor_ptr);
	return postprocessing_rstmap;
}
