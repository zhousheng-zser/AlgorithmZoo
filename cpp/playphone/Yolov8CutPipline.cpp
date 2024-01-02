#include "Yolov8CutPipline.hpp"
#include <algorithm>
#include <numeric>
#include <math.h>
#include "detect_code_internal.hpp"

namespace glasssix::playphone
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

	RknnYolov8Pipline::TensorSptr RknnYolov8Pipline::yolov8_concat(std::vector<TensorSptr>& vec_ts_rstSort)
	{
		static constexpr int blockSide[3] = { 80, 40, 20 }; //ScaleSteps[3][2] = { {80, 80}, {40, 40}, {20, 20} };
		//CHECK_EQ(3, vec_ts_rstSort.size());

		const int INTEGRATED_ONNX_OUT_UINTLINE_NUM_ = 5;
		auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{1, 1, 8400, INTEGRATED_ONNX_OUT_UINTLINE_NUM_}, -1, memory::NCHW);
		float* top_data = top->mutable_cpu_data();
		size_t top_line_counter = 0;

		for (int i = 0; i < 3; i++) {

			auto& Scaleblock = vec_ts_rstSort[i];
			Scaleblock->reshape(std::vector<int>{1, 1, 65, blockSide[i] * blockSide[i]});
			Scaleblock = tensor_transpose_0132(Scaleblock); // 1, 1, 65, 6400 -> 1, 1, 6400, 65

			int line_num = Scaleblock->data_shape()[2]; // 6400 + 1600 + 400 = 8400
			int per_line_length = Scaleblock->data_shape()[3]; // 65
			for (int line = 0; line < line_num; line++) // loop 6400 |
			{
				float* uintInfoLinePtrData = Scaleblock->mutable_cpu_data() + line * per_line_length;
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

				auto top_line_data = top_data + top_line_counter * INTEGRATED_ONNX_OUT_UINTLINE_NUM_;
				top_line_data[0] = loaction_0;
				top_line_data[1] = loaction_1;
				top_line_data[2] = loaction_2;
				top_line_data[3] = loaction_3;
				top_line_data[4] = uintInfoLinePtrData[64];
				top_line_counter++;
			}
		}
		return top;
	}

	std::string RknnYolov8Pipline::version() {
		switch (pipType_)
		{
		case PipType::rknn:
#ifdef USE_RKNN
			return base_instance_rknn_->version(); 
#endif // USE_RKNN			
		case PipType::excalibur:
			return base_instance_exbr_->version();
		default:
			return "pipline";
		}
	}

	// constructor 
	RknnYolov8Pipline::RknnYolov8Pipline(std::string model, int device) {
		if (model.size() > 5)
		{
			auto model_name = model.substr(0, model.find_last_of('.'));
			auto model_ext = model.substr(model.find_last_of('.'));
			if (model_ext == ".rknn")
			{
#ifdef USE_RKNN
				std::vector<std::string> rkn_phai;
				base_instance_rknn_ = std::make_unique<rknnwrapper::rknn_wrapper>(rkn_phai, model);
				pipType_ = PipType::rknn;
#else
				throw glasssix::exposing::abi_invalid_argument("Invalid model!");
#endif // USE_RKNN
			}
			else if (model_ext == ".exbr" || model_ext == ".phai")
			{
				base_instance_exbr_ = std::make_unique<excalibur::pipeline<float>>(model_name + ".phai", model_name + ".racy", device);
				pipType_ = PipType::excalibur;
			}

		}
	}

	std::vector<RknnYolov8Pipline::TensorSptr> RknnYolov8Pipline::sort_yolo_rst(const std::unordered_map<std::string, TensorSptr>& result) {
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



	std::unordered_map<std::string, RknnYolov8Pipline::TensorSptr> RknnYolov8Pipline::forward(cv::Mat img)
	{
		std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> rst_map;
		switch (pipType_)
		{
		case PipType::rknn:
#ifdef USE_RKNN
			rst_map = base_instance_rknn_->forward(img.data, { 1, img.rows, img.cols, img.channels() }, RKNN_TENSOR_NHWC);
#endif // USE_RKNN
			break;
		case PipType::excalibur:
		{
			std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, img.rows, img.cols, 3}, -1, glasssix::memory::NHWC));
			std::copy(img.data, img.data + img.step[0] * img.rows, input_tensor_u8->mutable_cpu_data());
			input_tensor_u8->convert_order();
			auto input_tensor_f32 = input_tensor_u8 | glasssix::memory::tensor_convert_to<float>; //convenient for exporting tensor.npy file 
			rst_map = base_instance_exbr_->forward(input_tensor_f32);
		}
		break;

		default:
			break;
		}

		// postprocessing
		if (rst_map.size() == 1) {
			rst_map.begin()->second = tensor_transpose_0132(rst_map.begin()->second);
			return rst_map;
		}
		else {
			auto det_rst_vec = sort_yolo_rst(rst_map);

			TensorSptr concat_tensor_ptr = yolov8_concat(det_rst_vec);

			std::unordered_map<std::string, TensorSptr> result_map;
			result_map.try_emplace("concat_output", concat_tensor_ptr); // complement result 
			return result_map;
		}
	}

	std::vector<ObjBox> RknnYolov8Pipline::detect(cv::Mat image, cv::Point image_start, float conf_thres, float iou_thres) {
		constexpr int imageResize = 640;

		auto letter_img = playphone_preprocess(image, imageResize);

		std::vector<ObjBox> obj_list;

		auto det_rst_map = forward(letter_img);
		auto tensor_out = det_rst_map.begin()->second;

		int targetnum = tensor_out->height();
		int infonum = tensor_out->width();
		for (size_t idx = 0; idx < targetnum; idx++) {
			float* pdata = tensor_out->mutable_cpu_data() + idx * infonum;
			float conf = pdata[4];

			if (conf > conf_thres)
			{
				//ObjBox phonebox(pdata[0], pdata[1], pdata[2], pdata[3], conf); // no-cut model
				ObjBox phonebox(pdata[0] * 640, pdata[1] * 640, pdata[2] * 640, pdata[3] * 640, conf);
				obj_list.push_back(phonebox);
			}
		}

		int pad = std::abs(image.cols - image.rows) / 2;
		bool is_vertical_pad = image.cols > image.rows;
		float mapping_ratio = static_cast<float>(std::max(image.cols, image.rows)) / imageResize;

		for (auto& bbox : obj_list) {
			bbox.mul_ratio(mapping_ratio);
			if (is_vertical_pad) {
				bbox.ymin -= pad;
				bbox.ymax -= pad;
			}
			else {
				bbox.xmin -= pad;
				bbox.xmax -= pad;
			}

			bbox.add(image_start);
		}

		NMS_CPU(obj_list, iou_thres);
		return obj_list;
	}



}