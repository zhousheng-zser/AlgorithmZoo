#include "deepMarMobileNet_net_internal.hpp"
#include "hardcode.hpp"

#include <fstream>
#include <algorithm>

#include <Excalibur/pipeline.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_resize.hpp>

#include "Primitives/tensor_conversions.hpp"
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>

#include <cfloat>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

namespace glasssix::rifleman
{
    template <typename T>
    std::shared_ptr < glasssix::memory::tensor<T>> operator-(std::shared_ptr<glasssix::memory::tensor<T>>& tensor, float x)
    {
        for (int i = 0; i < tensor->count(); ++i) {
            (*tensor)[i] -= x;
        }
        return tensor;
    }

    template <typename T>
    std::shared_ptr<glasssix::memory::tensor<T>> operator/(std::shared_ptr< glasssix::memory::tensor<T>>& tensor, float x)
    {
        for (int i = 0; i < tensor->count(); ++i) {
            (*tensor)[i] /= x;
        }
        return tensor;
    }

    class deepMarMobileNet_net_internal::impl
    {
    public:
        impl(std::string_view deepMarMobileNet_racy_path, int device) 
            : impl{ hardcode::get_model_params("deepMarMobileNet"), deepMarMobileNet_racy_path, device }
        {
        }

        impl(std::string_view deepMarMobileNet_phai, std::string_view deepMarMobileNet_racy_path, int device)
            : device_{ device }, deepMarMobileNet_instance_{ std::string{deepMarMobileNet_phai}, std::string{deepMarMobileNet_racy_path}, device }
        {
        }

        impl(const std::vector<std::string>& deepMarMobileNet_phai, std::string_view deepMarMobileNet_racy_path,  int device)
            : device_{ device }, deepMarMobileNet_instance_{ deepMarMobileNet_phai, std::string{deepMarMobileNet_racy_path}, device }
        {
        }
        
        inline float sigmoid(float x)
        {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

        template <typename DataType>
        void normalize(std::shared_ptr<memory::tensor<DataType>> tensor, float mean, float std)
        {
            *tensor = (*tensor - mean) / std; 
        }
        
        template <typename T>
        void normalize(std::shared_ptr<memory::tensor<T>> tensor, const std::vector<float>& mean, const std::vector<float>& std)
        {
            int batchsize = tensor->num();
            int channels = tensor->channels();
            int height = tensor->height();
            int width = tensor->width();

            if (tensor->order() == memory::orderType::NCHW)
            {
                for (int batch = 0; batch < batchsize; ++batch)
                {
                    for (int channel = 0; channel < channels; ++channel) 
                    {
                        float in_mean = mean[channel];
                        float in_std = std[channel];

                        for (int row = 0; row < height; ++row)
                        {
                            for (int col = 0; col < width; ++col)
                            {
                                size_t idx = size_t(0
                                    + batch * channels * height * width 
                                    + channel * height * width 
                                    + row * width 
                                    + col);

                                (*tensor)[idx] = ((*tensor)[idx] - in_mean) / in_std;
                            }
                        }
                    }
                }
            }
            else //NHWC
            {
                for (int batch = 0; batch < batchsize; ++batch)
                {
                    for (int row = 0; row < height; ++row)
                    {
                        for (int col = 0; col < width; ++col)
                        {
                            for (int channel = 0; channel < channels; ++channel)
                            {
                                float in_mean = mean[channel];
                                float in_std = std[channel];

                                size_t idx = (size_t)(0
                                    + batch * height * width * channels
                                    + row * width * channels
                                    + col * channels
                                    + channel);

                                (*tensor)[idx] = ((*tensor)[idx] - in_mean) / in_std;
                            }
                        }
                    }
                }
            }
        }

        void extract_feature(int channels, int height, int width, int order, std::vector<std::vector<float>>& batch_results) 
        {
            // resize
            int w = 224, h = 224;
            std::shared_ptr<memory::tensor<std::uint8_t>> cache_forward;
            excalibur::resize_cpu(cache_, cache_forward, h, w);
            std::shared_ptr<memory::tensor<std::uint8_t>> cache_img;
            excalibur::resize_cpu(cache_, cache_img, 256, 512);

            for (int i = 0; i < 10; ++i)
            {
                std::cout << (int)(cache_forward->cpu_data()[i]) << std::endl;
            }

            auto input_tensor = cache_forward | memory::tensor_convert_to<float>;
            input_tensor = input_tensor / 255.0;

            std::cout << "src data :  ------------------------------" << std::endl;
            for (int i = 0; i < 10; ++i)
            {
                std::cout << input_tensor->cpu_data()[i] << std::endl;
            }
            for (int i = 100352; i < 100352 + 10; ++i)
            {
                std::cout << input_tensor->cpu_data()[i] << std::endl;
            }
            
            // normalize
            std::vector <float > mean = { 0.485, 0.456, 0.406 };
            std::vector <float > var = { 0.229, 0.224, 0.225 };
            normalize(input_tensor, mean, var);

            std::cout << "normalized data :  ------------------------------" << std::endl;
            for (int i = 0; i < 10; ++i)
            {
                std::cout << input_tensor->cpu_data()[i] << std::endl;
            }
            for (int i = 100352; i < 100352 + 10; ++i)
            {
                std::cout << input_tensor->cpu_data()[i] << std::endl;
            }

            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> out = deepMarMobileNet_instance_.forward(input_tensor);
            auto result = out["output"];

            // out
            size_t step = (size_t)(result->data_shape()[1]);
            for (size_t i = 0; i < result->num(); ++i) {
                std::vector<float> per_result;
                
                const float* per_img_data = result->cpu_data() + i * step;
                
                std::copy(per_img_data, per_img_data + step, std::back_inserter(per_result));
                batch_results.push_back(per_result);
            }

        }

        exposing::param_vector<exposing::param_vector<exposing::param_pair<float, exposing::param_string>>>
            detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
        {
            // bgr 2 rgb
            for (int row = 0; row < height; ++row)
            {
                for (int col = 0; col < width; ++col)
                {
                    int step0 = width * channels;
                    int step1 = channels;
                    
                    auto temp = bitmap.data()[step0 * row + step1 * col + 0];
                    bitmap.data()[step0 * row + step1 * col + 0] = bitmap.data()[step0 * row + step1 * col + 2];
                    bitmap.data()[step0 * row + step1 * col + 2] = temp;
                }
            }

            this->init_cache(bitmap, channels, height, width, order);


            std::vector<std::vector<float>> results;
            extract_feature(channels, height, width, order, results);

            //¡Ÿ ±
            std::vector<std::string> labels = {"Female", "AgeOver60", "Age18 - 60","AgeLess18", "Front", "Side", "Back", "Hat", "Glasses","HandBag", "ShoulderBag", "Backpack", "HoldObjectsInFront", 
                "ShortSleeve", "LongSleeve","UpperStride", "UpperLogo", "UpperPlaid", "UpperSplice", "LowerStripe", "LowerPattern","LongCoat", "Trousers", "Shorts", "Skirt & Dress", "boots" };

            auto result_batchs = exposing::make_param_vector<exposing::param_vector<exposing::param_pair<float, exposing::param_string>>>();
            for (auto result : results)
            {
                auto result_per = exposing::make_param_vector<exposing::param_pair<float, exposing::param_string>>();

                assert(result.size() == labels.size());
                for (int i = 0; i < result.size(); ++i)
                {
                    auto temp = result[i];
                    if (result[i] >= 0.0)
                    {
                        auto record = exposing::make_param_pair((float)result[i], exposing::param_string(labels[i]));
                        //auto record = exposing::make_param_pair<double, exposing::param_string>((double)result[i], exposing::param_string(labels[i]));
                        result_per.push_back(record);
                    }
                }

                result_batchs.push_back(result_per);
            }
            
            return result_batchs;
        }

        static std::string version()
        {
            return "1.0.0";
        }

    private:

        void init_cache(exposing::param_span<std::uint8_t>& bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order)
        {
            if (cache_ == nullptr || cache_->channels() != channels || cache_->height() != height || cache_->width() != width || cache_->order() != order)
            {
                std::vector<int> shape;
                if (order == memory::NCHW)
                    shape = { static_cast<int>(1), channels, height, width };
                else if (order == memory::NHWC)
                    shape = { static_cast<int>(1), height, width, channels };
                else
                    NOT_IMPLEMENTED;

                cache_ = std::make_shared<memory::tensor<std::uint8_t>>(shape, -1, (memory::orderType)order/*, &memory::pool_allocator_default<std::uint8_t>::get()*/);
            }

            if (cache_->device() > 0)
            {
#ifdef USE_CUDA
                cudaMemcpy(cache->mutable_gpu_data(), bitmap, channels * height * width, cudaMemcpyHostToDevice);
#else
                NO_GPU;
#endif
            }
            else
                std::copy(bitmap.begin(), bitmap.end(), cache_->mutable_cpu_data());

            if (order == memory::NHWC)
                cache_->convert_order();
        }

    private:
        int device_;
        excalibur::pipeline<float> deepMarMobileNet_instance_;

        std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
    };

    deepMarMobileNet_net_internal::deepMarMobileNet_net_internal(std::string_view deepMarMobileNet_racy_path, int device) 
        : impl_{ std::make_unique<impl>(deepMarMobileNet_racy_path, device) }
    {
    }

    deepMarMobileNet_net_internal::deepMarMobileNet_net_internal(std::string_view deepMarMobileNet_phai, std::string_view deepMarMobileNet_racy_path, int device)
        : impl_{ std::make_unique<impl>(deepMarMobileNet_phai, deepMarMobileNet_racy_path, device) }
    {
    }

    deepMarMobileNet_net_internal::deepMarMobileNet_net_internal(const std::vector<std::string>& deepMarMobileNet_phai, std::string_view deepMarMobileNet_racy_path, int device) 
        : impl_{ std::make_unique<impl>(deepMarMobileNet_phai, deepMarMobileNet_racy_path, device) }
    {
    }

    deepMarMobileNet_net_internal::deepMarMobileNet_net_internal()
    {
    }

    deepMarMobileNet_net_internal::~deepMarMobileNet_net_internal()
    {
    }

    std::string deepMarMobileNet_net_internal::version()
    {
        return impl::version();
    }

    exposing::param_vector<exposing::param_vector<exposing::param_pair<float, exposing::param_string>>> deepMarMobileNet_net_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
    {
        return impl_->detect(bitmap, channels, height, width, order);
    }
}