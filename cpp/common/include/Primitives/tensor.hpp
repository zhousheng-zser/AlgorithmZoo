#pragma once

#ifndef _TENSOR_HPP_
#define _TENSOR_HPP_

#include "syncedmem.hpp"

#include <vector>
#include <memory>

namespace glasssix
{
	namespace memory
	{
		enum orderType { NCHW, NHWC };

		/// <summary>
		/// The four dimension tensor in N, C, H, W
		/// </summary>
		template <typename Dtype>
		class EXPORT_EXCALIBUR_PRIMITIVES tensor
		{
			// Data pointer
			std::shared_ptr<syncedmem<Dtype>> data_;
			// Allocator from outside
			pool_allocator<Dtype>* allocator_;
			// The 4-dim shape of the tensor in" NCHW/NHWC
			std::vector<int> shape_;
			// For NCHW: n * c * step_
			// For NHWC: n * h * step_
			size_t count_;

			// Pick CUDA support device
			int device_;
			// Data arrange order
			orderType order_;
			// Size of the data_offset(step_ * sizeof(Dtype) is aligned to 16):
			// h * w in NCHW order
			// w * c in NHWC order
			size_t step_;

		public:
			// empty tensor
			explicit tensor(orderType order = NCHW, pool_allocator<Dtype>* allocator = nullptr);
			// vector
			explicit tensor(const int w, int device = -1, orderType order = NCHW, pool_allocator<Dtype>* allocator = nullptr);
			// matrix/gray image
			explicit tensor(const int h, const int w, int device = -1, orderType order = NCHW, pool_allocator<Dtype>* allocator = nullptr);
			// external matrix/gray image
			explicit tensor(const int h, const int w, Dtype* data, int device = -1, orderType order = NCHW, pool_allocator<Dtype>* allocator = nullptr);
			// 3-dimension tensor/multi-channel image
			explicit tensor(const int c, const int h, const int w, int device = -1, orderType order = NCHW, pool_allocator<Dtype>* allocator = nullptr);
			// external 3-dimension tensor/multi-channel image
			explicit tensor(const int c, const int h, const int w, Dtype* data, int device = -1, orderType order = NCHW, pool_allocator<Dtype>* allocator = nullptr);
			// 4-dimension/any dimention tensor
			explicit tensor(const std::vector<int>& shape, int device = -1, orderType order = NCHW, pool_allocator<Dtype>* allocator = nullptr);

			~tensor();

			tensor(const tensor& t);
			tensor(tensor<Dtype>&& t) noexcept;
			tensor& operator=(const tensor& t);

			// Deep copy
			tensor clone() const;
			// Check empty
			bool empty() const 
			{
				return data_ == nullptr || count() == 0;
			}
			
			const Dtype* cpu_data() const;
			const Dtype* gpu_data() const;
			Dtype* mutable_cpu_data() const;
			Dtype* mutable_gpu_data() const;
			void fill(Dtype v);
			void convert_order();
			void copy_from(const void* data, size_t size) ;

			int num() const 
			{
				return shape_[0];
			}

			int channels() const 
			{
				return order_ == NCHW ? shape_[1] : shape_[3];
			}

			int height() const 
			{
				return order_ == NCHW ? shape_[2] : shape_[1];
			}

			int width() const 
			{
				return order_ == NCHW ? shape_[3] : shape_[2];
			}

			int count(int start_axis, int end_axis) const 
			{
				int count = 1;
				for (int i = start_axis; i < end_axis; ++i) {
					count *= shape_[i];
				}
				return count;
			}

			int count() const 
			{
				return count(0, static_cast<int>(shape_.size()));
			}

			int device() const 
			{
				return device_;
			}

			orderType order() const 
			{
				return order_;
			}

			int offset(const int n, const int c = 0, const int h = 0, const int w = 0) const
			{
				if (order_ == NCHW)
				{
					return ((n * channels() + c) * height() + h) * width() + w;
				}
				else
				{
					return ((n * height() + h) * width() + w) * channels() + c;
				}
			}

			std::vector<int> data_shape() const 
			{
				return shape_;
			}
			
			void reshape(std::vector<int> data_shape)
			{
				shape_ = data_shape;
			}

			void set_allocator(pool_allocator<Dtype>* allocator)
			{
				if (!allocator_)
				{
					allocator_ = allocator;
				}
			}

			pool_allocator<Dtype>* allocator() const
			{
				return allocator_;
			}

			// data reference
			tensor channel(int c);
			const tensor channel(int c) const;

			Dtype* row(int y);
			const Dtype* row(int y) const;

			// access raw data
			operator Dtype*();
			operator const Dtype*() const;

			// convenient access float vec element
			Dtype& operator[](size_t i);
			const Dtype& operator[](size_t i) const;
		};
	}
}
#endif // !_TENSOR_HPP_
