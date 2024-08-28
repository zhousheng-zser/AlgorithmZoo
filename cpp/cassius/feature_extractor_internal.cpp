#include "feature_extractor_internal.hpp"
#include "hardcode.hpp"

#include <algorithm>
#include <fstream>

#ifdef USE_RKNNAPI
#include "RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "RKNN2Wrapper/rknn2_wrapper.hpp"
#else
#include <Excalibur/pipeline.hpp>
#endif
#ifdef USE_BMNN
#include <fstream>
#include <GenPipeline/GenPipeline.hpp>
#include <GenPipeline/GenPipeTools.hpp>
#endif
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


    void nchw_to_nhwc(const std::uint8_t* sou_data, std::uint8_t* dst_data, int C, int H, int W) 
    {
        
            for (int h = 0; h < H; ++h) 
                for (int w = 0; w < W; ++w) 
                    for (int c = 0; c < C; ++c) {
                        int nchw_index =  c * H * W + h * W + w;
                        int nhwc_index =  h * W * C + w * C + c;
                        dst_data[nhwc_index] = sou_data[nchw_index];
                    }
    }

    class feature_extractor_internal::impl
    {
    public:
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        impl(std::int32_t model_type, std::string_view racy_path, int device, bool use_int8) :
        impl{get_model_params(model_type ? "SimpleRepUnicorn" : "SimpleRepUnicorn", use_int8), racy_path, device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string_view racy_path, int device) try : device_{device}, unicorn_{phai, std::string{racy_path}+std::string("/SimpleRepUnicorn128.rknn"), device}
        {
        }
#else
        std::vector<std::string> phai;
        impl(std::int32_t model_type, std::string_view racy_path, int device, bool use_int8) :
        impl{phai, racy_path, device}
        {
        }
        std::string model_ext{".bmodel"};

        impl(const std::vector<std::string> &phai, std::string_view racy_path, int device) try : device_{device}, unicorn_{std::make_unique<GenPipeline>(std::string{racy_path}+std::string("/SimpleRepUnicorn128" + model_ext), device)}
        {
			unicorn_->manual_possible_normalization({104,117,123}, {1.f / 128, 1.f / 128, 1.f / 128});
        }
#endif
        catch (...)
        {

        }

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::vector<std::vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order)
        {
            if (bitmaps.empty())
                return {};

            // std::span<std::uint8_t> NHWC_DATA(3*128*128);
            std::uint8_t NHWC_DATA[3*128*128];
            std::vector<std::vector<float>> result;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            nchw_to_nhwc(bitmaps.data(), NHWC_DATA, 3,128, 128  );
#ifdef USE_RKNNAPI
            auto network_result = unicorn_.forward(NHWC_DATA, { static_cast<int>(count), 128, 128, 3 }, static_cast<rknn_tensor_format>(order));
            if (auto iter = network_result.find("dequantize_at_636_107_out0_108"); iter != network_result.end())
#else
            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> network_result;
            network_result = unicorn_.forward(NHWC_DATA, { static_cast<int>(count), 128, 128, 3 }, rknn_tensor_format::RKNN_TENSOR_NHWC);
            if (auto iter = network_result.find("predict"); iter != network_result.end())
#endif
#else
            init_cache(bitmaps, count, order);
            auto network_result = unicorn_.forward(cache_ | memory::tensor_convert_to<float>);
            if (auto iter = network_result.find("834"); iter != network_result.end())
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
#else
        std::vector<std::vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order)
        {
            if (bitmaps.empty())
                return {};
            
            std::vector<std::vector<float>> result;
            cv::Mat image(cv::Size(single_bitmap_width, single_bitmap_height), CV_8UC3, const_cast<uint8_t*>(bitmaps.data()));
            // cv::imwrite("./cassius.jpg", image);
            auto network_result = unicorn_->forward(image).begin()->second->cpu_data();
                for (std::size_t i = 0; i < count; i++)
                {
                    std::vector<float> feature(feature_size);
                    std::copy(network_result, network_result + feature_size, feature.data());
                    network_result += feature_size;
                    result.emplace_back(feature);
                }
                // std::ofstream outfile("./outfile_cassius.txt");
                // // std::cout << "cassius " << std::endl;
                 
				// for_each(result.begin(), result.end(), [&outfile](std::vector<float> vals) {
				// for_each(vals.begin(), vals.end(), [&outfile](float val) {
				// 	// std::cout << val << std::endl;
				// 	outfile << val << std::endl;
				// 	});
				// 	});
				// 	// std::cout << std::endl << " selene end" << std::endl;
				// 	outfile.close();
            return result;
        }
#endif

        static std::string version()
        {
            return "1.0.0";
        }

    private:
        void init_cache(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order)
        {
            if (cache_ == nullptr || cache_->num() != count || cache_->order() != order)
            {
                cache_ = order == memory::NCHW ? std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{static_cast<int>(count), single_bitmap_channels, single_bitmap_height, single_bitmap_width}, -1, static_cast<memory::orderType>(order) /*, &memory::pool_allocator_default<std::uint8_t>::get()*/) : std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{static_cast<int>(count), single_bitmap_height, single_bitmap_width, single_bitmap_channels}, -1, static_cast<memory::orderType>(order) /*, & memory::pool_allocator_default<std::uint8_t>::get()*/);
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

        int device_;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        rknnwrapper::rknn_wrapper unicorn_;
#elif defined(USE_BMNN)
		std::shared_ptr<GenPipeline> unicorn_;
#else
        excalibur::pipeline<float> unicorn_;
#endif
        std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
    };

    feature_extractor_internal::feature_extractor_internal(std::int32_t model_type, std::string_view racy_path, int device, bool use_int8) : 
    impl_{std::make_unique<impl>(model_type, racy_path, device, use_int8)}
    {
    }

    feature_extractor_internal::feature_extractor_internal(const std::vector<std::string> &phai, std::string_view racy_path, int device) : impl_{std::make_unique<impl>(phai, racy_path, device)}
    {
    }

    feature_extractor_internal::~feature_extractor_internal()
    {
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
