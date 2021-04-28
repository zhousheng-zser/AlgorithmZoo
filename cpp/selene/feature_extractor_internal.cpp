#include "feature_extractor_internal.hpp"
#include "hardcode.hpp"

#include <algorithm>

#include <Excalibur/pipeline.hpp>
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
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
		impl(std::string_view general_racy_path, std::string_view id_racy_path, int device, bool use_int8) : impl{ hardcode::get_model_params("unicorn_light", use_int8), general_racy_path, id_racy_path, device }
		{
		}

		impl(const std::vector<std::string>& phai, std::string_view general_racy_path, std::string_view id_racy_path, int device) : device_{ device }, unicorn_light_general_{ phai, std::string{ general_racy_path }, device }, unicorn_light_identity_{ phai, std::string{ id_racy_path }, device }
		{
		}

		std::vector<std::vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order, bool is_id_image)
		{
			if (bitmaps.empty() || count <= 0)
			{
				return {};
			}

			init_cache(bitmaps, count, order);

			std::vector<std::vector<float>> result;
			std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> network_result;
			if(is_id_image)
				network_result = unicorn_light_identity_.forward(cache_ | memory::tensor_convert_to<float>);
			else
				network_result = unicorn_light_general_.forward(cache_ | memory::tensor_convert_to<float>);

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

		int device_;
		excalibur::pipeline<float> unicorn_light_general_;
		excalibur::pipeline<float> unicorn_light_identity_;
		std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
	};

	feature_extractor_internal::feature_extractor_internal(std::string_view general_racy_path, std::string_view id_racy_path, int device, bool use_int8) : impl_{ std::make_unique<impl>(general_racy_path, id_racy_path, device, use_int8) }
	{
	}

	feature_extractor_internal::feature_extractor_internal(const std::vector<std::string>& phai, std::string_view general_racy_path, std::string_view id_racy_path, int device) : impl_{ std::make_unique<impl>(phai, general_racy_path, id_racy_path, device) }
	{
	}

	feature_extractor_internal::~feature_extractor_internal()
	{
	}

	std::vector<std::vector<float>> feature_extractor_internal::get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order, bool is_id_image) const
	{
		return impl_->get(bitmaps, count, order, is_id_image);
	}

	std::string feature_extractor_internal::version()
	{
		return impl::version();
	}
}
