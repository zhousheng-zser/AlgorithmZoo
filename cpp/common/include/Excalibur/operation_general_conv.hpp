#pragma once
#ifndef _OPEARTION_GENERAL_CONV_HPP_
#define _OPEARTION_GENERAL_CONV_HPP_

#include "operation.hpp"

namespace glasssix
{
	namespace excalibur
	{
		template<typename Dtype>
		class operation_general_conv : public operation<Dtype>
		{
		public:
			explicit operation_general_conv(const operation_param& param);

			virtual const char* type() const { return this->params_.type_.c_str(); }

			virtual ~operation_general_conv() {}

		public:
			/// parameters
			int output_channel_ = 0;
			int kernel_size_w_ = 0;
			int kernel_size_h_ = 0;
			int dilation_w_ = 1;
			int dilation_h_ = 1;
			int stride_w_ = 1;
			int stride_h_ = 1;
			int group_ = 1;
			int pad_left_ = 0;
			int pad_right_ = 0;
			int pad_top_ = 0;
			int pad_bottom_ = 0;
			Dtype pad_value_ = (Dtype)0.0;
			bool int8_scale_term_ = false;
			bool bias_term_ = false;
			int weight_data_size_ = 0;
			int activation_type_ = 0;
			int impl_type_ = 0;
			std::string activation_params_;
			int output_pad_right_ = 0;
			int output_pad_bottom_ = 0;
			int output_dim_h_ = 0;
			int output_dim_w_ = 0;
			/// calculate in runtime
			int num_;
			int input_channel_ = 0;
			int input_dim_h_;
			int input_dim_w_;
			int input_spatial_dim_;
			int bottom_dim_;
			int output_spatial_dim_;
			int top_dim_;
			bool isfirst = true;
			int last_height;
			int last_width;
			int kernel_dim_;
			int weight_offset_;
			int col_offset_;
			int output_offset_;

			std::shared_ptr<memory::tensor<float>> weight_data;
			std::shared_ptr<memory::tensor<float>> bias_data;
		};
	}
}
#endif // !_OPEARTION_GENERAL_CONV_HPP_
