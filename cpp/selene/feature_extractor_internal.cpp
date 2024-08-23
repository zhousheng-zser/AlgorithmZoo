#include "feature_extractor_internal.hpp"
#include "hardcode.hpp"
#include <fstream>
#include <algorithm>
#include <GenPipeline/GenPipeline.hpp>
#include <GenPipeline/GenPipeTools.hpp>
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
			feature_extractor_instance_ = std::make_unique<GenPipeline>(get_model_params(std::get<1>(*model_iter), use_int8), std::string(models_directory) + "/" + std::get<2>(*model_iter) + ".rknn", device);
#else
			feature_extractor_instance_ = std::make_unique<GenPipeline>(get_model_params(std::get<1>(*model_iter), use_int8), std::string(models_directory) + "/" + std::get<2>(*model_iter) + (use_int8 ? "_int8.bmodel" : ".bmodel"), device);
#endif
		}

		std::vector<std::vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order)
		{
			if (bitmaps.empty() || count <= 0)
			{
				return {};
			}

			std::vector<std::uint8_t> temp_data;
			if (model_type_ == 3)
			{
				temp_data.resize(bitmaps.size());
				std::copy(bitmaps.begin(), bitmaps.end(), temp_data.begin());
				if (order == 0)
					convert_bgr2rgb_nchw(temp_data.data(), count);
				else if (order == 1)
					convert_bgr2rgb_nhwc(temp_data.data(), count);

				bitmaps = exposing::param_span<std::uint8_t>(temp_data.data(), temp_data.size());
			}
			cv::Mat image(cv::Size(single_bitmap_width, single_bitmap_height), CV_8UC3, const_cast<uint8_t*>(temp_data.data()));

			std::vector<std::vector<float>> result;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
#ifdef USE_RKNNAPI

			auto network_result = (*feature_extractor_instance_).forward(image);
			//std::string output_name = model_type_ == 3 ? "Conv_Conv_71/out0_0" : "conv5_dw_83_84";
#elif defined(USE_RKNN2API)
			//rknn2 can't transform order, so manual transform is needed
			std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> network_result;
			if (order == 0)
			{
				constexpr std::size_t stride_c = single_bitmap_width * single_bitmap_height;
				std::vector<std::uint8_t> nhwc_bitmaps(count * single_bitmap_bytes);
				for (size_t i = 0; i < count; i++)
				{
					std::uint8_t* c1 = bitmaps.data() + i * single_bitmap_bytes;
					std::uint8_t* c2 = bitmaps.data() + i * single_bitmap_bytes + stride_c;
					std::uint8_t* c3 = bitmaps.data() + i * single_bitmap_bytes + 2 * stride_c;
					for (size_t j = 0; j < stride_c; j++)
					{
						nhwc_bitmaps[i * single_bitmap_bytes + j * single_bitmap_channels] = c1[j];
						nhwc_bitmaps[i * single_bitmap_bytes + j * single_bitmap_channels + 1] = c2[j];
						nhwc_bitmaps[i * single_bitmap_bytes + j * single_bitmap_channels + 2] = c3[j];
					}
				}
				cv::Mat image(cv::Size(single_bitmap_width, single_bitmap_height), CV_8UC3, const_cast<uint8_t*>(nhwc_bitmaps.data()));
				// cv::Mat image_fix = image(cv::Size(single_bitmap_width, single_bitmap_height));//这是错误的写法
				network_result = (*feature_extractor_instance_).forward(image);
			}
			else
				network_result = (*feature_extractor_instance_).forward(image);

			//std::string output_name = model_type_ == 3 ? "predict" : "conv5_dw";
#endif
#else
			// init_cache(bitmaps, count, order);
			// auto network_result = (*feature_extractor_instance_).forward(cache_ | memory::tensor_convert_to<float>);
			
			auto network_result = (*feature_extractor_instance_).forward(image);
			//std::string output_name = model_type_ == 3 ? "predict" : "conv5_dw";
#endif
			// std::ofstream outfile("./outfile.txt");
			CHECK_EQ(1, network_result.size());
			auto node = *network_result.begin();
			auto iter_conv5 = node.second->cpu_data();

			for (std::size_t i = 0; i < count; i++)
			{
				std::vector<float> feature(feature_size);

				std::copy(iter_conv5, iter_conv5 + feature_size, feature.data());
				iter_conv5 += feature_size;
				result.emplace_back(feature);
			}
			// std::cout << " selene " << std::endl;
			// 	for_each(result.begin(), result.end(), [&outfile](std::vector<float> vals) {
			// 	for_each(vals.begin(), vals.end(), [&outfile](float val) {
			// 		std::cout << val << std::endl;
			// 		outfile << val << std::endl;
			// 		});
			// 		});
			// 		std::cout << std::endl << " selene end" << std::endl;
			// 		outfile.close();
			return result;
		}

		std::int32_t get_model_type()
		{
			return model_type_;
		}

		std::string version()
		{
			const std::string algo_module_version = "1.0.0";

			std::string nn_frame_version = feature_extractor_instance_->version();
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

		void convert_bgr2rgb_nchw(std::uint8_t* data, size_t count)
		{
			constexpr std::size_t stride_c = single_bitmap_width * single_bitmap_height;
			for (size_t i = 0; i < count; i++)
			{
				std::uint8_t* ptr = data + i * single_bitmap_bytes;
				std::vector<std::uint8_t> temp(stride_c);
				std::copy(ptr, ptr + stride_c, temp.data());
				std::copy(ptr + 2 * stride_c, ptr + single_bitmap_bytes, ptr);
				std::copy(temp.data(), temp.data() + stride_c, ptr + 2 * stride_c);
			}
		}

		void convert_bgr2rgb_nhwc(std::uint8_t* data, size_t count)
		{
			constexpr std::size_t stride_w = single_bitmap_width * single_bitmap_channels;
			for (size_t i = 0; i < count; i++)
			{
				std::uint8_t* ptr = data + i * single_bitmap_bytes;
				for (size_t h = 0; h < single_bitmap_height; h++)
				{
					for (size_t w = 0; w < single_bitmap_width; w++)
					{
						ptr[h * stride_w + w * single_bitmap_channels + 0] = ptr[h * stride_w + w * single_bitmap_channels + 0] ^ ptr[h * stride_w + w * single_bitmap_channels + 2];
						ptr[h * stride_w + w * single_bitmap_channels + 2] = ptr[h * stride_w + w * single_bitmap_channels + 0] ^ ptr[h * stride_w + w * single_bitmap_channels + 2];
						ptr[h * stride_w + w * single_bitmap_channels + 0] = ptr[h * stride_w + w * single_bitmap_channels + 0] ^ ptr[h * stride_w + w * single_bitmap_channels + 2];
					}
				}
			}
		}

		int model_type_;
		int device_;
		bool use_int8_;
		std::unique_ptr<GenPipeline> feature_extractor_instance_;
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
