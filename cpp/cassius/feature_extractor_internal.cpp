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
            
            std::vector<std::vector<float>> result;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::array<std::uint8_t,128*128*3> face_nhwc;
            if( order == 0 ) 
                for (size_t h = 0; h < 128; h++)
                    for (size_t w = 0; w < 128; w++)
                    {
                        face_nhwc[h*128*3 + w * 3 + 0] = bitmaps[0 * 128 * 128 + h * 128 + w];
                        face_nhwc[h*128*3 + w * 3 + 1] = bitmaps[1 * 128 * 128 + h * 128 + w];
                        face_nhwc[h*128*3 + w * 3 + 2] = bitmaps[2 * 128 * 128 + h * 128 + w];
                    }
#ifdef USE_RKNNAPI
            auto network_result = unicorn_.forward(face_nhwc.data(), { static_cast<int>(count), 128, 128, 3 }, static_cast<rknn_tensor_format>(order));
            if (auto iter = network_result.find("dequantize_at_636_107_out0_108"); iter != network_result.end())
#else
            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> network_result;
            network_result = unicorn_.forward(face_nhwc.data(), { static_cast<int>(count), 128, 128, 3 }, rknn_tensor_format::RKNN_TENSOR_NHWC);
            if (auto iter = network_result.find("834"); iter != network_result.end())
#endif
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

        int device_;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        rknnwrapper::rknn_wrapper unicorn_;
#endif
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
