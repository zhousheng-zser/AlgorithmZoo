#include <iostream>
#include <cmath>

#include "classify_code_internal.hpp"
#include "box_info_impl.hpp"
#include <Excalibur/pipeline.hpp>
#include <Primitives/tensor_conversions.hpp>
#include "logger.hpp"

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#ifdef BUILD_DEBUG_INFO
#include <opencv2/highgui/highgui.hpp>
#endif // BUILD_DEBUG_INFO

#include <abi/param_vector.hpp>
#include <Primitives/fmt/format.h>
#include <utility>

#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include <GenPipeline/GetPostprocessing.hpp>
#include "../genpipeline/market/yolov8_GEN.hpp"

namespace glasssix::pedestrian
{
    class classify_code_internal::impl
    {
    public:
		impl() {}

        impl(const exposing::param_string model_directory, int device = -1):impl()
        {
            std::string model_dir = exposing::to_narrow_string(model_directory) + "/";
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "pedestrian.rknn", 0);
#elif defined(USE_BMNN)
            ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "pedestrian.bmodel", 0);
#else
            ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "pedestrian.onnx", 0);
#endif
            ioprocess_pipeline_->manual_possible_normalization(0, 1.f / 255);
            ioprocess_pipeline_->set_postprocessing(yolov8_GEN<1, 1>);
        }

        exposing::param_vector<pedestrian::box_info> detect(const exposing::param_span<std::uint8_t> &bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float> &param_map)
        {
            auto results_box_info = exposing::make_param_vector<pedestrian::box_info>();
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            if (roi_x < 0 || roi_x > width || roi_y > height || roi_y < 0 || roi_height < 0 || (roi_height + roi_y) > height || roi_width < 0 || (roi_width + roi_x) > width)
            {
                throw exposing::abi_invalid_argument("incorrect roi in universal_pedestrian");
            }

            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));

            std::vector<pedestrian::box_info_internal> run_detect_result;
            cv::Point roi_start(roi_x, roi_y);
            run_detect(run_detect_result, cropped_image, roi_start, param_map);

            //mapping roi
            for (auto &it : run_detect_result)
            {
                it.x1 += roi_x;
                it.x2 += roi_x;
                it.y1 += roi_y;
                it.y2 += roi_y;
                results_box_info.push_back(glasssix::exposing::make_as_first<box_info_impl>(it));
            }

            return results_box_info;
        }

        void run_detect(std::vector<box_info_internal>& results, cv::Mat& image, cv::Point& roi_start, std::map<std::string, float>& param_map) {
            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.5f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;
            const int letter_h = 736;
            const int letter_w = 1280;

            GenPipTools::LetterInfo letter_op;
            auto letter_img = GenPipTools::letter_image(image, letter_w, letter_h, letter_op, true);
            auto tensor_out = ioprocess_pipeline_->forward(letter_img).begin()->second;
            const int vf_nums = tensor_out->height(); //vf, visual field
            const int per_vf_len = tensor_out->width();
            std::vector<PersonBBox> box_list;
            for (size_t idx = 0; idx < vf_nums; idx++) {
                float* pdata = tensor_out->mutable_cpu_data() + idx * per_vf_len;
                float conf = pdata[4];
                if (conf > con_thres) {
                    PersonBBox obj_box(pdata[0] * letter_w, pdata[1] * letter_h, pdata[2] * letter_w, pdata[3] * letter_h, conf, 0);
                    box_list.push_back(obj_box);
                }
            }
            GenPipTools::nms_cpu(box_list, iou_thres);
            GenPipTools::letter_map_origin_location(box_list, letter_op);

            for (auto person : box_list) {
                person.add(roi_start);
                box_info_internal box_info;
                box_info.x1 = person.xmin;
                box_info.x2 = person.xmax;
                box_info.y1 = person.ymin;
                box_info.y2 = person.ymax;
                box_info.score = person.score;
                box_info.category = 1;
                results.push_back(box_info);
            }
        }

        std::string version()
        {
            const std::string algo_module_version = "4.1.0";
            std::string nn_frame_version = ioprocess_pipeline_->version();
            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    private:
        std::shared_ptr<PrePostProcessGenPipeline> ioprocess_pipeline_;
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

    exposing::param_vector<pedestrian::box_info> classify_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}