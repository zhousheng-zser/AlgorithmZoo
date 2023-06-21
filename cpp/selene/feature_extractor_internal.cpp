#include "feature_extractor_internal.hpp"
#include "hardcode.hpp"

#include <algorithm>

#include <Excalibur/pipeline.hpp>
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <Primitives/fmt/format.h>

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

		std::array<std::tuple<int, std::string, std::string>, 4> types =
		{
			{
				{0, "unicorn_light", "unicorn_light_universal"},
				{1, "unicorn_light", "unicorn_light_id"},
				{2, "unicorn_light", "unicorn_light_universal_mask"},
				{3, "unicorn_light_union", "simple_UnicornNet_Mask"}
			}
		};
	}

	class feature_extractor_internal::impl
	{
	public:
		impl(int model_type, int device, bool use_int8) : model_type_{ model_type }, device_{ device }, use_int8_{use_int8} {}
		impl(std::string_view models_directory, std::int32_t model_type, int device, bool use_int8) : impl{ model_type, device, use_int8 }
		{
			auto model_iter = std::find_if(types.begin(), types.end(), [model_type](const std::tuple<int, std::string, std::string>& t)
				{ return std::get<0>(t) == model_type; });

			if (model_iter == types.end())
				throw exposing::abi_invalid_argument("Invalid model_type param!");

			//Excalibur needs to distinguish between float and int8 models, rknn and rknn2 does not
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			feature_extractor_instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(hardcode::get_model_params(std::get<1>(*model_iter), use_int8), std::string(models_directory) + "/" + std::get<2>(*model_iter) + ".rknn", device);
#else
			feature_extractor_instance_ = std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<1>(*model_iter), use_int8), std::string(models_directory) + "/" + std::get<2>(*model_iter) + (use_int8 ? "_int8.racy" : ".racy"), device);
#endif
		}

		std::vector<std::vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order)
		{
			if (bitmaps.empty() || count <= 0)
			{
				return {};
			}

			std::vector<std::vector<float>> result;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
#ifdef USE_RKNNAPI
			auto network_result = (*feature_extractor_instance_).forward(bitmaps.data(), { static_cast<int>(count), 3, 128, 128 }, static_cast<rknn_tensor_format>(order));
			std::string output_name = model_type_ == 3 ? "Conv_Conv_71/out0_0" : "conv5_dw_83_84";
#else
			//rknn2 can't transform order, so manual transform is needed
			std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> network_result;
			if (order == 0)
			{
				std::vector<std::uint8_t> nhwc_bitmaps(count * 3 * 128 * 128);
				for (size_t i = 0; i < count; i++)
				{
					std::uint8_t* b = bitmaps.data() + i * 3 * 128 * 128;
					std::uint8_t* g = bitmaps.data() + i * 3 * 128 * 128 + 128 * 128;
					std::uint8_t* r = bitmaps.data() + i * 3 * 128 * 128 + 2 * 128 * 128;
					for (size_t j = 0; j < 128 * 128; j++)
					{
						*(nhwc_bitmaps.data() + i * 3 * 128 * 128 + j) = *(b + j);
						*(nhwc_bitmaps.data() + i * 3 * 128 * 128 + j + 1) = *(g + j);
						*(nhwc_bitmaps.data() + i * 3 * 128 * 128 + j + 2) = *(r + j);
					}
				}
				network_result = (*feature_extractor_instance_).forward(nhwc_bitmaps.data(), { static_cast<int>(count), 3, 128, 128 }, rknn_tensor_format::RKNN_TENSOR_NHWC);
			}
			else
				network_result = (*feature_extractor_instance_).forward(bitmaps.data(), { static_cast<int>(count), 3, 128, 128 }, rknn_tensor_format::RKNN_TENSOR_NHWC);

			std::string output_name = model_type_ == 3 ? "predict" : "conv5_dw";
#endif
#else
			init_cache(bitmaps, count, order);
			auto network_result = (*feature_extractor_instance_).forward(cache_ | memory::tensor_convert_to<float>);
			std::string output_name = model_type_ == 3 ? "predict" : "conv5_dw";
#endif
			if (auto iter = network_result.find(output_name); iter != network_result.end())
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

		std::string version()
		{
			const std::string algo_module_version = "1.0.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			//#if 0
			std::string nn_frame_version = rknnwrapper::rknn_wrapper::version();
#else
			std::string nn_frame_version = excalibur::pipeline<float>::version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
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

			if (cache_->order() == memory::NHWC)
				cache_->convert_order();
		}

		int model_type_;
		int device_;
		bool use_int8_;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
		//#if 0
		std::unique_ptr<rknnwrapper::rknn_wrapper> feature_extractor_instance_;
#else
		std::unique_ptr<excalibur::pipeline<float>> feature_extractor_instance_;
#endif
		std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
	};

	feature_extractor_internal::feature_extractor_internal(std::string_view models_directory, int model_type, int device, bool use_int8) : impl_{ std::make_unique<impl>(models_directory, model_type, device, use_int8) }
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
		return impl_->version();
	}
}
