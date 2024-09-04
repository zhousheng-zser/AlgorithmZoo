#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"
#include <abi/param_vector.hpp>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <tuple>

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#elif defined(USE_BMNN)
#include <sophonyolov8/SophonYolov8Wrapper.hpp>
#endif

#include <YoloFamily/Yolo_wrapper.hpp>
#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include <GenPipeline/GenPipeline.hpp>
#include <Excalibur/pipeline.hpp>

namespace glasssix::pump_glove
{
    struct Bbox
    {
        int x1;
        int y1;
        int x2;
        int y2;
        int category;
        float score;
        int frame_id;
        Bbox(int x11, int y11, int x22, int y22, int category_, float score_, int frame_index) :x1(x11), x2(x22), y1(y11), y2(y22), category(category_), score(score_), frame_id(frame_index)
        {}
        Bbox(const Bbox& input) :x1(input.x1), x2(input.x2), y1(input.y1), y2(input.y2), category(input.category), score(input.score), frame_id(input.frame_id)
        {}
        Bbox(int x11, int y11, int x22, int y22) :x1(x11), x2(x22), y1(y11), y2(y22), category(-1), score(-1), frame_id(-1)
        {}

        Bbox& operator=(const Bbox& input)
        {
            if (this != &input) // 避免自我赋值
            {
                x1 = input.x1;
                x2 = input.x2;
                y1 = input.y1;
                y2 = input.y2;
                category = input.category;
                score = input.score;
                frame_id = input.frame_id;
            }
            return *this;
        }

        double area()
        {
            return (y2 - y1) * (x2 - x1);
        }
    };
    class detect_code_internal::impl
    {
    public:

        impl() {}
        impl(const exposing::param_string model_directory, int device) :impl()
        {
            std::string model_dir = exposing::to_narrow_string(model_directory);
            if (*model_dir.rbegin() != '/') model_dir += '/';
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            net_pump_glove_hand_detect_ = std::make_shared<GenPipeline>(model_dir + "pump_glove_hand_detect.rknn", device);
            net_pump_glove_hand_detect_->manual_possible_normalization(std::array<float, 3>{0.f, 0.f, 0.f}, std::array<float, 3>{1.f / 255.f, 1.f / 255.f, 1.f / 255.f});
            yolov8_instance = std::make_shared<Yolov8<GenPipeline>>(1280, 736, net_pump_glove_hand_detect_);
            net_pump_glove_cls = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "pump_glove_class.rknn", device);
#elif defined(USE_BMNN)
            yolov8_instance = std::make_shared<SophonYolov8Wrapper>(model_dir + "pump_glove_hand_detect.bmodel");
            yolov8_instance->init();
            net_pump_glove_cls = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "pump_glove_class.bmodel", device);
#endif
            net_pump_glove_cls->manual_possible_normalization(0, 1.f / 255);
        }

        cv::Mat preprocess_detection(cv::Mat& src, cv::Size input_shape = cv::Size(640, 640))
        {
            cv::Mat mask_image;
            if (src.rows != input_shape.height || src.cols != input_shape.width)
            {
                cv::resize(src, mask_image, input_shape, cv::INTER_LINEAR);
            }
            else
            {
                src.copyTo(mask_image);
            }
            cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);
            return mask_image;
        }

        exposing::param_vector<pump_glove::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width,
            int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {

            float hand_conf_thres = param_map.count("hand_conf_thres") ? param_map["hand_conf_thres"] : 0.4f;
            float conf_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.7f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.65f;

            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            if (roi_x<0 || roi_x>width || roi_y > height || roi_y < 0 || roi_height<0 || (roi_height + roi_y) >height || roi_width<0 || (roi_width + roi_x) > width)
            {
                throw exposing::abi_invalid_argument("incorrect roi in pump_glove");
            }
            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width)).clone();

            auto objects = yolov8_instance->get_objects(cropped_image, hand_conf_thres, iou_thres);
            std::vector<Bbox> hand_list; //手
            for (auto& it : objects)
            {
                hand_list.push_back(Bbox(it.x1, it.y1, it.x2, it.y2, it.category, it.score, 0));
            }
            auto results = exposing::make_param_vector<pump_glove::box_info>();
            for (auto& hand : hand_list)
            {
                // TODO 把这个手拿去做分类
                cv::Mat crop = cropped_image(cv::Range(hand.y1, hand.y2), cv::Range(hand.x1, hand.x2)).clone();
                auto new_shape = cv::Size(128, 128);
                cv::Mat blob = preprocess_detection(crop, new_shape);
                auto tensor_out = net_pump_glove_cls->forward(blob).begin()->second;
                float* cls_conf = tensor_out->mutable_cpu_data();
                if (conf_thres < cls_conf[0] && cls_conf[1] <= cls_conf[0] && cls_conf[2] <= cls_conf[0])
                {
                    pump_glove::box_info_internal val;
                    val.x1 = hand.x1 + roi_x;
                    val.y1 = hand.y1 + roi_y;
                    val.x2 = hand.x2 + roi_x;
                    val.y2 = hand.y2 + roi_y;
                    val.score = cls_conf[0];
                    val.category = 0;
                    results.push_back(glasssix::exposing::make_as_first<box_info_impl>(val));
                }
                else if (conf_thres < cls_conf[1] && cls_conf[0] <= cls_conf[1] && cls_conf[2] <= cls_conf[1])
                {
                    pump_glove::box_info_internal val;
                    val.x1 = hand.x1 + roi_x;
                    val.y1 = hand.y1 + roi_y;
                    val.x2 = hand.x2 + roi_x;
                    val.y2 = hand.y2 + roi_y;
                    val.score = cls_conf[1];
                    val.category = 1;
                    results.push_back(glasssix::exposing::make_as_first<box_info_impl>(val));
                }
                else if (conf_thres < cls_conf[2] && cls_conf[0] <= cls_conf[2] && cls_conf[1] <= cls_conf[2])
                {
                    pump_glove::box_info_internal val;
                    val.x1 = hand.x1 + roi_x;
                    val.y1 = hand.y1 + roi_y;
                    val.x2 = hand.x2 + roi_x;
                    val.y2 = hand.y2 + roi_y;
                    val.score = cls_conf[2];
                    val.category = 2;
                    results.push_back(glasssix::exposing::make_as_first<box_info_impl>(val));
                }
            }

            return results;
        }

        std::string version()
        {
            const std::string algo_module_version = "1.0.0";
            std::string nn_frame_version = "1.0.0";
            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    private:
        std::string model_directory_;
        int device_;
        std::shared_ptr<PrePostProcessGenPipeline> net_pump_glove_cls;

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::shared_ptr<GenPipeline> net_pump_glove_hand_detect_;
        std::shared_ptr<Yolov8<GenPipeline>> yolov8_instance;
#elif defined(USE_BMNN)
        std::shared_ptr<SophonYolov8Wrapper> yolov8_instance;
#endif
    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;

    std::string detect_code_internal::version()
    {
        return impl_->version();
    }

    exposing::param_vector<pump_glove::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width,
        int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}
