#include <iostream>
#include <cmath>
#include "hardcode.hpp"
#include "general.hpp"

#include "classify_code_internal.hpp"
#include "box_info_impl.hpp"
#include <Excalibur/pipeline.hpp>
#include <Primitives/tensor_conversions.hpp>
#include "logger.hpp"

#include "../posture/detect_code.hpp"

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#ifdef BUILD_DEBUG_INFO
#include <opencv2/highgui/highgui.hpp>
#endif // BUILD_DEBUG_INFO


#include <abi/param_vector.hpp>
#include <Primitives/fmt/format.h>
#include <utility>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif


#ifdef USE_RKNNAPI
//#if 0
#include "../../common/include/RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#endif

//YHC
// #include "dbg.h"

namespace glasssix::workcloth
{
    class classify_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
            : model_directory_{ std::string(model_directory) }, device_{ device }
        {
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            classify_instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("workcloth_cls"), std::string(model_directory) + "/" + "workcloth_cls" + ".rknn", device);
#endif
            static bool ready = glasssix::exposing::get_component_loader().add_module_by_name("posture");
            posture_instance_ = glasssix::exposing::make_exported_interface<posture::detect_code>(model_directory, device);

        }

        exposing::param_vector<workcloth::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, int color_index, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image2(height,width, CV_8UC3);
            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);
            if (roi_x<0 || roi_x>width || roi_y > height || roi_y < 0 || roi_height<0 || (roi_height + roi_y) >height || roi_width<0 || (roi_width + roi_x) > width)
            {
                throw exposing::abi_invalid_argument("incorrect roi in phone");
            }

			cv::Rect roi_rect{ roi_x, roi_y, roi_width, roi_height };

            std::vector<box_info_internal> results;
            auto result = exposing::make_param_vector<box_info>();


            // //YHC
            // auto vis_mat = image.clone();

            auto empty_map_abi = exposing::make_param_hash_map<exposing::param_string, float>();
            exposing::param_vector<posture::box_info> posture_info_list = posture_instance_.detect(bitmap, channels, height, width, 0, 0, width, height, empty_map_abi);
            std::vector<PostureInfo> persons_info;

            int pinfo_counter = 0;
            for (auto pinfo : posture_info_list) {
                pinfo_counter++;
                PostureInfo postureInfo{ pinfo };

                // std::cout<<"## Kpoints len: "<< postureInfo.Kpoints.size() <<std::endl;


                if(postureInfo.x1<=roi_x||postureInfo.x2>=(roi_x+roi_width)||postureInfo.y1<=roi_y||postureInfo.y2>=(roi_y+roi_height)) continue;

                persons_info.push_back(postureInfo);

                //YHC
                // cv::rectangle(vis_mat, postureInfo.get_rect(), cv::Scalar{ 250, 0, 250 }, 3);
                // cv::rectangle(vis_mat, postureInfo.cls_cut, cv::Scalar{ 250, 0, 0 }, 3);
                // cv::rectangle(vis_mat, postureInfo.color_cut, cv::Scalar{ 0, 0, 255 }, 3);
                // for (auto kp : postureInfo.Kpoints) {
                //     cv::circle(vis_mat, kp, 3, { 0,0,250 }, 3);
                // }
                //YHC~
            }

            //YHC
            // std::cout<<"## export posture.png"<<std::endl;
            // cv::rectangle(vis_mat, roi_rect, cv::Scalar{ 250, 250, 250 }, 3);
            // cv::imwrite("/home/firefly/yhc/call_wkch/img_out/posture.png", vis_mat);
            //YHC~

            run_workcloth(results, image, persons_info, color_index, param_map);


            for (auto& i : results)
            {
                result.push_back(exposing::make_as_first<box_info_impl>(i));
            }
            return result;
        }

        std::string version()
        {
            const std::string algo_module_version = "1.2.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            //#if 0
            //std::string nn_frame_version = rknnwrapper::rknn_wrapper::version();
            std::string nn_frame_version = classify_instance_->version();
#else
            std::string nn_frame_version = excalibur::pipeline<float>::version();
#endif
            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    private:

        cv::Mat preprocess(cv::Mat img, int hope_w = 640, int hope_h = 640)
        {
            int H = img.rows;
            int W = img.cols;
            float ratio_w = (float)W / (float)hope_w;
            float ratio_h = (float)H / (float)hope_h;
            cv::Mat resize_img;
            if (ratio_w == ratio_h)
                cv::resize(img, resize_img, cv::Size2i{ hope_w, hope_h });
            else if (ratio_w > ratio_h) {
                int new_x = hope_w;
                int new_y = (int)(H / ratio_w);
                int pad1 = (int)((hope_h - new_y) / 2);
                int pad2 = hope_h - new_y - pad1;
                cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
            }
            else {
                int new_y = hope_h;
                int new_x = (int)(W / ratio_h);
                int pad1 = (int)((hope_w - new_x) / 2);
                int pad2 = hope_w - new_x - pad1;
                cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 127,127,127 });
            }
            return resize_img;
        }


        std::pair<float,float> run_classify(cv::Mat& image)
        {
            cv::Mat blob = preprocess(image, 112, 112);
            cv::cvtColor(blob, blob, cv::COLOR_BGR2RGB);

            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forwards;
            auto network_result = classify_instance_->forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);
            for (auto& out : network_result) {
                forwards.push_back(out.second);
            }

            const float* data_ptr = forwards[0]->cpu_data();
            return std::make_pair(data_ptr[0], data_ptr[1]);
        }

        enum class Color {
            black = 0, grey, white, red, orange, yellow, green, cyan, blue, purple
        };

        std::map<Color, std::pair<cv::Scalar, cv::Scalar>> color_hsv_cfg{
            {Color::black,{cv::Scalar{0, 0, 0},cv::Scalar{180, 255, 46}}},
            {Color::grey,{cv::Scalar{0, 0, 0},cv::Scalar{180, 35, 220}}},
            {Color::grey,{cv::Scalar{0, 0, 0},cv::Scalar{180, 35, 220}}},

            //{Color::red,{cv::Scalar{0, 35, 46},cv::Scalar{180, 35, 220}}},
            {Color::orange,{cv::Scalar{11, 35, 46},cv::Scalar{25, 255, 255}}},
            {Color::yellow,{cv::Scalar{26, 35, 46},cv::Scalar{34, 255, 255}}},

            {Color::green,{cv::Scalar{35, 35, 46},cv::Scalar{77, 255, 255}}},
            {Color::cyan,{cv::Scalar{78, 35, 46},cv::Scalar{99, 255, 255}}},
            {Color::blue,{cv::Scalar{100, 35, 46},cv::Scalar{124, 255, 255}}},
            {Color::purple,{cv::Scalar{125, 35, 46},cv::Scalar{155, 255, 255}}},
        };

        std::pair<float,bool> calculate_singglehsv_method(cv::Mat image, float judge_thred, Color mode) {
            auto img = image.clone();
            int H = img.rows;
            int W = img.cols;
            int total_pixels = H * W;

            cv::Mat color_mask;
            cv::Mat hsv_img;
            cv::cvtColor(img, hsv_img, cv::COLOR_BGR2HSV);

            if(mode!=Color::red){
                cv::Scalar hsv_lower = color_hsv_cfg.at(mode).first;
                cv::Scalar hsv_upper = color_hsv_cfg.at(mode).second;

                cv::inRange(hsv_img, hsv_lower, hsv_upper, color_mask);
            }
            else{
                cv::Mat mask1;
                cv::Scalar red_lower1 = cv::Scalar{0, 35, 46};
                cv::Scalar red_upper1 = cv::Scalar{10, 255, 255};
                cv::inRange(hsv_img, red_lower1, red_upper1, mask1);

                cv::Mat mask2;
                cv::Scalar red_lower2 = cv::Scalar{156, 35, 46};
                cv::Scalar red_upper2 = cv::Scalar{180, 255, 255};
                cv::inRange(hsv_img, red_lower2, red_upper2, mask2);
                color_mask = mask1 + mask2;
            }

            float color_pixels = cv::countNonZero(color_mask);
            float color_ratio = color_pixels / total_pixels;
            
            bool color_pure = color_ratio>judge_thred;

            return {color_ratio,color_pure};

        }

        void run_workcloth(std::vector<box_info_internal>& results, cv::Mat& image, std::vector<PostureInfo>& persons,int color_index, std::map<std::string, float>& param_map)
        {

            std::cout << "==== run_workcloth ====" << std::endl;

            float W = image.cols;
            float H = image.rows;
            float conf_threshold = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.5f;
            float nms_threshold = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.5f;

            for(auto& person:persons){
                cv::Mat cls_image = image(person.cls_cut).clone();
                auto classify_result = run_classify(cls_image);
                // dbg(classify_result);

                float score = std::max(classify_result.first, classify_result.second);
                int category = classify_result.first < classify_result.second;
                // dbg(score,category);

                cv::Mat color_image = image(person.cls_cut).clone();

                auto [color_conf, color_pure] = calculate_singglehsv_method(color_image,0.85, static_cast<Color>(color_index));

                box_info_internal in_box_info;

                in_box_info.score = score;
                in_box_info.category = category;
                in_box_info.color_conf = color_conf;
                in_box_info.color_pure = color_pure;
                in_box_info.x1 = person.x1;
                in_box_info.y1 = person.y1;
                in_box_info.x2 = person.x2;
                in_box_info.y2 = person.y2;
                results.push_back(in_box_info);
            }

        }


    private:
        std::string model_directory_;
        int device_;

        posture::detect_code posture_instance_;

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        //#if 0
        std::unique_ptr<rknnwrapper::rknn_wrapper> classify_instance_;
#else
        std::unique_ptr<excalibur::pipeline<float>> classify_instance_;
#endif
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

    exposing::param_vector<workcloth::box_info> classify_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height,int color_index, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, color_index, param_map);
    }
}