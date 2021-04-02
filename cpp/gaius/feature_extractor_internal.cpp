#include "feature_extractor_internal.hpp"
#include "hardcode.hpp"

#include <vector>
#include <algorithm>

#include <Excalibur/pipeline.hpp>
#include <Primitives/pool_allocator.hpp>
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
		impl(std::string_view racy_path, std::string_view mask_racy_path, int device, bool use_int8) : impl{ hardcode::get_model_params("mobile_unicorn", use_int8), std::string{ racy_path }, hardcode::get_model_params("mobile_unicorn_mask", use_int8), std::string{ mask_racy_path }, device }
		{
		}

		impl(const std::vector<std::string>& phai, std::string_view racy_path, const std::vector<std::string>& mask_phai, std::string_view mask_racy_path, int device) : device_{ device }, mobile_unicorn_{ phai, std::string{ racy_path }, device }, mask_mobile_unicorn_{ mask_phai, std::string{ mask_racy_path }, device }
		{
		}

		std::vector<std::vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order, bool has_mask)
		{
			if (bitmaps.empty() || count <= 0)
			{
				return {};
			}

			init_cache(bitmaps, count, order);

			std::vector<std::vector<float>> result;
			std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> network_result;
			
			if(has_mask)
				network_result = mask_mobile_unicorn_.forward(cache_ | memory::tensor_convert_to<float>);
			else
				network_result = mobile_unicorn_.forward(cache_ | memory::tensor_convert_to<float>);

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
					std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{  static_cast<int>(count), single_bitmap_channels, single_bitmap_height, single_bitmap_width }, -1, static_cast<memory::orderType>(order)/*, &memory::pool_allocator_default<std::uint8_t>::get()*/) :
					std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{ static_cast<int>(count), single_bitmap_height, single_bitmap_width, single_bitmap_channels }, -1, static_cast<memory::orderType>(order)/*, &memory::pool_allocator_default<std::uint8_t>::get()*/);
			}
			if (cache_->device() > 0)
			{
#ifdef USE_CUDA
				cudaMemcpy(cache_->mutable_gpu_data(), bitmaps.data(), bitmaps.size(), cudaMemcpyHostToDevice);
#else
				NO_GPU;
#endif
			}
			else
				std::copy(bitmaps.begin(), bitmaps.end(), cache_->mutable_cpu_data());
		}

		int device_;
		excalibur::pipeline<float> mobile_unicorn_;
		excalibur::pipeline<float> mask_mobile_unicorn_;
		std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
	};

	feature_extractor_internal::feature_extractor_internal(std::string_view racy_path, std::string_view mask_racy_path, int device, bool use_int8) : impl_{ std::make_unique<impl>(racy_path, mask_racy_path, device, use_int8) }
	{
	}

	feature_extractor_internal::feature_extractor_internal(const std::vector<std::string>& phai, std::string_view racy_path, const std::vector<std::string>& mask_phai, std::string_view mask_racy_path, int device) : impl_{ std::make_unique<impl>(phai, racy_path, mask_phai, mask_racy_path, device) }
	{
	}

	feature_extractor_internal::~feature_extractor_internal()
	{
	}

	std::vector<std::vector<float>> feature_extractor_internal::get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order, bool has_mask) const
	{
		return impl_->get(bitmaps, count, order, has_mask);
	}

	std::string feature_extractor_internal::version()
	{
		return impl::version();
	}
}
