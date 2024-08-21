#include "feature_extractor_internal.hpp"

#include <algorithm>

#include <fstream>
#include <GenPipeline/GenPipeline.hpp>
#include <GenPipeline/GenPipeTools.hpp>

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
        std::vector<std::string> phai;
        impl(std::int32_t model_type, std::string_view racy_path, int device, bool use_int8) :
        impl{phai, racy_path, device}
        {
        }
		#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::string model_ext{".rknn"};
		#elif defined(USE_BMNN)
            std::string model_ext{".bmodel"};
		#else
            std::string model_ext{".onnx"};
		#endif
        impl(const std::vector<std::string> &phai, std::string_view racy_path, int device) try : device_{device}, unicorn_{std::make_unique<GenPipeline>(std::string{racy_path}+std::string("/SimpleRepUnicorn128" + model_ext), device)}
        {
			unicorn_->manual_possible_normalization({104,117,123}, {1.f / 128, 1.f / 128, 1.f / 128});
        }
        catch (...)
        {

        }

        std::vector<std::vector<float>> get(exposing::param_span<std::uint8_t> bitmaps, std::size_t count, int order)
        {
            if (bitmaps.empty())
                return {};
            
            std::vector<std::vector<float>> result;
            cv::Mat image(cv::Size(single_bitmap_width, single_bitmap_height), CV_8UC3, const_cast<uint8_t*>(bitmaps.data()));
            cv::imwrite("./cassius.jpg", image);
            auto network_result = unicorn_->forward(image).begin()->second->cpu_data();
                for (std::size_t i = 0; i < count; i++)
                {
                    std::vector<float> feature(feature_size);
                    std::copy(network_result, network_result + feature_size, feature.data());
                    network_result += feature_size;
                    result.emplace_back(feature);
                }
                std::ofstream outfile("./outfile_cassius.txt");
                // std::cout << "cassius " << std::endl;
                
				for_each(result.begin(), result.end(), [&outfile](std::vector<float> vals) {
				for_each(vals.begin(), vals.end(), [&outfile](float val) {
					// std::cout << val << std::endl;
					outfile << val << std::endl;
					});
					});
					// std::cout << std::endl << " selene end" << std::endl;
					outfile.close();
            return result;
        }

        static std::string version()
        {
            return "1.0.0";
        }

        int device_;
		std::shared_ptr<GenPipeline> unicorn_;

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
