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
		/// A non-generic abstaction of glasssix::memory::tensor.
		/// </summary>
		struct tensor_
		{
			virtual bool empty() const = 0;
			virtual int num() const = 0;
			virtual int channels() const = 0;
			virtual int height() const = 0;
			virtual int width() const = 0;
			virtual int count(int start_axis, int end_axis) const = 0;
			virtual int count() const = 0;
			virtual int device() const = 0;
			virtual orderType order() const = 0;
			virtual int offset(const int n, const int c = 0, const int h = 0, const int w = 0) const = 0;
			virtual std::vector<int> data_shape() const = 0;
			virtual void* cpu_data_any() const = 0;
			virtual void* gpu_data_any() const = 0;
			virtual void* data_auto() const = 0;
			virtual tensor_* clone_new() = 0;
			virtual std::shared_ptr<tensor_> clone_shared() = 0;
			virtual void copy_from(const void* data, size_t size) = 0;
		};

		/// <summary>
		/// The four dimension tensor in N, C, H, W
		/// </summary>
		template <typename Dtype>
		class EXPORT_EXCALIBUR_PRIMITIVES tensor : public tensor_
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
			virtual bool empty() const override
			{
				return data_ == nullptr || count() == 0;
			}
			
			virtual tensor_* clone_new() override
			{
				return new tensor<Dtype>{ clone() };
			}

			virtual std::shared_ptr<tensor_> clone_shared() override
			{
				return std::make_shared<tensor<Dtype>>(clone());
			}

			// DEPTRCATED!
			tensor channel_tensor_ptr(int c) 
			{
				DEPRECATED;
				return tensor();
			};

			const Dtype* cpu_data() const;

			const Dtype* gpu_data() const;

			Dtype* mutable_cpu_data() const;

			Dtype* mutable_gpu_data() const;

			// DEPTRCATED!
			void set_cpu_data(Dtype* data)
			{
				DEPRECATED;
			}

			// DEPRECATED!
			void set_gpu_data(Dtype* data)
			{
				DEPRECATED;
			}

			void fill(Dtype v);

			void convert_order();

			virtual void* cpu_data_any() const override
			{
				return mutable_cpu_data();
			}

			virtual void* gpu_data_any() const override
			{
				return mutable_gpu_data();
			}

			virtual void* data_auto() const override
			{
				return device_ >= 0 ? gpu_data_any() : cpu_data_any();
			}

			virtual void copy_from(const void* data, size_t size) override;

			virtual int num() const override
			{
				return shape_[0];
			}

			virtual int channels() const override
			{
				return order_ == NCHW ? shape_[1] : shape_[3];
			}

			virtual int height() const override
			{
				return order_ == NCHW ? shape_[2] : shape_[1];
			}

			virtual int width() const override
			{
				return order_ == NCHW ? shape_[3] : shape_[2];
			}

			virtual int count(int start_axis, int end_axis) const override
			{
				int count = 1;
				for (int i = start_axis; i < end_axis; ++i) {
					count *= shape_[i];
				}
				return count;
			}

			virtual int count() const override
			{
				return count(0, static_cast<int>(shape_.size()));
			}

			virtual int device() const override
			{
				return device_;
			}

			virtual orderType order() const override
			{
				return order_;
			}

			virtual int offset(const int n, const int c = 0,
				const int h = 0, const int w = 0) const override
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

			virtual std::vector<int> data_shape() const override
			{
				return shape_;
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
