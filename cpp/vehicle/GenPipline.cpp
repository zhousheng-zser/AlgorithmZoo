#include "GenPipline.hpp"
//#include "numpyExtensor.hpp"

using namespace glasssix;

GenPipline::GenPipline(std::string model, int device)
{
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
			printf("System environment dnot support using rknn !");
			throw glasssix::exposing::abi_invalid_argument("Invalid model!");
#endif // USE_RKNN
		}
		else if (model_ext == ".exbr" || model_ext == ".phai")
		{
			base_instance_exbr_ = std::make_unique<excalibur::pipeline<float>>(model_name + ".phai", model_name + ".racy", device);
			pipType_ = PipType::excalibur;
		}
		else if (model_ext == ".onnx")
		{
#ifdef USE_ONNXRT
			base_instance_onnx_ = std::make_unique<onx_pipline>(model_name + ".onnx");

			//if (fs::exists(model_name + ".phai"))
			//{
			//	std::cout << "[note] exists corresponding excalibur model(.phai), onnx_pip use common normalization param from " << model_name + ".phai" << std::endl;
			//	base_instance_onnx_->read_exbr_hardcode_params_file(model_name + ".phai");
			//}

			//base_instance_onnx_->set_normalization_param({ {0,0,0},{0.003921568,0.003921568,0.003921568} });
			//base_instance_onnx_->set_normalization_param({ {0,0,0},{0.0078125,0.0078125,0.0078125} });//{104,117,123},{0.0078125,0.0078125,0.0078125} 
			pipType_ = PipType::onnx;
#else
			printf("System environment dnot support using onnxruntime !");
			throw glasssix::exposing::abi_invalid_argument("Invalid model!");
#endif // USE_ONNXRT
		}
	}
}


std::unordered_map<std::string, GenPipline::TensorSptr> GenPipline::forward(cv::Mat img)
{
	std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> rst_map;

	switch (pipType_)
	{
	case GenPipline::PipType::rknn:
#ifdef USE_RKNN
		//<infer speed test>
		//if (infer_time_count > 0) {
		//	std::cout << "\n[infer_time_count] ####" << std::endl;
		//	for (int i = 0; i < 10; i++)
		//		base_instance_rknn_->forward(img.data, { 1, img.rows, img.cols, img.channels() }, RKNN_TENSOR_NHWC);
		//	int loop = 100;
		//	auto timer_start = std::chrono::system_clock::now(); //timer
		//	for (int i = 0; i < loop; i++)
		//		base_instance_rknn_->forward(img.data, { 1, img.rows, img.cols, img.channels() }, RKNN_TENSOR_NHWC);
		//	int det_inference = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - timer_start).count(); //timer
		//	std::cout << "loop " << loop << " avg cost = " << det_inference * 1.f / loop << std::endl;
		//	std::cout << "#######################" << std::endl;
		//	infer_time_count--;
		//}
		//</infer speed test>

		rst_map = base_instance_rknn_->forward(img.data, { 1, img.rows, img.cols, img.channels() }, RKNN_TENSOR_NHWC);
#endif // USE_RKNN
		break;
	case GenPipline::PipType::excalibur:
	{
		std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, img.rows, img.cols, 3}, -1, glasssix::memory::NHWC));
		std::copy(img.data, img.data + img.step[0] * img.rows, input_tensor_u8->mutable_cpu_data());
		input_tensor_u8->convert_order();
		auto input_tensor_f32 = input_tensor_u8 | glasssix::memory::tensor_convert_to<float>; //convenient for exporting tensor.npy file 
		rst_map = base_instance_exbr_->forward(input_tensor_f32);
	}
	break;
	case GenPipline::PipType::onnx:
#ifdef USE_ONNXRT
	{
		std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, img.rows, img.cols, 3}, -1, glasssix::memory::NHWC));
		std::copy(img.data, img.data + img.step[0] * img.rows, input_tensor_u8->mutable_cpu_data());
		input_tensor_u8->convert_order();
		auto input_tensor_f32 = input_tensor_u8 | glasssix::memory::tensor_convert_to<float>;
		rst_map = base_instance_onnx_->forward(input_tensor_f32);
	}
#endif // USE_ONNXRT
	break;
	case GenPipline::PipType::unknown:
		printf("unknown model pipline type.");
		break;
	default:
		break;
	}

	//if (if_use_ppfunc) {
	//	rst_map = ppfunc_(rst_map); // using self-define postprocessing
	//}

	return rst_map;
}

std::string GenPipline::pipTypeInfo() {
	switch (pipType_)
	{
	case GenPipline::PipType::rknn:
		return "rknn";
		break;
	case GenPipline::PipType::excalibur:
		return "excalibur";
		break;
	case GenPipline::PipType::onnx:
		return "onnx";
		break;
	default:
		return "unknown";
		break;
	}
}

int GenPipline::pipTypeID() {
	return static_cast<int>(pipType_);
}

std::string GenPipline::version() {
	return "GenPipline";
}

