#pragma once
#ifndef RKNNWRAPPER_HPP
#define RKNNWRAPPER_HPP

#include <exception>
#include <unordered_map>

#include "Primitives/tensor.hpp"
#include "Primitives/fmt/format.h"

#if defined(BUILD_RV1106) 
#include "rknn_api_rv1106.h"
#else
#include "rknn_api.h"
#endif
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
		
		static void dump_tensor_attr(rknn_tensor_attr* attr)
		{
		  printf("index=%d name=%s n_dims=%d dims=[", attr->index, attr->name, attr->n_dims);
		  for (size_t i = 0; i < attr->n_dims; i++)
		  {
			  if (i == attr->n_dims - 1)
				  printf("%d] ", attr->dims[i]);
			  else
				  printf("%d ", attr->dims[i]);
		  }
		  printf("n_elems=%d size=%d fmt=%s type=%s qnt_type=%s zp=%d scale=%f\n", attr->n_elems, attr->size, get_format_string(attr->fmt), get_type_string(attr->type),
			  get_qnt_type_string(attr->qnt_type), attr->zp, attr->scale);
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

#if defined(BUILD_RV1106) 
		static float deqnt_affine_to_f32(int8_t qnt, int32_t zp, float scale)
		{
			return ((float)qnt - (float)zp) * scale;
		}

		static void NC1HWC2_int8_to_NCHW_float(const int8_t *src, float *dst, int *dims, int channel, int h, int w, int32_t qnt_zps, float &qnt_scales)
		{
			int batch = dims[0];
			int C1 = dims[1];
			int C2 = dims[4];
			int hw_src = dims[2] * dims[3];
			int hw_dst = h * w;
			for (int i = 0; i < batch; i++)
			{
				src = src + i * C1 * hw_src * C2;
				dst = dst + i * channel * hw_dst;
				for (int c = 0; c < channel; ++c)
				{
				int plane = c / C2;
				const int8_t *src_c = plane * hw_src * C2 + src;
				int offset = c % C2;
				for (int cur_h = 0; cur_h < h; ++cur_h)
					for (int cur_w = 0; cur_w < w; ++cur_w)
					{
					int cur_hw = cur_h * w + cur_w;
					dst[c * hw_dst + cur_h * w + cur_w] = deqnt_affine_to_f32(src_c[C2 * cur_hw + offset] , qnt_zps, qnt_scales )  ;
					}
				}
			}
		}

		static void NCHW_int8_to_NCHW_float(const int8_t *src, float *dst, int32_t qnt_zps, float &qnt_scales,int length)
		{
			for (int i = 0; i < length; i++)
			{
				dst[i] = deqnt_affine_to_f32(src[i] , qnt_zps, qnt_scales )  ;
			}
		}

		static float f16_2_f32(const uint16_t &f16)
		{
			uint32_t sign = (f16 >> 15) & 0x1;
			uint32_t exponent = ((f16 >> 10) & 0x1f) - 15 + 127;
			uint32_t mantissa = (f16 & 0x3ff) << 13;
			uint32_t bits = (sign << 31) | (exponent << 23) | mantissa;
			float f = *((float*) &bits);
			// std::cout << "float: " << f << std::endl;
			return f;
		}

		static void NC1HWC2_f16_to_NCHW_float(const uint16_t *src, float *dst, int *dims, int channel, int h, int w)
		{
			int batch = dims[0];
			int C1 = dims[1];
			int C2 = dims[4];
			int hw_src = dims[2] * dims[3];
			int hw_dst = h * w;
			for (int i = 0; i < batch; i++)
			{
				src = src + i * C1 * hw_src * C2;
				dst = dst + i * channel * hw_dst;
				for (int c = 0; c < channel; ++c)
				{
				int plane = c / C2;//c2==0
				const uint16_t *src_c = plane * hw_src * C2 + src;
				int offset = c % C2;
				for (int cur_h = 0; cur_h < h; ++cur_h)
					for (int cur_w = 0; cur_w < w; ++cur_w)
					{
					int cur_hw = cur_h * w + cur_w;
					dst[c * hw_dst + cur_h * w + cur_w] = f16_2_f32( src_c[C2 * cur_hw + offset] )  ;
					}
				}
			}
		}

		void tensorf16_2_f32(const uint16_t *source ,float* dst,int length)
		{
			for(int i=0;i<length;i++)
			{
				dst[i]= f16_2_f32(source[i]);
			}
		}

#endif



		class rknn_wrapper
		{
		public:
			rknn_wrapper() = delete;
			rknn_wrapper(uint32_t flag):ctx_(0), flag_(flag){}
			rknn_wrapper(const std::vector<std::string>& phai, std::string racy, int device = -1, uint32_t flag = 0) :rknn_wrapper(flag)
			{
				int model_data_size = 0;
				unsigned char* model_data = load_model(racy.replace(racy.length() - 4, 4, "rknn").c_str(), &model_data_size);
				
				int ret = rknn_init(&ctx_, reinterpret_cast<void*>(model_data), model_data_size, flag_, nullptr);
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
				input_attrs.resize( io_num_.n_input );
				std::memset(input_attrs.data(), 0, io_num_.n_input * sizeof(rknn_tensor_attr));
				for (int i = 0; i < io_num_.n_input; i++) 
				{
					input_attrs[i].index = i;
					ret = rknn_query(ctx_, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
					if (ret != RKNN_SUCC) 
					{
						throw rknn_exception(ret, "rknn_query input_attrs fail!");
					}
					dump_tensor_attr(&(input_attrs[i]));
				}

				printf("output tensors:\n");
				output_attrs.resize(io_num_.n_output);
				memset(output_attrs.data(), 0, io_num_.n_output * sizeof(rknn_tensor_attr));
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
			
					output_tensor_shape_index_[output_attrs[i].index] = shape;
					dump_tensor_attr(&(output_attrs[i]));
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

#if defined(BUILD_RV1106) 
			std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> forward(const std::uint8_t* input_data, std::vector<int> data_shape, rknn_tensor_format fmt)
			{
				CHECK_EQ(1, io_num_.n_input);
				int ret_=0;
				std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> result;
				int size = data_shape[1] * data_shape[2] * data_shape[3]*sizeof(uint8_t);
				std::vector<std::vector<float>> temp(io_num_.n_output);
				for (int num_i = 0; num_i < data_shape[0]; num_i++)
				{
					rknn_input inputs[1];
					std::memset(inputs, 0, sizeof(inputs));
					inputs[0].type = RKNN_TENSOR_UINT8;
					inputs[0].size = size;
					inputs[0].fmt = fmt;

					rknn_tensor_mem *input_mems[1];
					input_mems[0] = rknn_create_mem(ctx_, input_attrs[0].size_with_stride);

					int width = input_attrs[0].dims[2];
  					int stride = input_attrs[0].w_stride;
					if (width == stride)
					{
						memcpy(input_mems[0]->virt_addr, input_data, width * input_attrs[0].dims[1] * input_attrs[0].dims[3]);
					}
					else
					{
						int height = input_attrs[0].dims[1];
						int channel = input_attrs[0].dims[3];
						// copy from src to dst with stride
						const uint8_t *src_ptr = input_data;
						uint8_t *dst_ptr = (uint8_t *)input_mems[0]->virt_addr;
						// width-channel elements
						int src_wc_elems = width * channel;
						int dst_wc_elems = stride * channel;
						for (int h = 0; h < height; ++h)
						{
							memcpy(dst_ptr, src_ptr, src_wc_elems);
							src_ptr += src_wc_elems;
							dst_ptr += dst_wc_elems;
						}
					}

					rknn_tensor_attr orig_output_attrs[io_num_.n_output];
  					memset(orig_output_attrs, 0, io_num_.n_output * sizeof(rknn_tensor_attr));
					for (uint32_t i = 0; i < io_num_.n_output; i++)
					{
						orig_output_attrs[i].index = i;
						// query info
						ret_ = rknn_query(ctx_, RKNN_QUERY_NATIVE_OUTPUT_ATTR, &(orig_output_attrs[i]), sizeof(rknn_tensor_attr));
						if (ret_ != RKNN_SUCC)
						{
							printf("rknn_query fail! ret_=%d\n", ret_);
						}
						// dump_tensor_attr(&orig_output_attrs[i]); for debug
					}


					rknn_tensor_mem *output_mems[io_num_.n_output];

					for (uint32_t i = 0; i < io_num_.n_output; ++i)
					{
						output_mems[i] = rknn_create_mem(ctx_, orig_output_attrs[i].size_with_stride);
					}
					ret_ = rknn_set_io_mem(ctx_, input_mems[0], &input_attrs[0]);

					for (uint32_t i = 0; i < io_num_.n_output; ++i)
					{
						// set output memory and attribute
						ret_ = rknn_set_io_mem(ctx_, output_mems[i], &orig_output_attrs[i]);
						if (ret_ < 0)
						{
							printf("rknn_set_io_mem fail! ret_=%d\n", ret_);
						}
					}
					// Run
					{
						ret_ = rknn_run(ctx_, NULL);
						if (ret_ < 0)
						{
							printf("rknn run error %d\n", ret_);
						}
					}
					
					std::vector<float> out_scales;
					std::vector<int32_t> out_zps;

					for (int i = 0; i < io_num_.n_output; ++i)
					{
                        {
						    out_scales.push_back(orig_output_attrs[i].scale);
						    out_zps.push_back(orig_output_attrs[i].zp);
                        }
					}
				
					for (uint32_t i = 0; i < io_num_.n_output; i++)
					{
						std::vector<float> temp(output_attrs[i].n_elems);	

						std::vector<int> temp_shape = output_tensor_shape_index_[i];
						temp_shape[0] = data_shape[0];
						auto output_tensor = std::make_shared<memory::tensor<float>>(temp_shape);
						// int8_t *datas=  (int8_t *)output_mems[i]->virt_addr;
						// int8_t datas1= ((int8_t *)output_mems[i]->virt_addr)[0];
						if(orig_output_attrs[i].type==RKNN_TENSOR_INT8)
						{

							if( orig_output_attrs[i].fmt==RKNN_TENSOR_NC1HWC2 )
							{
								int channel = output_attrs[i].dims[1];
								int h = output_attrs[i].n_dims > 2 ? output_attrs[i].dims[2] : 1;
								int w = output_attrs[i].n_dims > 3 ? output_attrs[i].dims[3] : 1;
								int hw = h * w;
								NC1HWC2_int8_to_NCHW_float((int8_t *)output_mems[i]->virt_addr, output_tensor->mutable_cpu_data(), (int *)orig_output_attrs[i].dims,
														channel, h, w , out_zps[i] , out_scales[i]);
							}
							else
							{
								NCHW_int8_to_NCHW_float((int8_t *)output_mems[i]->virt_addr,output_tensor->mutable_cpu_data() ,out_zps[i] , out_scales[i],output_attrs[i].n_elems );								
							}
						}
						
						if(orig_output_attrs[i].type==RKNN_TENSOR_FLOAT16)
						{
							if( orig_output_attrs[i].fmt==RKNN_TENSOR_NC1HWC2 )
							{
								int channel = output_attrs[i].dims[1];
								int h = output_attrs[i].n_dims > 2 ? output_attrs[i].dims[2] : 1;
								int w = output_attrs[i].n_dims > 3 ? output_attrs[i].dims[3] : 1;
								int hw = h * w;
								NC1HWC2_f16_to_NCHW_float((uint16_t *)output_mems[i]->virt_addr, output_tensor->mutable_cpu_data(), 
															(int *)orig_output_attrs[i].dims, channel, h, w );
							}
							else
							{					
								tensorf16_2_f32((uint16_t *)output_mems[i]->virt_addr ,output_tensor->mutable_cpu_data() , output_attrs[i].n_elems);
							}								
						}
						// float *data= output_tensor->mutable_cpu_data();
						// std::cout<<" "<<output_name_index_[i]<<std::endl;
						result[output_name_index_[i]] = output_tensor;
						// int m=10;
					}

					rknn_destroy_mem(ctx_, input_mems[0]);
					for (uint32_t i = 0; i < io_num_.n_output; ++i)
					{
						rknn_destroy_mem(ctx_, output_mems[i]);
					}
				}
				return result;
			}

#else	

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
				for (size_t index = 0; index < io_num_.n_output; index++)
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

				int size = data_shape[1] * data_shape[2] * data_shape[3]*sizeof(uint8_t);
				// std::cout<<"pic size"<<size<<"\n";
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
			
			std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> forward(const float* input_data, std::vector<int> data_shape, rknn_tensor_format fmt)
			{
				CHECK_EQ(1, io_num_.n_input);

				int size = data_shape[1] * data_shape[2] * data_shape[3]*sizeof(float);
				// std::cout<<"pic size"<<size<<"\n";
				std::vector<std::vector<float>> temp(io_num_.n_output);
				for (int num_i = 0; num_i < data_shape[0]; num_i++)
				{
					rknn_input inputs[1];
					std::memset(inputs, 0, sizeof(inputs));
					inputs[0].index = 0;
					inputs[0].type = RKNN_TENSOR_FLOAT32;
					inputs[0].size = size;
					inputs[0].fmt = fmt;
					inputs[0].buf = const_cast<float*>(input_data + num_i * size);

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
#endif		
		private:
			rknn_context ctx_;
			uint32_t flag_;
			rknn_input_output_num io_num_;
			std::vector<rknn_tensor_attr> input_attrs;
			std::vector<rknn_tensor_attr> output_attrs;
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
