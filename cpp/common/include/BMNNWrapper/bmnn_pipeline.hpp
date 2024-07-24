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

		cv::Mat sample_float(m_net_h, m_net_w, CV_32FC3, cv::SophonDevice(this->m_dev_id));
		image.convertTo(sample_float, CV_32FC3);

		cv::add(sample_float, m_mean, sample_float);	 // sample_float += m_mean
		cv::multiply(sample_float, m_std, sample_float); // sample_float *= m_std
		cv::Mat input_c0(m_net_h, m_net_w, CV_32FC1, m_input_float + 0 * m_net_h * m_net_w);
		cv::Mat input_c1(m_net_h, m_net_w, CV_32FC1, m_input_float + 1 * m_net_h * m_net_w);
		cv::Mat input_c2(m_net_h, m_net_w, CV_32FC1, m_input_float + 2 * m_net_h * m_net_w);
		std::array<cv::Mat, 3> input_channels{ input_c0 ,input_c1 ,input_c2 };
		cv::split(sample_float, input_channels); //eq split to m_input_float

		return forward(m_input_float, { 1,3,image.rows,image.cols }, 0);
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forwardtest(cv::Mat image) 
	{
		cv::Mat sample_float(m_net_h, m_net_w, CV_32FC3, cv::SophonDevice(this->m_dev_id));
		image.convertTo(sample_float, CV_32FC3);
		return forward(m_input_float, { 1,3,image.rows,image.cols }, 0);
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(std::shared_ptr<glasssix::memory::tensor<float>> input_ts) {
		return forward(input_ts->cpu_data(), input_ts->data_shape(), input_ts->order());
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(const float* input_data, std::vector<int> data_shape, int order) {

		bm_memcpy_s2d(m_bmContext->handle(), input_tensor.device_mem, (void*)input_data);

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

			if (std::abs(output_scale - 1.f) < 0.0001) {
				auto output_tensor = std::make_shared<glasssix::memory::tensor<float>>(out_shape);
				std::copy(output_data, output_data + output_tensor->count(), output_tensor->mutable_cpu_data());

				result_map[output_names[i]] = output_tensor;
			}
			else {
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
		}

		return result_map;
	}

};

#endif /* _BMNNPIPELINE_HPP_ */
