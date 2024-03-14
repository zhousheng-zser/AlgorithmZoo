//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// SOPHON-DEMO is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#ifndef _BMNNPIPELINE_HPP_
#define _BMNNPIPELINE_HPP_

#include "bm_wrapper.hpp"
#include "bmnn_utils.h"
#include <iostream>
#include <string>
#include <vector>
#include "opencv2/opencv.hpp"
#include "tensor.hpp"
#include <numeric>

class BMNNPipeline {
	std::shared_ptr<BMNNContext> m_bmContext;
	std::shared_ptr<BMNNNetwork> m_bmNetwork;
	std::shared_ptr<BMNNTensor>  m_input_tensor;

	// model info 
	int m_net_h;
	int m_net_w;
	int m_num_channels;
	int m_dev_id = 0;
	int max_batch;
	int output_num;
	float input_scale;
	bm_tensor_t input_tensor;

public:
	std::vector<std::string> output_names;

private:
	float* m_input_float;
	int8_t* m_input_int8;
	int m_input_count;

	cv::Mat m_mean;
	cv::Mat m_std;

	std::vector<float> mean_;
	std::vector<float> std_;

	bool convertRGB_ = false;


	void wrapInputLayer(std::vector<cv::Mat>* input_channels, int batch_id) {
		int h = m_net_h;
		int w = m_net_w;

		//init input_channels
		if (m_input_tensor->get_dtype() == BM_INT8) {
			int8_t* channel_base = m_input_int8;
			channel_base += h * w * m_num_channels * batch_id;
			for (int i = 0; i < m_num_channels; i++) {
				cv::Mat channel(h, w, CV_8SC1, channel_base);
				input_channels->push_back(channel);
				channel_base += h * w;
			}
		}
		else {
			float* channel_base = m_input_float;
			channel_base += h * w * m_num_channels * batch_id;
			for (int i = 0; i < m_num_channels; i++) {
				cv::Mat channel(h, w, CV_32FC1, channel_base);
				input_channels->push_back(channel);
				channel_base += h * w;
			}
		}
	}

	int pre_process(std::vector<cv::Mat>& images) {
		//Safety check.
		assert(images.size() <= max_batch);

		//1. Preprocess input images in host memory.
		for (int i = 0; i < max_batch; i++) {
			std::vector<cv::Mat> input_channels;
			wrapInputLayer(&input_channels, i);
			if (i < images.size())
				pre_process_image(images[i], &input_channels);
			else {
				cv::Mat tmp = cv::Mat::zeros(m_net_h, m_net_w, CV_32FC3);
				pre_process_image(tmp, &input_channels);
			}
		}
		//2. Attach to input tensor.
		if (m_input_tensor->get_dtype() == BM_INT8) {
			// dbg("m_input_int8");
			bm_memcpy_s2d(m_bmContext->handle(), input_tensor.device_mem, (void*)m_input_int8);
		}
		else {

			// dbg("m_input_float");
			// std::vector<long unsigned int> m_input_shape{1,3,640,640};
			// std::cout<<"#### save m_input_float"<<std::endl;
			// npy::SAVE_ARRAY_TO_NUMPY(m_input_float,m_input_shape,"m_input.npy");
			// auto test_model_onx_pede_input = npy::LoadNpy("pede_input.npy");
			// pede_input_compare(test_model_onx_pede_input->mutable_cpu_data(),m_input_float,test_model_onx_pede_input->count());
			// dbg(m_input_count);
			// bm_memcpy_s2d(m_bmContext->handle(), input_tensor.device_mem, (void *)test_model_onx_pede_input->mutable_cpu_data());

			bm_memcpy_s2d(m_bmContext->handle(), input_tensor.device_mem, (void*)m_input_float);
		}

		return 0;
	}

	void pre_process_image(const cv::Mat& img, std::vector<cv::Mat>* input_channels) {
		/* Convert the input image to the input image format of the network. */
		cv::Mat sample = img;
		cv::Mat sample_resized(m_net_h, m_net_w, CV_8UC3, cv::SophonDevice(m_dev_id));
		if (sample.size() != cv::Size(m_net_w, m_net_h)) {
			cv::resize(sample, sample_resized, cv::Size(m_net_w, m_net_h));
		}
		else {
			sample_resized = sample;
		}
		cv::Mat sample_resized_rgb(cv::SophonDevice(this->m_dev_id));
		cv::cvtColor(sample_resized, sample_resized_rgb, cv::COLOR_BGR2RGB);

		if (convertRGB_ == false) {
			cv::cvtColor(sample_resized_rgb, sample_resized_rgb, cv::COLOR_BGR2RGB);
		}

		cv::Mat sample_float(cv::SophonDevice(this->m_dev_id));
		sample_resized_rgb.convertTo(sample_float, CV_32FC3);

		cv::Mat sample_normalized_0(cv::SophonDevice(this->m_dev_id));
		cv::Mat sample_normalized(cv::SophonDevice(this->m_dev_id));

		cv::add(sample_float, m_mean, sample_normalized_0);
		cv::multiply(sample_normalized_0, m_std, sample_normalized);


		// /*note: int8 in convert need mul input_scale*/
		if (m_input_tensor->get_dtype() == BM_INT8) {
			cv::Mat sample_int8(cv::SophonDevice(this->m_dev_id));
			sample_normalized.convertTo(sample_int8, CV_8SC1, input_scale);
			cv::split(sample_int8, *input_channels);
		}
		else {
			cv::Mat sample_fp32(cv::SophonDevice(this->m_dev_id));
			sample_normalized.convertTo(sample_fp32, CV_32FC3, input_scale);
			cv::split(sample_fp32, *input_channels);
		}
	}

public:
	BMNNPipeline(std::shared_ptr<BMNNContext> context, int dev_id) : m_bmContext(context), m_dev_id(dev_id) {
		std::cout << "BMNNPipeline create bm_context" << std::endl;
	}

	BMNNPipeline(std::string modelPath, int dev_id) {
		std::cout << "BMNNPipeline create bm_context" << std::endl;
		BMNNHandlePtr handle = std::make_shared<BMNNHandle>(m_dev_id);
		m_bmContext = std::make_shared<BMNNContext>(handle, modelPath.c_str());

		Init();
	}

	~BMNNPipeline() {
		std::cout << "BMNNPipeline delete bm_context" << std::endl;
		bm_free_device(m_bmContext->handle(), input_tensor.device_mem);
		if (m_input_tensor->get_dtype() == BM_INT8) {
			delete[] m_input_int8;
		}
		else {
			delete[] m_input_float;
		}
	}

	void setStdMean(std::vector<float>& mean, std::vector<float>& std) {
		mean_ = mean;
		std_ = std;
		// init mat m_mean
		std::vector<cv::Mat> std_channels;
		std::vector<cv::Mat> mean_channels;
		for (int i = 0; i < m_num_channels; i++) {
			/* Extract an individual channel. */
			cv::Mat std_channel(m_net_h, m_net_w, CV_32FC1, cv::Scalar((float)std[i]), cv::SophonDevice(this->m_dev_id));
			std_channels.push_back(std_channel);
			cv::Mat mean_channel(m_net_h, m_net_w, CV_32FC1, cv::Scalar((float)mean[i]), cv::SophonDevice(this->m_dev_id));
			mean_channels.push_back(mean_channel);
		}
		// Todo: fp16
		if (m_input_tensor->get_dtype() == BM_INT8) {
			m_std.create(m_net_h, m_net_w, CV_8SC3, m_dev_id);
			m_mean.create(m_net_h, m_net_w, CV_8SC3, m_dev_id);
		}
		else {
			m_std.create(m_net_h, m_net_w, CV_32FC3, m_dev_id);
			m_mean.create(m_net_h, m_net_w, CV_32FC3, m_dev_id);
		}

		cv::merge(std_channels, m_std);
		cv::merge(mean_channels, m_mean);
	}

	// int Init(std::vector<float> means, std::vector<float> stds) {
	int Init() {
		//1. get network
		m_bmNetwork = m_bmContext->network(0);

		//2. get input
		max_batch = m_bmNetwork->maxBatch();
		m_input_tensor = m_bmNetwork->inputTensor(0);
		m_num_channels = m_input_tensor->get_shape()->dims[1];
		m_net_h = m_input_tensor->get_shape()->dims[2];
		m_net_w = m_input_tensor->get_shape()->dims[3];
		m_input_count = bmrt_shape_count(m_input_tensor->get_shape());
		input_scale = m_input_tensor->get_scale();
		if (m_input_tensor->get_dtype() == BM_INT8) {
			m_input_int8 = new int8_t[m_input_count];
		}
		else {
			m_input_float = new float[m_input_count];
		}

		//3. get output
		// m_output_tensor = m_bmNetwork->outputTensor(0);
		// output_scale = m_output_tensor->get_scale();
		output_num = m_bmNetwork->outputTensorNum();

		std::vector<std::string> outputNames(output_num);
		for (int i = 0; i < output_num; i++) {
			outputNames[i] = m_bmNetwork->outputs_name[i];
		}
		output_names = outputNames;


		assert(output_num > 0);

		////4. set mean and scale
		//std::vector<float> mean_values;
		//std::vector<float> scale_values;
		//if (means.empty() || stds.empty())
		//{
		//	mean_values.push_back(0);
		//	mean_values.push_back(0);
		//	mean_values.push_back(0);

		//	scale_values.push_back(0.00392157);
		//	scale_values.push_back(0.00392157);
		//	scale_values.push_back(0.00392157);
		//}
		//else
		//{
		//	for (auto& v : means) {
		//		mean_values.push_back(-std::abs(v));
		//	}
		//	// mean_values = means;
		//	scale_values = stds;
		//}
		////dbg(mean_values);
		////dbg(scale_values);
		//setStdMean(scale_values, mean_values);

		//5. set device mem
		bmrt_tensor(&input_tensor,
			m_bmContext->bmrt(),
			m_input_tensor->get_dtype(),
			*m_input_tensor->get_shape());
		m_input_tensor->set_device_mem(&input_tensor.device_mem);
		return 0;
	}

	int batch_size() {
		return max_batch;
	}

//CHECK
	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(cv::Mat image) {
		int ret = 0;

		std::vector<cv::Mat> input_images;
		input_images.push_back(image);

		// 1. preprocess
		ret = pre_process(input_images);
		CV_Assert(ret == 0);
		ret = m_bmNetwork->forward();
		CV_Assert(ret == 0);

		// dbg(output_num);

		std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> result_map;

		std::vector<std::shared_ptr<BMNNTensor>> outputTensors(output_num);
		for (int i = 0; i < output_num; i++) {
			outputTensors[i] = m_bmNetwork->outputTensor(i);
		}


		for (int i = 0; i < output_num; i++) {
			auto m_output_tensor = outputTensors[i];

			float* output_data = (float*)m_output_tensor->get_cpu_data();
			float output_scale = m_output_tensor->get_scale();
			auto NetworkShape = m_output_tensor->get_shape();
			std::vector<int> out_shape(NetworkShape->dims, NetworkShape->dims + NetworkShape->num_dims);

			// dbg(output_scale);
			// dbg(out_shape.size());

			auto top = std::make_shared<glasssix::memory::tensor<float>>(out_shape);
			auto top_data = top->mutable_cpu_data();
			for (size_t idx = 0; idx < top->count(); idx++) {
				top_data[idx] = output_data[idx] * output_scale;
			}

			// npy::SAVE_TENSOR_TO_NUMPY(top,"blur_512out_tensr.npy");

			std::string tensor_name = output_names[i];
			// std::string tensor_name = "out" + std::to_string(i);

			result_map[tensor_name] = top;
		}

		return result_map;
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(std::shared_ptr<glasssix::memory::tensor<float>> input_ts) {
		// printf("bmnn_pipeline.hpp:334");
		// dbg(mean_);
		// dbg(std_);
		//normal
		if (input_ts->channels() == 3 && input_ts->order() == glasssix::memory::orderType::NCHW && mean_.size() == 3 && std_.size() == 3) {
			int HWStep = input_ts->width() * input_ts->height();
			for (int c = 0; c < input_ts->channels(); c++) {
				auto cpdata = input_ts->mutable_cpu_data() + c * HWStep;
				for (int i = 0; i < HWStep; i++)
					cpdata[i] = (cpdata[i] - std::abs(mean_[c])) * std_[c];
			}
		}
		
		// npy::SAVE_ARRAY_TO_NUMPY(input_ts->mutable_cpu_data(),{1,3,640,640},"m_input.npy");

		bm_memcpy_s2d(m_bmContext->handle(), input_tensor.device_mem, (void*)input_ts->mutable_cpu_data());

		int ret = m_bmNetwork->forward();
		CV_Assert(ret == 0);

		// dbg(output_num);

		std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> result_map;

		std::vector<std::shared_ptr<BMNNTensor>> outputTensors(output_num);
		for (int i = 0; i < output_num; i++) {
			outputTensors[i] = m_bmNetwork->outputTensor(i);
		}

		for (int i = 0; i < output_num; i++) {
			auto m_output_tensor = outputTensors[i];

			float* output_data = (float*)m_output_tensor->get_cpu_data();
			float output_scale = m_output_tensor->get_scale();
			auto NetworkShape = m_output_tensor->get_shape();
			std::vector<int> out_shape(NetworkShape->dims, NetworkShape->dims + NetworkShape->num_dims);

			// dbg(output_scale);
			// dbg(out_shape.size());

			auto top = std::make_shared<glasssix::memory::tensor<float>>(out_shape);
			auto top_data = top->mutable_cpu_data();
			for (size_t idx = 0; idx < top->count(); idx++) {
				top_data[idx] = output_data[idx] * output_scale;
			}

			// npy::SAVE_ARRAY_TO_NUMPY(output_data,out_shape,"blur_512out_array.npy");
			// npy::SAVE_TENSOR_TO_NUMPY(top,"blur_512out_tensr.npy");

			std::string tensor_name = output_names[i];
			// std::string tensor_name = "out" + std::to_string(i);

			result_map[tensor_name] = top;
		}

		return result_map;
	}

	//void set_convert_rgb(bool is_convert){
	//  convertRGB_=is_convert;
	//}

};

#endif /* _BMNNPIPELINE_HPP_ */
