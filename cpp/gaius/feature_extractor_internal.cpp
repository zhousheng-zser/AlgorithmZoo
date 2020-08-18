#include "feature_extractor_internal.hpp"

#include <memory>
#include <vector>
#include <algorithm>

#include <Excalibur/pipeline.hpp>
#include <Primitives/tensor_conversions.hpp>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

namespace glasssix::gaius
{
	namespace
	{
		constexpr std::size_t feature_size = 128;
		constexpr std::size_t single_bitmap_width = 128;
		constexpr std::size_t single_bitmap_height = 128;
		constexpr std::size_t single_bitmap_channels = 3;
		constexpr std::size_t single_bitmap_bytes = single_bitmap_channels * single_bitmap_width * single_bitmap_height;
	}

	class feature_extractor_internal::impl
	{
	public:
		impl() : impl{ -1 }
		{
		}

		impl(int device) : device_{ device }, mobile_unicorn_{ "models/mobile_unicorn.phai", "models/mobile_unicorn.racy", device }
		{
		}

		~impl() = default;

		std::vector<std::vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order)
		{
			init_cache(bitmaps, count, order);

			std::vector<std::vector<float>> result;
			auto network_result = mobile_unicorn_.forward(cache_ | memory::tensor_convert_to<float>);

			if (auto iter = network_result.find("fc5"); iter != network_result.end())
			{
				auto iter_fc5 = iter->second->cpu_data();

				for (std::size_t i = 0; i < count; i++)
				{
					std::vector<float> feature(feature_size);

					std::copy(iter_fc5, iter_fc5 + feature_size, feature.data());
					iter_fc5 += feature_size;
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
					std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{  static_cast<int>(count), single_bitmap_channels, single_bitmap_height, single_bitmap_width }, device_, order) :
					std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{ static_cast<int>(count), single_bitmap_height, single_bitmap_width, single_bitmap_channels }, device_, order);
		}

#ifdef USE_CUDA
			cudaMemcpy(cache_->mutable_gpu_data(), bitmaps.data(), bitmaps.size(), cudaMemcpyHostToDevice);
#else
			std::copy(bitmaps.begin(), bitmaps.end(), cache_->mutable_cpu_data());
#endif
	}

		int device_;
		excalibur::pipeline<float> mobile_unicorn_;
		std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
};

	feature_extractor_internal::feature_extractor_internal() : impl_{ new impl }
	{
	}

	feature_extractor_internal::feature_extractor_internal(int device) : impl_{ new impl{ device } }
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
