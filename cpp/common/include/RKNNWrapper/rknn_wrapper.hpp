#pragma once
#ifndef RKNNWRAPPER_HPP
#define RKNNWRAPPER_HPP

#include <exception>
#include <unordered_map>

#include "Primitives/tensor.hpp"
#include "Primitives/fmt/format.h"
#include "rknn_api.h"

namespace glasssix
{
	namespace rknnwrapper
	{
		class rknn_exception : public std::exception
		{
		public:
			rknn_exception() : code_(RKNN_SUCC), message_("No Exception.") {}
			rknn_exception(int code, std::string &str) :code_(code), message_(str) {}
			rknn_exception(int code, const char *str) :code_(code), message_(str) {}
			~rknn_exception() throw () {
			}

			virtual const char* what() const throw () {
				return message_.c_str();
			}
			int what_code() const throw () {
				return code_;
			}

		private:
			int code_;
			std::string message_;
		};

		static void printRKNNTensor(rknn_tensor_attr* attr)
		{
			printf("index=%d name=%s n_dims=%d dims=[", attr->index, attr->name, attr->n_dims);
			for (size_t i = 0; i < attr->n_dims; i++)
			{
				if (i == attr->n_dims - 1)
					printf("%d] ", attr->dims[attr->n_dims - i - 1]);
				else
					printf("%d ", attr->dims[attr->n_dims - i - 1]);
			}
			printf("n_elems=%d size=%d fmt=%d type=%d qnt_type=%d fl=%d zp=%d scale=%f\n", attr->n_elems, attr->size, 0, attr->type, attr->qnt_type, attr->fl, attr->zp, attr->scale);
		}
		
		static unsigned char *load_model(const char *filename, int *model_size)
		{
			FILE *fp = fopen(filename, "rb");
			if(fp == nullptr) {
				printf("fopen %s fail!\n", filename);
				return NULL;
			}
			fseek(fp, 0, SEEK_END);
			int model_len = ftell(fp);
			unsigned char *model = (unsigned char*)malloc(model_len);
			fseek(fp, 0, SEEK_SET);
			if(model_len != fread(model, 1, model_len, fp)) {
				printf("fread %s fail!\n", filename);
				free(model);
				return NULL;
			}
			*model_size = model_len;
			if(fp) {
				fclose(fp);
			}
			return model;
		}

		class rknn_wrapper
		{
		public:
			rknn_wrapper() = delete;
			rknn_wrapper(uint32_t flag):ctx_(0), flag_(flag){}
			rknn_wrapper(const std::vector<std::string>& phai, std::string racy, int device = -1, uint32_t flag = 0) :rknn_wrapper(flag)
			{
				int model_data_size = 0;
				unsigned char* model_data = load_model(racy.replace(racy.length() - 4, 4, "rknn").c_str(), &model_data_size);
				
				int ret = rknn_init(&ctx_, reinterpret_cast<void*>(model_data), model_data_size, flag_);
				free(model_data);
				model_data = nullptr;
				if(ret != 0)
					throw rknn_exception(ret, "rknn_init fail!");
				
				ret = rknn_query(ctx_, RKNN_QUERY_IN_OUT_NUM, &io_num_, sizeof(io_num_));
				if (ret != RKNN_SUCC) 
				{
					throw rknn_exception(ret, "rknn_query io_num_ fail!");
				}
				printf("model input num: %d, output num: %d\n", io_num_.n_input, io_num_.n_output);

				printf("input tensors:\n");
				rknn_tensor_attr input_attrs[io_num_.n_input];
				std::memset(input_attrs, 0, sizeof(input_attrs));
				for (int i = 0; i < io_num_.n_input; i++) 
				{
					input_attrs[i].index = i;
					ret = rknn_query(ctx_, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
					if (ret != RKNN_SUCC) 
					{
						throw rknn_exception(ret, "rknn_query input_attrs fail!");
					}
					printRKNNTensor(&(input_attrs[i]));
				}

				printf("output tensors:\n");
				rknn_tensor_attr output_attrs[io_num_.n_output];
				memset(output_attrs, 0, sizeof(output_attrs));
				for (int i = 0; i < io_num_.n_output; i++) 
				{
					output_attrs[i].index = i;
					ret = rknn_query(ctx_, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
					if (ret != RKNN_SUCC) 
					{
						throw rknn_exception(ret, "rknn_query output_attrs fail!");
					}
					output_name_index_[output_attrs[i].index] = std::string(output_attrs[i].name);
					std::vector<int> shape(output_attrs[i].n_dims > 4 ? output_attrs[i].n_dims : 4, 1);
					for(uint32_t j = 0; j < output_attrs[i].n_dims; j++)
						shape[j] = static_cast<int>(output_attrs[i].dims[j]);
					std::reverse(shape.begin(), shape.end());
					output_tensor_shape_index_[output_attrs[i].index] = shape;
					printRKNNTensor(&(output_attrs[i]));
				}
			}
			
			~rknn_wrapper()
			{
				int ret = rknn_destroy(ctx_);
				if(ret != 0)
					printf("rknn_destroy fail!\n");
			}

			std::string version()
			{
				rknn_sdk_version version;
				int ret = rknn_query(ctx_, RKNN_QUERY_SDK_VERSION, &version, sizeof(rknn_sdk_version));
				if (ret < 0)
					throw rknn_exception(ret, "rknn query sdk version failed");

				return fmt::format(R"({}_{})", version.api_version, version.drv_version);
			}
			
			std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> forward(const std::shared_ptr<memory::tensor<std::uint8_t>>& input_tensor)
			{
				CHECK_EQ(1, io_num_.n_input);
				
				int num = input_tensor->num();
				int size = input_tensor->count(1, 4);
				
				std::vector<std::vector<float>> temp(io_num_.n_output);
				for(int num_i = 0; num_i < num; num_i++)
				{
					
					rknn_input inputs[1];
					std::memset(inputs, 0, sizeof(inputs));
					inputs[0].index = 0;
					inputs[0].type = RKNN_TENSOR_UINT8;
					inputs[0].size = size;
					inputs[0].fmt = static_cast<rknn_tensor_format>(input_tensor->order());
					inputs[0].buf = const_cast<std::uint8_t *>(input_tensor->cpu_data() + num_i * size);

					int ret = rknn_inputs_set(ctx_, io_num_.n_input, inputs);
					if(ret < 0) 
					{
						throw rknn_exception(ret, "rknn_input_set fail!");
					}
					
					ret = rknn_run(ctx_, nullptr);
					if(ret < 0) 
					{
						throw rknn_exception(ret, "rknn_run fail!");
					}
					
					rknn_output outputs[io_num_.n_output];
					std::memset(outputs, 0, sizeof(outputs));
					for(size_t i = 0; i < io_num_.n_output; i++)
						outputs[i].want_float = 1;
					
					ret = rknn_outputs_get(ctx_, io_num_.n_output, outputs, NULL);
					if(ret < 0) 
					{
						throw rknn_exception(ret, "rknn_outputs_get fail!");
					}
					
					for(size_t i = 0; i < io_num_.n_output; i++)
					{
						temp[outputs[i].index].insert(temp[outputs[i].index].end(), reinterpret_cast<float *>(outputs[i].buf), reinterpret_cast<float*>(outputs[i].buf) + outputs[i].size / sizeof(float));
					}
					
					rknn_outputs_release(ctx_, io_num_.n_output, outputs);
				}
				
				std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> result;
				for(size_t index = 0; index < io_num_.n_output; index++)
				{
					std::vector<int> temp_shape = output_tensor_shape_index_[index];
					temp_shape[0] = num;
					auto output_tensor = std::make_shared<memory::tensor<float>>(temp_shape);
					std::copy(temp[index].begin(), temp[index].end(), output_tensor->mutable_cpu_data());
					result[output_name_index_[index]] = output_tensor;
				}
				return result;
			}
			
			std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> forward(const std::uint8_t* input_data, std::vector<int> data_shape, rknn_tensor_format fmt)
			{
				CHECK_EQ(1, io_num_.n_input);

				int size = data_shape[1] * data_shape[2] * data_shape[3];

				std::vector<std::vector<float>> temp(io_num_.n_output);
				for (int num_i = 0; num_i < data_shape[0]; num_i++)
				{

					rknn_input inputs[1];
					std::memset(inputs, 0, sizeof(inputs));
					inputs[0].index = 0;
					inputs[0].type = RKNN_TENSOR_UINT8;
					inputs[0].size = size;
					inputs[0].fmt = fmt;
					inputs[0].buf = const_cast<std::uint8_t*>(input_data + num_i * size);

					int ret = rknn_inputs_set(ctx_, io_num_.n_input, inputs);
					if (ret < 0)
					{
						throw rknn_exception(ret, "rknn_input_set fail!");
					}

					ret = rknn_run(ctx_, nullptr);
					if (ret < 0)
					{
						throw rknn_exception(ret, "rknn_run fail!");
					}

					rknn_output outputs[io_num_.n_output];
					std::memset(outputs, 0, sizeof(outputs));
					for (size_t i = 0; i < io_num_.n_output; i++)
						outputs[i].want_float = 1;

					ret = rknn_outputs_get(ctx_, io_num_.n_output, outputs, NULL);
					if (ret < 0)
					{
						throw rknn_exception(ret, "rknn_outputs_get fail!");
					}

					for (size_t i = 0; i < io_num_.n_output; i++)
					{
						temp[outputs[i].index].insert(temp[outputs[i].index].end(), reinterpret_cast<float*>(outputs[i].buf), reinterpret_cast<float*>(outputs[i].buf) + outputs[i].size / sizeof(float));
					}

					rknn_outputs_release(ctx_, io_num_.n_output, outputs);
				}

				std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> result;
				for (size_t index = 0; index < io_num_.n_output; index++)
				{
					std::vector<int> temp_shape = output_tensor_shape_index_[index];
					temp_shape[0] = data_shape[0];
					auto output_tensor = std::make_shared<memory::tensor<float>>(temp_shape);
					std::copy(temp[index].begin(), temp[index].end(), output_tensor->mutable_cpu_data());
					result[output_name_index_[index]] = output_tensor;
				}
				return result;
			}
			
		private:
			rknn_context ctx_;
			uint32_t flag_;
			rknn_input_output_num io_num_;
			std::unordered_map<int, std::string> output_name_index_;
			std::unordered_map<int, std::vector<int>> output_tensor_shape_index_;

			static std::vector<std::string> split_string(const std::string& s, const std::string& c)
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
	}
}

#endif
