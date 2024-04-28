#pragma once
#ifndef _ONNXRT_HPP_
#define _ONNXRT_HPP_
#ifdef USE_ONNXRT

#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <onnxruntime_cxx_api.h>
#include <onnxruntime_c_api.h>
#include "Primitives/tensor.hpp"
#include "Primitives/fmt/format.h"
#include <cassert>

#ifdef BUILD_DEBUG_INFO
//#include "dbg.h"
//#include "../../../test_model/numpy_extensor/numpyExtensor.hpp"
#endif // BUILD_DEBUG_INFO

class ONNXRTPipeline {
	// frame
	Ort::Env env_;
	Ort::Session* session_ptr_ = nullptr;
	Ort::SessionOptions session_options_;
	Ort::AllocatorWithDefaultOptions allocator_;
	std::vector<std::array<float, 3>> normalization_param_;
public:

	size_t inNodes_Num_;
	size_t outNodes_Num_;

	std::vector<std::string> model_inNode_names_;
	std::vector<std::string> model_outNode_names_;

	std::vector<std::vector<int64_t>> model_input_shape_;
	std::vector<std::vector<int64_t>> model_output_shape_;

	std::string version() {
		auto onxrt_ver = std::to_string(ORT_API_VERSION);
		return "onnxruntime_" + onxrt_ver;
	}

	void set_normalization_param(std::vector<std::array<float, 3>> normalization_param) {
		normalization_param_.clear();
		printf("onnx pipeline set normalization param {%f, %f, %f} {%f, %f, %f}\n",
			normalization_param[0][0],
			normalization_param[0][1],
			normalization_param[0][2],
			normalization_param[1][0],
			normalization_param[1][1],
			normalization_param[1][2]);
		auto& onnx_normalization_means = normalization_param[0];
		auto& onnx_normalization_stands = normalization_param[1];
		//dbg(onnx_normalization_means);
		//dbg(onnx_normalization_stands);
		for (auto& arr : normalization_param)
			normalization_param_.push_back(arr);
	}

	void set_normalization_param(std::array<float, 3> means, std::array<float, 3> stands) {
		normalization_param_.clear();
		printf("onnx pipeline set normalization param {%f, %f, %f} {%f, %f, %f}\n",
			means[0], means[1], means[2],
			stands[0], stands[1], stands[2]);
		normalization_param_.push_back(means);
		normalization_param_.push_back(stands);
	}

	void set_normalization_param(float mean, float stand) {
		normalization_param_.clear();
		printf("onnx pipeline set normalization param {%f, %f, %f} {%f, %f, %f}\n",
			mean, mean, mean, stand, stand, stand);
		normalization_param_ = { {mean,mean,mean},{stand,stand,stand} };
	}

	ONNXRTPipeline(std::string model_path, int device = -1)
	{
		//model_path = model_path.substr(0, model_path.find_first_of('.')) + ".onnx";
		session_options_.SetIntraOpNumThreads(4);
		session_options_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL); //ORT_ENABLE_ALL ORT_ENABLE_EXTENDED ORT_ENABLE_BASIC
#ifdef WIN32
		//OrtCUDAProviderOptions options;
		//options.device_id = 0;
		//options.arena_extend_strategy = 0;
		////options.gpu_mem_limit = (size_t)1 * 1024 * 1024 * 1024; //onnxruntime1.8.1, onnxruntime1.9.0
		//options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearch::OrtCudnnConvAlgoSearchHeuristic;
		//options.do_copy_in_default_stream = 1;
		//session_options_.AppendExecutionProvider_CUDA(options);
		//OrtSessionOptionsAppendExecutionProvider_CUDA(session_options_, 0);
		auto model_path_ = std::wstring(model_path.begin(), model_path.end());
#else
		auto model_path_ = model_path;
#endif

		env_ = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "TheNet");
		session_ptr_ = new Ort::Session(env_, model_path_.data(), session_options_);

		//Ort::SessionOptions::AppendExecutionProvider_CUDA(*session_ptr_,0);

		inNodes_Num_ = session_ptr_->GetInputCount();
		outNodes_Num_ = session_ptr_->GetOutputCount();

		for (int i = 0; i < inNodes_Num_; i++)
		{
			auto InputNameAlc = session_ptr_->GetInputNameAllocated(i, allocator_);
			model_inNode_names_.push_back(InputNameAlc.get());
			model_input_shape_.push_back((*session_ptr_).GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape());
		}

		for (int i = 0; i < outNodes_Num_; i++)
		{
			auto OutputNameAlc = session_ptr_->GetOutputNameAllocated(i, allocator_);
			model_outNode_names_.push_back(OutputNameAlc.get());
			model_output_shape_.push_back((*session_ptr_).GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape());
		}
	}

	~ONNXRTPipeline()
	{
		if (session_ptr_ != nullptr) {
			delete session_ptr_;
			session_ptr_ = nullptr;
		}
	}


	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(std::shared_ptr<glasssix::memory::tensor<float>> input_exbtensor) {
		auto exbTensorShape = input_exbtensor->data_shape();
		return forward(input_exbtensor->cpu_data(), input_exbtensor->data_shape(), input_exbtensor->order());
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(cv::Mat image) {
		std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, image.rows, image.cols, 3}, -1, glasssix::memory::NHWC));
#ifdef USE_BMNN
		for (int i = 0; i < image.rows; i++) {
			auto row_ptr = image.ptr<uint8_t>(i);
			std::copy(row_ptr, row_ptr + image.cols * image.channels(), input_tensor_u8->mutable_cpu_data() + i * image.cols * image.channels());
		}
#else
		std::copy(image.data, image.data + image.step[0] * image.rows, input_tensor_u8->mutable_cpu_data());
#endif
		input_tensor_u8->convert_order();
		auto input_exbtensor = input_tensor_u8 | glasssix::memory::tensor_convert_to<float>;

		if (!normalization_param_.empty()) {
			CHECK_EQ(normalization_param_.size(), 2);
			const std::array<float, 3> cls_mean = normalization_param_[0];
			const std::array<float, 3> cls_std = normalization_param_[1];
			int HWStep = input_exbtensor->width() * input_exbtensor->height();
			for (int c = 0; c < input_exbtensor->channels(); c++) {
				auto cpdata = input_exbtensor->mutable_cpu_data() + c * HWStep;
				for (int i = 0; i < HWStep; i++)
					cpdata[i] = (cpdata[i] - cls_mean[c]) * cls_std[c];
			}
		}

		return forward(input_exbtensor->cpu_data(), input_exbtensor->data_shape(), input_exbtensor->order());
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(const float* input_data, std::vector<int> data_shape, int order) {
#ifdef BUILD_DEBUG_INFO
		//npy::SAVE_ARRAY_TO_NUMPY((float*)input_data, data_shape, "D:/onxinpt.npy");
#endif
		std::vector<int64_t> input_Img_shape(data_shape.begin(), data_shape.end());
		int input_count = 1;
		for (int number : input_Img_shape) {
			input_count *= number;
		}

		Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

		Ort::Value input_onxTensor = Ort::Value::CreateTensor<float>(
			memory_info,
			(float*)input_data,
			input_count,
			input_Img_shape.data(),
			input_Img_shape.size()
			);

		std::vector<const char*> model_inNode_names;
		std::vector<const char*> model_outNode_names;
		for (auto& inName : model_inNode_names_) {
			model_inNode_names.push_back(inName.c_str());
		}
		for (auto& ouName : model_outNode_names_) {
			model_outNode_names.push_back(ouName.c_str());
		}

		std::vector<Ort::Value> output_tensors = (*session_ptr_).Run(
			Ort::RunOptions{ nullptr }, // run_options
			model_inNode_names.data(),
			&input_onxTensor,
			model_inNode_names_.size(),
			model_outNode_names.data(),
			model_outNode_names_.size()
		);

		std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> result_map;
		for (int i = 0; i < outNodes_Num_; i++)
		{
			auto shape = output_tensors[i].GetTensorTypeAndShapeInfo().GetShape();
			std::vector<int> ts_shape(shape.begin(), shape.end());
			std::shared_ptr<glasssix::memory::tensor<float>> output_exbtensor;
			output_exbtensor.reset(new glasssix::memory::tensor<float>(ts_shape, -1, glasssix::memory::orderType::NCHW));
			auto data_ptr = output_tensors[i].GetTensorMutableData<float>();
			//std::memcpy(output_exbtensor->mutable_cpu_data(), data_ptr, sizeof(float) * output_exbtensor->count());
			std::copy(data_ptr, data_ptr + output_exbtensor->count(), output_exbtensor->mutable_cpu_data());
			result_map[model_outNode_names_[i]] = output_exbtensor;
		}
		return result_map;
	}

	void read_exbr_hardcode_params_file(std::string_view filepath)
	{
		std::vector<float> vars_;
		std::vector<float> means_;

		std::vector<std::string> output;
		std::ifstream in{ std::string(filepath) };
		std::string temp;
		if (!in.is_open())
		{
			return;
		}
		while (std::getline(in, temp))
		{
			output.push_back(temp);
		}
		in.close();
		//return output;

		decltype(auto) lines = output;
		if (lines.size() <= 0)
		{
			printf("[read phai] Incorrect param file.\n");
			return;
		}
		std::string param_version = split_string_(lines[0], " ")[0];
		if (param_version != "glsv1" && param_version != "7767517")
		{
			printf("[read phai] Incorrect param file version.\n");
			return;
		}

		std::vector<std::vector<std::string>> pipe_param_str_;
		for (size_t i = 2; i < lines.size(); i++)
		{
			if (lines[i].size() <= 0)
			{
				continue;
			}
			auto sarray = split_string_(lines[i], " ");
			std::vector<std::string> useful_array;
			for (size_t j = 0; j < sarray.size(); j++)
			{
				if (sarray[j] != std::string(""))
				{
					useful_array.push_back(sarray[j]);
				}
			}
			pipe_param_str_.push_back(useful_array);
		}

		auto& input_node_param = pipe_param_str_[0];

		std::string specific_params_;

		const int specific_start_id = 4 + atoi(input_node_param[2].c_str()) + atoi(input_node_param[3].c_str());

		for (size_t j = specific_start_id; j < input_node_param.size(); j++)
		{
			specific_params_ += (input_node_param[j] + " ");
		}

		auto attrs = split_string_(specific_params_, " ");

		int c_ = 0;

		for (size_t i = 0; i < attrs.size(); i++)
		{

			if (split_string_(attrs[i], "=")[0] == "2")
			{
				c_ = atoi(split_string_(attrs[i], "=")[1].c_str());
				assert(c_ == 3);
			}
			else if (split_string_(attrs[i], "=")[0] == "3")
			{
				auto means_str = split_string_(split_string_(attrs[i], "=")[1], ",");
				for (size_t j = 0; j < means_str.size(); j++)
				{
					means_.push_back(atof(means_str[j].c_str()));
				}
			}
			else if (split_string_(attrs[i], "=")[0] == "4")
			{
				auto vars_str = split_string_(split_string_(attrs[i], "=")[1], ",");
				for (size_t j = 0; j < vars_str.size(); j++)
				{
					vars_.push_back(atof(vars_str[j].c_str()));
				}
				if (c_ != 1 && c_ != vars_.size())
				{
					if (vars_.size())
						vars_.resize(1);
					for (size_t i = 1; i < c_; i++)
						vars_.push_back(vars_[0]);
				}
			}
		}

		assert(means_.size() == 3);
		assert(vars_.size() == 3);
		std::vector<std::array<float, 3>> normalization_param{ {means_[0],means_[1],means_[2]},{vars_[0],vars_[1],vars_[2]} };
		set_normalization_param(normalization_param);
	}

private:
	static std::vector<std::string> split_string_(const std::string& s, const std::string& c)
	{
		std::vector<std::string> v;
		std::string::size_type pos1, pos2;
		pos2 = s.find(c);
		pos1 = 0;
		while (std::string::npos != pos2)
		{
			v.push_back(s.substr(pos1, pos2 - pos1));

			pos1 = pos2 + c.size();
			pos2 = s.find(c, pos1);
		}
		if (pos1 != s.length())
			v.push_back(s.substr(pos1));
		return v;
	}
};

#endif //!USE_ONNXRT
#endif //!_ONNXRT_HPP_