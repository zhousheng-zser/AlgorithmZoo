#include "land_code_internal.hpp"
#include "land_info_internal.hpp"
#include "land_info_impl.hpp"

#include <algorithm>
#include <numeric>
#include <iostream>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include <GenPipeline/GenPipeline.hpp>
#include "hardcode.hpp"
#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

namespace glasssix::facelandmarks
{
    class land_code_internal::impl
    {
    public:
		impl() noexcept {}

		impl(std::string_view model_directory, int device) :impl()
        {
            std::string model_dir = exposing::to_narrow_string(model_directory);
            if (*model_dir.rbegin() != '/') model_dir += '/';
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            pipeline_ = std::make_shared<GenPipeline>(model_dir + "face_landmark.rknn", 0);
#elif defined(USE_BMNN)
            pipeline_ = std::make_shared<GenPipeline>(model_dir + "face_landmark.bmodel", 0);
#else
            auto phai = get_model_params("face_landmark");
			pipeline_ = std::make_shared<GenPipeline>(phai, model_dir + "face_landmark.racy", 0);
#endif
			pipeline_->manual_possible_normalization(127.5f, 1.f / 127.5f);
        }

        facelandmarks::land_info detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
            static constexpr int infr_H = 128;
            static constexpr int infr_W = 128;
			cv::resize(image, image, { infr_W,infr_H });

            auto rst_map = pipeline_->forward(image);
            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> nodes;
            for (auto& node : rst_map) {
                nodes.emplace_back(node.second);
            }
            std::sort(nodes.begin(), nodes.end(),
                [](std::shared_ptr<glasssix::memory::tensor<float>>& A, std::shared_ptr<glasssix::memory::tensor<float>>& B) {return A->count() < B->count(); });
            float* land = nodes[1]->mutable_cpu_data();
            const size_t land_sz = nodes[1]->count();
            Softmax(nodes[0]->mutable_cpu_data(), 2);

            land_info_internal landmark;
            landmark.score = nodes[0]->mutable_cpu_data()[1];

			for (size_t i = 0; i < land_sz / 2; i++) {
                // when use cv::resize no pad, mul width & height
				landmark.pts.push_back(exposing::make_param_pair(land[2 * i] * width, land[2 * i + 1] * height));
            }

            facelandmarks::land_info result(exposing::make_as_first<land_info_impl>(landmark));
            return result;
        }

        static inline void Softmax(float* data, int num)
        {
            double L2_Sum = 0.f;
            for (size_t i = 0; i < num; i++)
            {
                data[i] = (exp(data[i]));
                L2_Sum += data[i];
            }
            for (size_t i = 0; i < num; i++)
            {
                data[i] = data[i] / L2_Sum;
            }
        }

        std::string version()
        {
            const std::string algo_module_version = "1.0.0";
            std::string nn_frame_version;
            nn_frame_version = pipeline_->version();
            return fmt::format(R"({ {"nn_frame_version":"{}", "algo_module_version" : "{}"} })", nn_frame_version, algo_module_version);
        }

    private:
        std::shared_ptr<GenPipeline> pipeline_;
    };

    land_code_internal::land_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    land_code_internal::~land_code_internal()
    {
    }

    facelandmarks::land_info land_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width)
    {
        return impl_->detect(bitmap, channels, height, width);
    }

    std::string land_code_internal::version()
    {
        return impl_->version();
    }
}
