#include "feature_extractor_internal.hpp"
#include <memory>
#include <vector>
#include <algorithm>

#include <Excalibur/pipeline.hpp>
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

namespace glasssix::cassius
{
	namespace
	{
		constexpr std::size_t feature_size = 512;
		constexpr std::size_t single_bitmap_width = 128;
		constexpr std::size_t single_bitmap_height = 128;
		constexpr std::size_t single_bitmap_channels = 3;
		constexpr std::size_t single_bitmap_bytes = single_bitmap_channels * single_bitmap_width * single_bitmap_height;
	}

	class feature_extractor_internal::impl
	{
	public:
		impl(std::string_view phai_path, int device) : device_{ device }, unicorn_{ std::string{ phai_path }, device }
		{
		}

		std::vector<std::vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order)
		{
			init_cache(bitmaps, count, order);

			std::vector<std::vector<float>> result;
			auto network_result = unicorn_.forward(cache_ | memory::tensor_convert_to<float>);

			if (auto iter = network_result.find("conv5_dw"); iter != network_result.end())
			{
				auto iter_conv5_dw = iter->second->cpu_data();

				for (std::size_t i = 0; i < count; i++)
				{
					std::vector<float> feature(feature_size);

					std::copy(iter_conv5_dw, iter_conv5_dw + feature_size, feature.data());
					iter_conv5_dw += feature_size;
					result.emplace_back(feature);
				}
			}

			return result;
		}

		static std::string version()
		{
			return "1.0.0";
		}
	private:
		void init_cache(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order)
		{
			if (cache_ == nullptr || cache_->num() != count || cache_->order() != order)
			{
				cache_ = order == memory::NCHW ?
					std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{  static_cast<int>(count), single_bitmap_channels, single_bitmap_height, single_bitmap_width }, device_, static_cast<memory::orderType>(order), &memory::pool_allocator_default<std::uint8_t>::get()) :
					std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{ static_cast<int>(count), single_bitmap_height, single_bitmap_width, single_bitmap_channels }, device_, static_cast<memory::orderType>(order), & memory::pool_allocator_default<std::uint8_t>::get());
			}
			if (device_ > 0)
			{
#ifdef USE_CUDA
				cudaMemcpy(cache_->mutable_gpu_data(), bitmaps.data(), bitmaps.size(), cudaMemcpyHostToDevice);
#else
				NO_GPU;
#endif
			}

			std::copy(bitmaps.begin(), bitmaps.end(), cache_->mutable_cpu_data());
		}

		int device_;
		excalibur::pipeline<float> unicorn_;
		std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
	};

	feature_extractor_internal::feature_extractor_internal(std::string_view phai_path, int device) : impl_{ new impl{ phai_path, device } }
	{
	}

	feature_extractor_internal::~feature_extractor_internal()
	{
		if (impl_ != nullptr)
		{
			delete impl_;
			impl_ = nullptr;
		}
	}

	std::vector<std::vector<float>> feature_extractor_internal::get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order) const
	{
		return impl_->get(bitmaps, count, order);
	}

	std::string feature_extractor_internal::version()
	{
		return impl::version();
	}
}
