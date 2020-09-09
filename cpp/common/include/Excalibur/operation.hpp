#pragma once
#ifndef _OPERATION_HPP_
#define _OPERATION_HPP_
#include <algorithm>
#include <string>
#include <vector>
#include "../../include/Primitives/tensor.hpp"
#include "../../include/Primitives/simd_types.hpp"
#include "../../include/Primitives/blas.hpp"

#ifdef CAFFE_SUPPORT
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/message.h>
#include "../3rdparty/proto/caffe.pb.h"
#endif // CAFFE_SUPPORT

namespace glasssix
{
	namespace excalibur
	{
		struct operation_param
		{
			//type name, such as Convolution Softmax etc.
			std::string type_;
			//name of this operation, must be unique among all operation names
			std::string name_;
			//count of the tensors this operation needs as input
			int input_count_;
			//count of the tensors this operation produces as output
			int output_count_;
			//name list of all the input tensor names, seperated by space, must be unique among input tensor names of all layers
			std::vector<std::string> input_featmaps_;
			//name list of all the output tensor names, seperated by space, must be unique among output tensor names of all layers
			std::vector<std::string> output_featmaps_;
			//
			bool int8_quantization_;
			//
			bool brain_float16_;
			//
			bool float16_;
			//
			bool inplace_;
			//
			int device_;
			//key=value pair list, seperated by space
			std::string specific_params_;

			operation_param(std::string type = "N/A",
				std::string name = "N/A",
				int input_count = 1,
				int output_count = 1,
				std::vector<std::string> input_featmaps = std::vector<std::string>{},
				std::vector<std::string> output_featmaps = std::vector<std::string>{},
				bool int8_quantization = false,
				bool brain_float16 = false,
				bool float16 = false,
				bool inplace = false,
				int device = -1,
				std::string specific_params = ""
			) :type_(type), name_(name), input_count_(input_count), output_count_(output_count), input_featmaps_(input_featmaps),
				output_featmaps_(output_featmaps), int8_quantization_(int8_quantization), brain_float16_(brain_float16), float16_(float16),
				inplace_(inplace), device_(device), specific_params_(specific_params)
			{
				CHECK(!(brain_float16_ && float16_));
				CHECK(!(brain_float16_ && int8_quantization_));
				CHECK(!(float16_ && int8_quantization_));
			}

			void set_int8_quantization(bool int8_quantization)
			{
				int8_quantization_ = int8_quantization;
			}

			void set_brain_float16(bool brain_float16)
			{
				brain_float16_ = brain_float16;
			}

			void set_float16(bool float16)
			{
				float16_ = float16;
			}
		};

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

		template<typename Dtype>
		class operation
		{
		public:
			operation() {}

			explicit operation(const operation_param& param) : params_(param) {}

			virtual ~operation() {}

			virtual int init_weights()
			{
				return 0;
			}

			virtual int init_weights(FILE *fp)
			{
				return 0;
			}

			// inference on cpu
			void forward_cpu(const std::vector<std::shared_ptr<memory::tensor<Dtype>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<Dtype>>>& tops);

			// inference on gpu
			void forward_gpu(
#ifdef USE_CUDA
				cublasHandle_t& cublas_handle_,
#ifdef USE_CUDNN
				cudnnHandle_t cudnn_handle,
#endif //!USE_CUDNN
#endif //!USE_CUDA
				const std::vector<std::shared_ptr<memory::tensor<Dtype>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<Dtype>>>& tops);

			std::vector<std::shared_ptr<memory::tensor<float>>>& weights_f32() { return weights_f32_; }

			std::vector<std::shared_ptr<memory::tensor<unsigned short>>>& weights_f16() { return weights_f16_; }

			std::vector<std::shared_ptr<memory::tensor<signed char>>>& weights_i8() { return weights_i8_; }

			const operation_param& param() const { return params_; }

			virtual const char* type() const
			{
				return "Unknown Type";
			};

		protected:
			std::vector<std::shared_ptr<memory::tensor<double>>> weights_d64_;
			std::vector<std::shared_ptr<memory::tensor<float>>> weights_f32_;
			std::vector<std::shared_ptr<memory::tensor<unsigned short>>> weights_f16_;
			std::vector<std::shared_ptr<memory::tensor<signed char>>> weights_i8_;
			operation_param params_;
			std::vector<float> weights_scaletable_i8_;
			std::vector<float> featmap_scaletable_i8_;

			virtual void forward_cpu_f32(const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<float>>>& tops)
			{
				NOT_IMPLEMENTED;
			};

			virtual void forward_gpu_f32(
#ifdef USE_CUDA
				cublasHandle_t& cublas_handle_,
#ifdef USE_CUDNN
				cudnnHandle_t cudnn_handle,
#endif //!USE_CUDNN
#endif //!USE_CUDA
				const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<float>>>& tops)
			{
				NOT_IMPLEMENTED;
			};

			virtual void forward_cpu_d64(const std::vector<std::shared_ptr<memory::tensor<double>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<double>>>& tops)
			{
				NOT_IMPLEMENTED;
			};

			virtual void forward_gpu_d64(
#ifdef USE_CUDA
				cublasHandle_t& cublas_handle_,
#ifdef USE_CUDNN
				cudnnHandle_t cudnn_handle,
#endif //!USE_CUDNN
#endif //!USE_CUDA
				const std::vector<std::shared_ptr<memory::tensor<double>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<double>>>& tops)
			{
				NOT_IMPLEMENTED;
			};

			virtual void forward_cpu_f16(const std::vector<std::shared_ptr<memory::tensor<unsigned short>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<unsigned short>>>& tops)
			{
				LOG(WARNING) << "Degenerate to float32 type to forward, the preformace will cause slight performance degradation.";
				if (params_.brain_float16_)
				{
					std::vector<std::shared_ptr<memory::tensor<float>>> bottoms_temp(bottoms.size());
					std::vector<std::shared_ptr<memory::tensor<float>>> tops_temp(params_.output_count_);
					for (size_t i = 0; i < bottoms_temp.size(); i++)
					{
						bottoms_temp[i].reset(new memory::tensor<float>(bottoms[i]->data_shape(), bottoms[i]->device(), bottoms[i]->order(), nullptr));
						auto bottoms_temp_data = bottoms_temp[i]->mutable_cpu_data();
						auto bottoms_data = bottoms[i]->cpu_data();
						for (size_t j = 0; j < bottoms[i]->count(); j++)
						{
							bottoms_temp_data[j] = bfloat16_to_float32(bottoms_data[j]);
						}
					}
					forward_cpu_f32(bottoms_temp, tops_temp);
					for (size_t i = 0; i < bottoms_temp.size(); i++)
					{
						tops[i].reset(new memory::tensor<unsigned short>(tops_temp[i]->data_shape(), tops_temp[i]->device(), tops_temp[i]->order(), nullptr));
						auto tops_data = tops[i]->mutable_cpu_data();
						auto top_temps_data = tops_temp[i]->cpu_data();
						for (size_t j = 0; j < tops[i]->count(); j++)
						{
							tops_data[j] = float32_to_bfloat16(top_temps_data[j]);
						}
					}
				}
				else if (params_.float16_)
				{
					std::vector<std::shared_ptr<memory::tensor<float>>> bottoms_temp(bottoms.size());
					std::vector<std::shared_ptr<memory::tensor<float>>> tops_temp(params_.output_count_);
					for (size_t i = 0; i < bottoms_temp.size(); i++)
					{
						bottoms_temp[i].reset(new memory::tensor<float>(bottoms[i]->data_shape(), bottoms[i]->device(), bottoms[i]->order(), nullptr));
						auto bottoms_temp_data = bottoms_temp[i]->mutable_cpu_data();
						auto bottoms_data = bottoms[i]->cpu_data();
#if (SIMD_X86_INSTR_SET >= SIMD_X86_SSE_VERSION)
						half2float(bottoms_data, bottoms_temp_data, bottoms[i]->count());
#else
						NOT_IMPLEMENTED << " function 'half2float' ";
#endif
					}
					forward_cpu_f32(bottoms_temp, tops_temp);
					for (size_t i = 0; i < bottoms_temp.size(); i++)
					{
						tops[i].reset(new memory::tensor<unsigned short>(tops_temp[i]->data_shape(), tops_temp[i]->device(), tops_temp[i]->order(), nullptr));
						auto tops_data = tops[i]->mutable_cpu_data();
						auto top_temps_data = tops_temp[i]->cpu_data();
#if (SIMD_X86_INSTR_SET >= SIMD_X86_SSE_VERSION)
						float2half(top_temps_data, tops_data, tops[i]->count());
#else
						NOT_IMPLEMENTED << " function 'float2half' ";
#endif
					}
				}
				else
				{
					NOT_IMPLEMENTED << " This function should be overrided in derived class";
				}
			}

			virtual void forward_gpu_f16(
#ifdef USE_CUDA
				cublasHandle_t& cublas_handle_,
#ifdef USE_CUDNN
				cudnnHandle_t cudnn_handle,
#endif //!USE_CUDNN
#endif //!USE_CUDA
				const std::vector<std::shared_ptr<memory::tensor<unsigned short>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<unsigned short>>>& tops)
			{
				LOG(WARNING) << "Degenerate to float32 type to forward, the preformace will cause slight performance degradation.";
				if (params_.brain_float16_)
				{
					std::vector<std::shared_ptr<memory::tensor<float>>> bottoms_temp(bottoms.size());
					std::vector<std::shared_ptr<memory::tensor<float>>> tops_temp(params_.output_count_);
					for (size_t i = 0; i < bottoms_temp.size(); i++)
					{
						bottoms_temp[i].reset(new memory::tensor<float>(bottoms[i]->data_shape(), bottoms[i]->device(), bottoms[i]->order(), nullptr));
						auto bottoms_temp_data = bottoms_temp[i]->mutable_cpu_data();
						auto bottoms_data = bottoms[i]->cpu_data();
						for (size_t j = 0; j < bottoms[i]->count(); j++)
						{
							bottoms_temp_data[j] = bfloat16_to_float32(bottoms_data[j]);
						}
					}
					forward_gpu_f32(
#ifdef USE_CUDA
						cublas_handle_,
#ifdef USE_CUDNN
						cudnn_handle,
#endif //!USE_CUDNN
#endif //!USE_CUDA
						bottoms_temp, tops_temp);
					for (size_t i = 0; i < bottoms_temp.size(); i++)
					{
						tops[i].reset(new memory::tensor<unsigned short>(tops_temp[i]->data_shape(), tops_temp[i]->device(), tops_temp[i]->order(), nullptr));
						auto tops_data = tops[i]->mutable_cpu_data();
						auto top_temps_data = tops_temp[i]->cpu_data();
						for (size_t j = 0; j < tops[i]->count(); j++)
						{
							tops_data[j] = float32_to_bfloat16(top_temps_data[j]);
						}
					}
				}
				else if (params_.float16_)
				{
					std::vector<std::shared_ptr<memory::tensor<float>>> bottoms_temp(bottoms.size());
					std::vector<std::shared_ptr<memory::tensor<float>>> tops_temp(params_.output_count_);
					for (size_t i = 0; i < bottoms_temp.size(); i++)
					{
						bottoms_temp[i].reset(new memory::tensor<float>(bottoms[i]->data_shape(), bottoms[i]->device(), bottoms[i]->order(), nullptr));
						auto bottoms_temp_data = bottoms_temp[i]->mutable_cpu_data();
						auto bottoms_data = bottoms[i]->cpu_data();
#if (SIMD_X86_INSTR_SET >= SIMD_X86_SSE_VERSION)
						half2float(bottoms_data, bottoms_temp_data, bottoms[i]->count());
#else
						NOT_IMPLEMENTED << " function 'half2float' ";
#endif
					}
					forward_gpu_f32(
#ifdef USE_CUDA
						cublas_handle_,
#ifdef USE_CUDNN
						cudnn_handle,
#endif //!USE_CUDNN
#endif //!USE_CUDA
						bottoms_temp, tops_temp);
					for (size_t i = 0; i < bottoms_temp.size(); i++)
					{
						tops[i].reset(new memory::tensor<unsigned short>(tops_temp[i]->data_shape(), tops_temp[i]->device(), tops_temp[i]->order(), nullptr));
						auto tops_data = tops[i]->mutable_cpu_data();
						auto top_temps_data = tops_temp[i]->cpu_data();
#if (SIMD_X86_INSTR_SET >= SIMD_X86_SSE_VERSION)
						float2half(top_temps_data, tops_data, tops[i]->count());
#else
						NOT_IMPLEMENTED << " function 'float2half' ";
#endif
					}
				}
				else
				{
					NOT_IMPLEMENTED << " This function should be overrided in derived class";
				}
			}

			virtual void forward_cpu_i8(const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<float>>>& tops)
			{
				NOT_IMPLEMENTED << " This function should be overrided in derived class";
			}

			virtual void forward_gpu_i8(
#ifdef USE_CUDA
				cublasHandle_t& cublas_handle_,
#ifdef USE_CUDNN
				cudnnHandle_t cudnn_handle,
#endif //!USE_CUDNN
#endif //!USE_CUDA
				const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,
				std::vector<std::shared_ptr<memory::tensor<float>>>& tops)
			{
				NOT_IMPLEMENTED << " This function should be overrided in derived class";
			}

		private:
			DISABLE_COPY_AND_ASSIGN(operation);
		};

	}
}
#endif // !_OPERATION_HPP_
