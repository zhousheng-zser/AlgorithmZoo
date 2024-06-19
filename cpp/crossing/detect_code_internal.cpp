#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include <abi/param_vector.hpp>

#include <opencv2/opencv.hpp>


#include <GenPipeline/GenPipeline.hpp>
#include <YoloFamily/Yolo_wrapper.hpp>
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#endif
namespace glasssix::crossing
{
    class detect_code_internal::impl
    {
    public:
        impl() {}

		impl(const exposing::param_string model_directory, int device = -1) :impl()
        {
            std::string model_dir = exposing::to_narrow_string(model_directory) ;

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            net_crossing_detect_ = std::make_shared<GenPipeline>(model_dir + "/crossing.rknn", device);
#elif defined(USE_BMNN)
            net_crossing_detect_ = std::make_shared<GenPipeline>(model_dir + "/crossing.bmodel", device);
#else
            net_crossing_detect_ = std::make_shared<GenPipeline>(model_dir + "/crossing.onnx", device);
#endif
            yolov8_instance = std::make_shared<Yolov8<GenPipeline, false, false>>(1280, 736, net_crossing_detect_);
        }

        exposing::param_vector<crossing::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {

            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.6f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;
			auto result = exposing::make_param_vector<crossing::box_info>();

            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
            {
                  throw exposing::abi_invalid_argument("incorrect roi in crossing");
            }
            
            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));

            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width)).clone();

			//auto crossing_list = run_detect(cropped_image, param_map);
            auto pump_objects = yolov8_instance->get_objects(cropped_image, con_thres, iou_thres);
            for (auto& var : pump_objects)
            {
                box_info_internal crossing_internal;
                //crossing.add(roi_x, roi_y);
                crossing_internal.x1 = var.x1+ roi_x;
                crossing_internal.y1 = var.y1+ roi_y;
                crossing_internal.x2 = var.x2+ roi_x;
                crossing_internal.y2 = var.y2+ roi_y;
                crossing_internal.score = var.score;
                crossing_internal.category = var.category;
                result.push_back(exposing::make_as_first<box_info_impl>(crossing_internal));
            }

			return result;
        }

        std::string version()
        {
            const std::string algo_module_version = "2.0.0";
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::string nn_frame_version = "2.0.0";
#else
            std::string nn_frame_version = net_crossing_detect_->version();
#endif
            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    private:

        //struct CrossingBox :public GenPipTools::YoloBoxBase {
        //public:
        //    using YoloBoxBase::YoloBoxBase; //Inheriting Constructors
        //};

        //std::vector<CrossingBox> run_detect(cv::Mat& image, std::map<std::string, float>& param_map)
        //{
        //    float conf_threshold= param_map.count("conf_thres") ? param_map["conf_thres"] : 0.6f;
        //    float nms_threshold = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;

        //    constexpr int infrW = 1280;
        //    constexpr int infrH = 736;
        //    constexpr bool ifCvtRGB = true;

        //    GenPipTools::LetterInfo letter_op;
        //    auto letter_img = GenPipTools::letter_image(image, infrW, infrH, letter_op, ifCvtRGB);
        //    auto net_rstmap = ioprocess_pipeline_->forward(letter_img);
        //    auto tensor_out = net_rstmap.begin()->second;
        //    const int vf_nums = tensor_out->height(); //vf, visual field
        //    const int per_vf_len = tensor_out->width();
        //    std::vector<CrossingBox> box_list;
        //    for (size_t idx = 0; idx < vf_nums; idx++) {
        //        float* pdata = tensor_out->mutable_cpu_data() + idx * per_vf_len;
        //        float conf_pos = pdata[4];
        //        float conf_neg = pdata[5];
        //        if (conf_pos > conf_threshold && conf_neg < 0.1) {
        //            CrossingBox obj_box(pdata[0] * infrW, pdata[1] * infrH, pdata[2] * infrW, pdata[3] * infrH, conf_pos, 0);
        //            box_list.push_back(obj_box);
        //        }
        //    }

        //    GenPipTools::nms_cpu(box_list, nms_threshold);
        //    GenPipTools::letter_map_origin_location(box_list, letter_op);
        //    return box_list;
        //}

    private:
        std::string model_directory_;
        int device_;
        /*std::shared_ptr<PrePostProcessGenPipeline> ioprocess_pipeline_;*/

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        // GenPipeline*  net_pump_hoisting_detect2_;
        std::shared_ptr<GenPipeline> net_crossing_detect_;
        std::shared_ptr<Yolov8<GenPipeline, false ,false>> yolov8_instance;
#else
        std::unique_ptr<excalibur::pipeline<float>> net_crossing_detect_;
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

    exposing::param_vector<crossing::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}
