#pragma once
#ifndef _OPERATION_FACTORY_HPP_
#define _OPERATION_FACTORY_HPP_

#include <functional>
#include <map>
#include <algorithm>  
#include "../../include/Primitives/logger.hpp"
#include "../../include/Primitives/singleton.hpp"
#include "operation.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_reflector : public singleton<operation_reflector<Dtype>>
		{
		private:
			std::map<std::string, 
				std::function<std::shared_ptr<operation<Dtype>>(const operation_param&)>> object_map_;

		public:
			std::shared_ptr<operation<Dtype>> create_object(const operation_param& param)
			{
				for (auto & x : object_map_)
				{
					std::string type = param.type_;
					std::transform(type.begin(), type.end(), type.begin(), ::tolower);
#ifdef __ARM_ARCH
					if (x.first == std::string("operation_") + type)
					{
						if (object_map_.find(x.first + "_arm") == object_map_.end())
							return x.second(param);
						else
							return object_map_.find(x.first + "_arm")->second(param);
					}
#else
					if (x.first == std::string("operation_") + type)
						return x.second(param);
#endif
				}
				LOG(FATAL) << "Un-suppoted operation type: " << param.type_ << ". ";
				return nullptr;
			}

			void registe(const std::string &class_name, 
				std::function<std::shared_ptr<operation<Dtype>>(const operation_param&)> && generator)
			{
				object_map_[class_name] = generator;
			}
		};

		template<typename Dtype>
		class register_action
		{
		public:
			register_action(const std::string &class_name, 
				std::function<std::shared_ptr<operation<Dtype>>(const operation_param&)> && generator)
			{
				operation_reflector<Dtype>::instance().registe(class_name, 
					std::forward<std::function<std::shared_ptr<operation<Dtype>>(const operation_param&)>>(generator));
			}
		};

#define INSTANCE_CLASS(CLASS_NAME)\
template class CLASS_NAME<float>;\
template class CLASS_NAME<double>;\
template class CLASS_NAME<unsigned short>;

#define REGISTE_SHORT(CLASS_NAME) \
register_action<unsigned short>\
g_us_register_action_##CLASS_NAME(#CLASS_NAME, [](const operation_param& param)\
{\
    return std::shared_ptr<CLASS_NAME<unsigned short>>(new CLASS_NAME<unsigned short>(param));\
});
#define REGISTE_FLOAT(CLASS_NAME) \
register_action<float>\
g_f_register_action_##CLASS_NAME(#CLASS_NAME, [](const operation_param& param)\
{\
    return std::shared_ptr<CLASS_NAME<float>>(new CLASS_NAME<float>(param));\
});
#define REGISTE_DOUBLE(CLASS_NAME) \
register_action<double>\
g_d_register_action_##CLASS_NAME(#CLASS_NAME, [](const operation_param& param)\
{\
    return std::shared_ptr<CLASS_NAME<double>>(new CLASS_NAME<double>(param));\
});

#define REGISTE(CLASS_NAME)\
REGISTE_SHORT(CLASS_NAME) \
REGISTE_FLOAT(CLASS_NAME) \
REGISTE_DOUBLE(CLASS_NAME)

#define STUB_GPU(Operation_Name)\
template<typename Dtype>\
void Operation_Name<Dtype>::forward_gpu_f32(\
			const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,\
			std::vector<std::shared_ptr<memory::tensor<float>>>& tops)\
		{ NO_GPU; }
			
#define INSTANTIATE_OPERATION_CUDA_FWDF32(Operation_Name)\
template void Operation_Name<float>::forward_gpu_f32(cublasHandle_t &cublas_handle_,\
 const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,\
std::vector<std::shared_ptr<memory::tensor<float>>>& tops);\
template void Operation_Name<double>::forward_gpu_f32(cublasHandle_t &cublas_handle_,\
 const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,\
std::vector<std::shared_ptr<memory::tensor<float>>>& tops);\
template void Operation_Name<unsigned short>::forward_gpu_f32(cublasHandle_t &cublas_handle_,\
 const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,\
std::vector<std::shared_ptr<memory::tensor<float>>>& tops);\

#define INSTANTIATE_OPERATION_CUDNN_FWDF32(Operation_Name)\
template void Operation_Name<float>::forward_gpu_f32(cublasHandle_t &cublas_handle_, cudnnHandle_t cudnn_handle,\
 const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,\
std::vector<std::shared_ptr<memory::tensor<float>>>& tops);\
template void Operation_Name<double>::forward_gpu_f32(cublasHandle_t &cublas_handle_, cudnnHandle_t cudnn_handle,\
 const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,\
std::vector<std::shared_ptr<memory::tensor<float>>>& tops);\
template void Operation_Name<unsigned short>::forward_gpu_f32(cublasHandle_t &cublas_handle_, cudnnHandle_t cudnn_handle,\
 const std::vector<std::shared_ptr<memory::tensor<float>>>& bottoms,\
std::vector<std::shared_ptr<memory::tensor<float>>>& tops);\

	}
}
#endif // !_OPERATION_FACTORY_HPP_
