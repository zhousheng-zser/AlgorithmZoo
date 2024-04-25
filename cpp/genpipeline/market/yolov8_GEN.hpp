#pragma once
#include "../postprocessing/postprocessing_tools.hpp"
#include <vector>

//编译时能确认的后处理函数，直接从此处获取
//运行时才绑定的后处理，则由由该hpp对应的cpp中的后处理封装类注册

namespace {

	struct BSize
	{
		int h;
		int w;
		int count() {
			return h * w;
		}
	};

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

		std::vector<BSize> blockSide; // blockSide maybe  { ..., {80,114}, {40,72}, {20,36} }; or means visual field size
		int INTEGR_ONNX_OUT_LINES = 0;
		for (auto& node : tensor_vector_sorted) {
			auto shape = node->data_shape();
			CHECK_EQ(shape.size(), 4);
			CHECK_EQ(shape[0], 1);
			CHECK_EQ(shape[1], CUT_MODEL_VISUALFIELD_RAW_INFO_);
			blockSide.push_back({ shape[2], shape[3] });
			INTEGR_ONNX_OUT_LINES += shape[2] * shape[3];
		}

		auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{1, 1, INTEGR_ONNX_OUT_LINES, INTEGR_ONNX_OUT_STD_INFO_NUM_}, -1, memory::NCHW);
		float* top_data = top->mutable_cpu_data();
		size_t top_visual_field_counter = 0;

		for (int i = 0; i < tensor_vector_sorted.size(); i++) {
			auto& Scaleblock_origin = tensor_vector_sorted[i];
			Scaleblock_origin->reshape(std::vector<int>{1, 1, CUT_MODEL_VISUALFIELD_RAW_INFO_, blockSide[i].count()});
			auto Scaleblock = PostprocessingTools::tensor_transpose_0132(Scaleblock_origin); // 1, 1, 65, 6400(if) -> 1, 1, 6400, 65 . here 6400 = BSize.count() e.g. BSize={80,80}

			const int visual_field_nums = Scaleblock->data_shape()[2]; // 1, 1, 6400(if), 66 -> [2]=6400(if) e.g.
			const int per_raw_line_length = Scaleblock->data_shape()[3]; // 66 e.g.
			CHECK_EQ(CUT_MODEL_VISUALFIELD_RAW_INFO_, per_raw_line_length);//per_raw_line_length should EQ CUT_MODEL_VISUALFIELD_RAW_INFO_

			for (int visual_field_idx = 0; visual_field_idx < visual_field_nums; visual_field_idx++) // loop BSize.count()
			{
				//[NOTE!] e.g. vehicle yolov8 model cut out is special, score in front, and raw loaction behind !!! (other yolo8 score behind)
				//[NOTE!] 66 = 2 + 64 ,means socre(2) + raw location(4*16) 
				float* unitInfoLinePtrData_ = Scaleblock->mutable_cpu_data() + visual_field_idx * per_raw_line_length; // _ * 66 e.g.

				//SOCRE_ORDER 0: score + location
				//SOCRE_ORDER 1: location + score
				float* unitInfoLinePtrData_raw_loca = SOCRE_ORDER == 0 ? unitInfoLinePtrData_ + SCORE_SIZE_ : unitInfoLinePtrData_;
				float* unitInfoLinePtrData_raw_score = SOCRE_ORDER == 0 ? unitInfoLinePtrData_ : unitInfoLinePtrData_ + RAW_LOCATION_SIZE_;

				alignas(16) float raw_location[4] = { 0.f,0.f,0.f,0.f };
				alignas(16) float softmax_total[4] = { 0.f,0.f,0.f,0.f };

				// pre-softmax (pre-softmax + after-softmax = softmax)
				for (int exp_i = 0; exp_i < 64; exp_i++) // per conv_group kernel = {0,1,2,3,...,15}, len 16
				{
					unitInfoLinePtrData_raw_loca[exp_i] = exp(unitInfoLinePtrData_raw_loca[exp_i]);
					softmax_total[exp_i / 16] += unitInfoLinePtrData_raw_loca[exp_i];
				}

				// after-softmax and convolution op
				for (int exp_i = 0; exp_i < 64; exp_i++) // per conv_group kernel = {0,1,2,3,...,15}, len 16
				{
					// exp_i / 16 : loop div 0...0, 1..1, 2..2, 3..3 per length 16
					// exp_i % 16 : loop mul 0,1,2..15,  0,1,2..15,  0,1,2..15,  0,1,2..15
					unitInfoLinePtrData_raw_loca[exp_i] /= softmax_total[exp_i / 16]; // after-softmax operation
					raw_location[exp_i / 16] += unitInfoLinePtrData_raw_loca[exp_i] * (exp_i % 16);// convolution
				}

				/*
				* visual_field_idx % blockSide[i].w : loop 0 -> w
				* visual_field_idx / blockSide[i].w : loop 0,0,0,0...(w times),1,1,1...(w times)...,h-1,h-1...(w times)
				* {0,..}->{h-1,..} because of visual_field_nums / w == h,
				* so add to h-1 depend on visual_field_nums, or mean (to h-1) from implied automatic computation
				*/
				raw_location[0] = 0.5 + visual_field_idx % blockSide[i].w - raw_location[0];
				raw_location[1] = 0.5 + visual_field_idx / blockSide[i].w - raw_location[1];
				raw_location[2] = 0.5 + visual_field_idx % blockSide[i].w + raw_location[2];
				raw_location[3] = 0.5 + visual_field_idx / blockSide[i].w + raw_location[3];

				float loaction_0 = (raw_location[2] + raw_location[0]) / 2; // center
				float loaction_1 = (raw_location[3] + raw_location[1]) / 2; // center
				float loaction_2 = raw_location[2] - raw_location[0];
				float loaction_3 = raw_location[3] - raw_location[1];

				auto top_line_data = top_data + top_visual_field_counter * INTEGR_ONNX_OUT_STD_INFO_NUM_;
				top_line_data[0] = loaction_0 / blockSide[i].w;
				top_line_data[1] = loaction_1 / blockSide[i].h;
				top_line_data[2] = loaction_2 / blockSide[i].w;
				top_line_data[3] = loaction_3 / blockSide[i].h;

				for (int sc_i = 0; sc_i < SCORE_SIZE_; sc_i++) {
					top_line_data[4 + sc_i] = PostprocessingTools::sigmoid_x(unitInfoLinePtrData_raw_score[sc_i]);
				}
				top_visual_field_counter++;
			}
		}
		return top;
	}


	template<int SCORE_LEN, bool SOCRE_ORDER>
	static inline TensorSptr yolov8_gen_complement_fast_(std::vector<TensorSptr>& tensor_vector_sorted)
	{
		// VF means VISUAL FIELD
		static constexpr int RAW_LOCATION_SIZE_ = 64;
		static constexpr int SCORE_SIZE_ = SCORE_LEN;
		static constexpr int INTEGR_ONNX_OUT_STD_INFO_NUM_ = SCORE_SIZE_ + 4;
		static constexpr int CUT_MODEL_VISUALFIELD_RAW_INFO_ = SCORE_SIZE_ + RAW_LOCATION_SIZE_; // include 4*BBoxlocaInfo(16 usually) + scores (64+N)

		std::vector<BSize> blockSide; // blockSide maybe  { ..., {80,114}, {40,72}, {20,36} }; or means visual field size
		int INTEGR_ONNX_OUT_LINES = 0;
		for (auto& node : tensor_vector_sorted) {
			auto shape = node->data_shape();
			CHECK_EQ(shape.size(), 4);
			CHECK_EQ(shape[0], 1);
			CHECK_EQ(shape[1], CUT_MODEL_VISUALFIELD_RAW_INFO_);
			blockSide.push_back({ shape[2], shape[3] });
			INTEGR_ONNX_OUT_LINES += shape[2] * shape[3];
		}

		struct VisualFieldPack {
			size_t block_id;
			size_t idx;
			float l0 = 0;
			float l1 = 0;
			float l2 = 0;
			float l3 = 0;
			std::array<float, SCORE_SIZE_> scores{ 0 };
		};
		std::vector<VisualFieldPack> visual_field_vec;

		for (int i = 0; i < tensor_vector_sorted.size(); i++) {
			std::vector<VisualFieldPack> visual_field_vec_theblock;

			auto& Scaleblock_origin = tensor_vector_sorted[i];
			Scaleblock_origin->reshape(std::vector<int>{1, 1, CUT_MODEL_VISUALFIELD_RAW_INFO_, blockSide[i].count()});
			auto Scaleblock_origin_data = Scaleblock_origin->mutable_cpu_data();
			auto& Scaleblock = Scaleblock_origin;

			const int visual_field_nums = Scaleblock->data_shape()[3]; //e.g. 6400
			const int per_raw_line_length = Scaleblock->data_shape()[2]; //e.g. 66


			//SOCRE_ORDER 0: score + location
			//SOCRE_ORDER 1: location + score
			//one group length = visual_field_nums;
			float* group_locat_begin = SOCRE_ORDER == 0 ? Scaleblock_origin_data + SCORE_SIZE_ * visual_field_nums : Scaleblock_origin_data;
			float* group_score_begin = SOCRE_ORDER == 0 ? Scaleblock_origin_data : Scaleblock_origin_data + RAW_LOCATION_SIZE_ * visual_field_nums;
			
			constexpr float SpeedMinAskConf_ = 0.01;

			//find block valid visual fields
			for (int vf_idx = 0; vf_idx < visual_field_nums; vf_idx++) {
				VisualFieldPack vfp;
				vfp.block_id = i;
				vfp.idx = vf_idx;

				bool is_usefulVF = false;
				for (auto categ_group_id = 0; categ_group_id < SCORE_SIZE_; categ_group_id++) {
					float* categ_group_begin = group_score_begin + categ_group_id * visual_field_nums;

					float vf_categ_score = PostprocessingTools::sigmoid_x(categ_group_begin[vf_idx]);
					vfp.scores[categ_group_id] = vf_categ_score;

					if (SpeedMinAskConf_ < vf_categ_score) {
						is_usefulVF = true;
					}
				}

				if (is_usefulVF) {
					visual_field_vec_theblock.emplace_back(vfp);
				}
			}
			//calcu visual fields` location
			for (auto& vfp : visual_field_vec_theblock) {
				alignas(16) float raw_location[4] = { 0.f,0.f,0.f,0.f };
				alignas(16) float softmax_total[4] = { 0.f,0.f,0.f,0.f };

				// pre-softmax (pre-softmax + after-softmax = softmax)
				for (int exp_i = 0; exp_i < 64; exp_i++) // per conv_group kernel = {0,1,2,3,...,15}, len 16
				{
					float* unitInfoLinePtrData_raw_loca_exp_i = group_locat_begin + exp_i * visual_field_nums + vfp.idx;
					*unitInfoLinePtrData_raw_loca_exp_i = exp(*unitInfoLinePtrData_raw_loca_exp_i);
					softmax_total[exp_i / 16] += *unitInfoLinePtrData_raw_loca_exp_i;
				}

				// after-softmax and convolution op
				for (int exp_i = 0; exp_i < 64; exp_i++) // per conv_group kernel = {0,1,2,3,...,15}, len 16
				{
					float* unitInfoLinePtrData_raw_loca_exp_i = group_locat_begin + exp_i * visual_field_nums + vfp.idx;
					// exp_i / 16 : loop div 0...0, 1..1, 2..2, 3..3 per length 16
					// exp_i % 16 : loop mul 0,1,2..15,  0,1,2..15,  0,1,2..15,  0,1,2..15
					*unitInfoLinePtrData_raw_loca_exp_i /= softmax_total[exp_i / 16]; // after-softmax operation
					raw_location[exp_i / 16] += *unitInfoLinePtrData_raw_loca_exp_i * (exp_i % 16);// convolution
				}

				/*
				* visual_field_idx % blockSide[i].w : loop 0 -> w
				* visual_field_idx / blockSide[i].w : loop 0,0,0,0...(w times),1,1,1...(w times)...,h-1,h-1...(w times)
				* {0,..}->{h-1,..} because of visual_field_nums / w == h,
				* so add to h-1 depend on visual_field_nums, or mean (to h-1) from implied automatic computation
				*/
				raw_location[0] = 0.5 + vfp.idx % blockSide[i].w - raw_location[0];
				raw_location[1] = 0.5 + vfp.idx / blockSide[i].w - raw_location[1];
				raw_location[2] = 0.5 + vfp.idx % blockSide[i].w + raw_location[2];
				raw_location[3] = 0.5 + vfp.idx / blockSide[i].w + raw_location[3];

				float loaction_0 = (raw_location[2] + raw_location[0]) / 2; // center
				float loaction_1 = (raw_location[3] + raw_location[1]) / 2; // center
				float loaction_2 = raw_location[2] - raw_location[0];
				float loaction_3 = raw_location[3] - raw_location[1];

				vfp.l0 = loaction_0 / blockSide[i].w;
				vfp.l1 = loaction_1 / blockSide[i].h;
				vfp.l2 = loaction_2 / blockSide[i].w;
				vfp.l3 = loaction_3 / blockSide[i].h;
			}
			visual_field_vec.insert(visual_field_vec.end(), visual_field_vec_theblock.begin(), visual_field_vec_theblock.end());
		}
		const int vfsize = visual_field_vec.size();
		auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{1, 1, vfsize, INTEGR_ONNX_OUT_STD_INFO_NUM_}, -1, memory::NCHW);
		float* top_data = top->mutable_cpu_data();
		for (size_t visual_field_vec_idx = 0; visual_field_vec_idx < vfsize; visual_field_vec_idx++) {
			auto& valid_visual_field = visual_field_vec[visual_field_vec_idx];
			auto vf_top_data = top->mutable_cpu_data() + visual_field_vec_idx * INTEGR_ONNX_OUT_STD_INFO_NUM_;
			//copy location
			vf_top_data[0] = valid_visual_field.l0;
			vf_top_data[1] = valid_visual_field.l1;
			vf_top_data[2] = valid_visual_field.l2;
			vf_top_data[3] = valid_visual_field.l3;
			for (int sc_i = 0; sc_i < SCORE_SIZE_; sc_i++) {
				vf_top_data[4 + sc_i] = valid_visual_field.scores[sc_i];
			}
		}

		return top;
	}
}

// SCORE_LEN: class num
// SOCRE_ORDER: 0 means raw out = [score + raw_location], 1 means raw out = [raw_location + score]
// using template for packing function type (ref PostprocessingFunction = std::function<tensorMap(tensorMap)>) conveniently
template<int SCORE_LEN = 1, bool SOCRE_ORDER = true>
static inline std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> yolov8_GEN(std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& input_tensor_map)
{
	auto det_rst_vec = PostprocessingTools::sort_yolo_rst(input_tensor_map);
	//TensorSptr concat_tensor_ptr = yolov8_gen_complement_<SCORE_LEN, SOCRE_ORDER>(det_rst_vec);
	TensorSptr concat_tensor_ptr = yolov8_gen_complement_fast_<SCORE_LEN, SOCRE_ORDER>(det_rst_vec);

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> postprocessing_rstmap;
	postprocessing_rstmap.try_emplace("yolo8_cat", concat_tensor_ptr);
	return postprocessing_rstmap;
}