#include "feature_extractor_internal.hpp"
#include "hardcode.hpp"

#include <algorithm>

#include <Excalibur/pipeline.hpp>
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

#ifdef USE_RKNNAPI
//#if 0
#include "../../common/include/RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#endif

namespace glasssix::selene
{
	namespace
	{
		constexpr std::size_t feature_size = 256;
		constexpr std::size_t single_bitmap_width = 128;
		constexpr std::size_t single_bitmap_height = 128;
		constexpr std::size_t single_bitmap_channels = 3;
		constexpr std::size_t single_bitmap_bytes = single_bitmap_channels * single_bitmap_width * single_bitmap_height;
	}

	class feature_extractor_internal::impl
	{
	public:
		impl(std::string_view racy_path, std::int32_t model_type, int device, bool use_int8) : impl{ hardcode::get_model_params("unicorn_light", use_int8), racy_path, model_type, device }
		{
		}

		impl(const std::vector<std::string>& phai, std::string_view racy_path, std::int32_t model_type, int device) : device_{ device }, unicorn_light_{ phai, std::string{ racy_path }, device }, model_type_(model_type)
		{
		}

		std::vector<std::vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order)
		{
			if (bitmaps.empty() || count <= 0)
			{
				return {};
			}

			std::vector<std::vector<float>> result;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			auto network_result = unicorn_light_.forward(bitmaps.data(), { static_cast<int>(count), 3, 128, 128 }, static_cast<rknn_tensor_format>(order));
#ifdef USE_RKNNAPI
			if (auto iter = network_result.find("conv5_dw_83_84"); iter != network_result.end())
#else
			if (auto iter = network_result.find("conv5_dw"); iter != network_result.end())
#endif
#else
			init_cache(bitmaps, count, order);
			auto network_result = unicorn_light_.forward(cache_ | memory::tensor_convert_to<float>);
			if (auto iter = network_result.find("conv5_dw"); iter != network_result.end())
#endif
			{
				auto iter_conv5 = iter->second->cpu_data();

				for (std::size_t i = 0; i < count; i++)
				{
					std::vector<float> feature(feature_size);

					std::copy(iter_conv5, iter_conv5 + feature_size, feature.data());
					iter_conv5 += feature_size;
					result.emplace_back(feature);
				}
			}

			return result;
		}

		std::int32_t get_model_type()
		{
			return model_type_;
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
					std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{ static_cast<int>(count), single_bitmap_height, single_bitmap_width, single_bitmap_channels }, -1, static_cast<memory::orderType>(order)/*, & memory::pool_allocator_default<std::uint8_t>::get()*/);
			}
			if (cache_->device() > 0)
			{
#ifdef USE_CUDA
				cudaMemcpy(cache_->mutable_gpu_data(), bitmaps.data(), bitmaps.size(), cudaMemcpyHostToDevice);
#else
				NO_GPU;
#endif
			}

			std::copy(bitmaps.begin(), bitmaps.end(), cache_->mutable_cpu_data());
		}

		int model_type_;
		int device_;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
		//#if 0
		rknnwrapper::rknn_wrapper unicorn_light_;
#else
		glasssix::excalibur::pipeline<float> unicorn_light_;
#endif
		std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
	};

	feature_extractor_internal::feature_extractor_internal(std::string_view racy_path, std::int32_t model_type, int device, bool use_int8) : impl_{ std::make_unique<impl>(racy_path, model_type, device, use_int8) }
	{
	}

	feature_extractor_internal::feature_extractor_internal(const std::vector<std::string>& phai, std::string_view racy_path, std::int32_t model_type, int device) : impl_{ std::make_unique<impl>(phai, racy_path, model_type, device) }
	{
	}

	feature_extractor_internal::~feature_extractor_internal()
	{
	}

	std::vector<std::vector<float>> feature_extractor_internal::get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order) const
	{
		return impl_->get(bitmaps, count, order);
	}

	std::int32_t feature_extractor_internal::get_model_type()
	{
		return impl_->get_model_type();
	}

	std::string feature_extractor_internal::version()
	{
		return impl::version();
	}
}
