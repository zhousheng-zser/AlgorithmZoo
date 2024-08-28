#include "feature_extractor_internal.hpp"
#include "hardcode.hpp"

#include <algorithm>

#ifdef USE_RKNNAPI
#include "RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "RKNN2Wrapper/rknn2_wrapper.hpp"
#else
#include <Excalibur/pipeline.hpp>
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


    void nchw_to_nhwc(const std::uint8_t* sou_data, std::uint8_t* dst_data, int N, int C, int H, int W) 
    {
        for (int n = 0; n < N; ++n) 
            for (int h = 0; h < H; ++h) 
                for (int w = 0; w < W; ++w) 
                    for (int c = 0; c < C; ++c) {
                        // NCHW index
                        int nchw_index = n * C * H * W + c * H * W + h * W + w;
                        // NHWC index
                        int nhwc_index = n * H * W * C + h * W * C + w * C + c;
                        dst_data[nhwc_index] = sou_data[nchw_index];
                    }
        
    }

    class feature_extractor_internal::impl
    {
    public:
        impl(std::int32_t model_type, std::string_view racy_path, int device, bool use_int8) :
        impl{get_model_params(model_type ? "SimpleRepUnicorn" : "SimpleRepUnicorn", use_int8), racy_path, device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string_view racy_path, int device) try : device_{device}, unicorn_{phai, std::string{racy_path}+std::string("/SimpleRepUnicorn128.rknn"), device}
        {
        }
        catch (...)
        {

        }

        std::vector<std::vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order)
        {
            if (bitmaps.empty())
                return {};
            // std::span<std::uint8_t> NHWC_DATA(3*128*128);
            std::uint8_t NHWC_DATA[count*3*128*128];
            std::vector<std::vector<float>> result;
            nchw_to_nhwc(bitmaps.data(), NHWC_DATA,count, 3,128, 128  );

#ifdef USE_RKNN2API
            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> network_result;
            network_result = unicorn_.forward(NHWC_DATA, { static_cast<int>(count), 128, 128, 3 }, rknn_tensor_format::RKNN_TENSOR_NHWC);
            if (auto iter = network_result.find("predict"); iter != network_result.end())
#else
            auto network_result = unicorn_.forward(NHWC_DATA, { static_cast<int>(count), 128, 128, 3 }, static_cast<rknn_tensor_format>(order));
            if (auto iter = network_result.find("dequantize_at_636_107_out0_108"); iter != network_result.end())
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
