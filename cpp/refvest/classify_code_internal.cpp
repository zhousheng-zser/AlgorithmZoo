#include <iostream>
#include <cmath>
#include <tuple>
#include <utility>
#include "logger.hpp"
#include <abi/param_vector.hpp>

#include "classify_code_internal.hpp"
#include "box_info_impl.hpp"
#include "hardcode.hpp"
#include "../posture/detect_code.hpp"

#include <opencv2/opencv.hpp>
#include <GenPipeline/GenPipeline.hpp>
#include <GenPipeline/GenPipeTools.hpp>


namespace glasssix::refvest
{
    class classify_code_internal::impl
    {
    public:
        impl() noexcept {}
        impl(std::string_view model_directory, int device) :impl()
        {
            std::string model_directory_ = exposing::to_narrow_string(model_directory);
            if (*model_directory_.rbegin() != '/') model_directory_ += '/';

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            classify_instance_ = std::make_unique<GenPipeline>(model_directory_ + "refvest_cls.rknn", 0);
#elif defined(USE_BMNN)
            classify_instance_ = std::make_unique<GenPipeline>(model_directory_ + "refvest_cls.bmodel", 0);
#else
            classify_instance_ = std::make_unique<GenPipeline>(model_directory_ + "refvest_cls.onnx", 0);
#endif
            constexpr float stand = 1.f / 255;
            classify_instance_->manual_possible_normalization({ 0,0,0 }, { stand,stand,stand });
        }  

        exposing::param_vector<refvest::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map)
        {

            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.8f;
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            auto result = exposing::make_param_vector<box_info>();

            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
            for (auto pinfo : posture_info_list)
            {
                PostureInfo postureInfo{ pinfo };

                if (postureInfo.if_bodyerr()) {
                    continue;
                }

                auto vest_cls_rect = postureInfo.get_vest_det_region();
                if (vest_cls_rect.height <= 1 || vest_cls_rect.width <= 1) continue; //invalid input
                auto vest_cls_region = GenPipeTools::safty_cut(image, vest_cls_rect);

                cv::resize(vest_cls_region, vest_cls_region, cv::Size2i{ 128, 128 });

                cv::cvtColor(vest_cls_region, vest_cls_region, cv::COLOR_BGR2RGB);

                auto vest_cls_rst_map = classify_instance_->forward(vest_cls_region);
                auto vest_cls_rst = vest_cls_rst_map.begin()->second;
                auto vest_cls_scores = vest_cls_rst->cpu_data();
                float no_refvest_score = vest_cls_scores[0];
                float is_refvest_score = vest_cls_scores[1];

                static constexpr int NO_REF_VEST = 0;
                static constexpr int IS_REF_VEST = 1;
                static constexpr float IS_REF_THRESH = con_thres;

                refvest::box_info_internal item;
                item.x1 = postureInfo.xmin;
                item.x2 = postureInfo.xmax;
                item.y1 = postureInfo.ymin;
                item.y2 = postureInfo.ymax;
				item.score = is_refvest_score;

				item.category = NO_REF_VEST;
				if (is_refvest_score > IS_REF_THRESH && is_refvest_score > no_refvest_score) {
                    item.category = IS_REF_VEST;
                }
                result.push_back(exposing::make_as_first<box_info_impl>(item));
            }
     
            return result;
            
        }

        std::string version()
        {
			const std::string algo_module_version = "3.0.0";
			std::string nn_frame_version = classify_instance_->version();
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    private:
        std::unique_ptr<GenPipeline> classify_instance_;
    };

    classify_code_internal::classify_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    classify_code_internal::~classify_code_internal() = default;

    std::string classify_code_internal::version()
    {
        return impl_->version();
    }

    exposing::param_vector<refvest::box_info> classify_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, posture_info_list, param_map);
    }
}
