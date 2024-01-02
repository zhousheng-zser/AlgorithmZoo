#include <iostream>
#include <cmath>
#include <tuple>
#include <utility>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <logger.hpp>


#ifdef USE_RKNNAPI
//#if 0
#include "../../common/include/RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#endif
#include "../posture/detect_code.hpp"

#include "Yolov8CutPipline.hpp"
#include "ObjBox.hpp"

namespace glasssix::playphone
{
    class detect_code_internal::impl
    {
    public:

        impl(int device) noexcept : device_{ device } {}
        impl(std::string_view model_directory, int device)
            : impl(device)
        {
            posture_instance_ = glasssix::exposing::make_exported_interface<posture::detect_code>(model_directory, device, 1);
            phone_pipilne_ = std::make_unique<RknnYolov8Pipline>(exposing::to_narrow_string(model_directory) + "/" + "playphone_v8" + ".rknn", device);        
        }

        exposing::param_vector<playphone::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
            {
                  throw exposing::abi_invalid_argument("incorrect roi in playphone");
            }
            auto result = exposing::make_param_vector<playphone::box_info>();
            
            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);            

            float man_conf_thres = param_map.count("man_conf_thres") ? param_map["man_conf_thres"] : 0.6f;
            float man_nms_thres = param_map.count("man_nms_thres") ? param_map["man_nms_thres"] : 0.7f;
            float phone_conf_thres = param_map.count("phone_conf_thres") ? param_map["phone_conf_thres"] : 0.6f;
            float phone_nms_thres = param_map.count("phone_nms_thres") ? param_map["phone_nms_thres"] : 0.5f;
            auto posture_param_abi = exposing::make_param_hash_map<exposing::param_string, float>();
            posture_param_abi.add_or_update("conf_thres", man_conf_thres);
            posture_param_abi.add_or_update("nms_thres", man_nms_thres);
            exposing::param_vector<posture::box_info> posture_info_list_raw = posture_instance_.detect(bitmap, channels, height, width, 0, 0, width, height, posture_param_abi);

            for (auto pinfo : posture_info_list_raw)
            {
                PostureInfo postureInfo{ pinfo };
                box_info_internal pphone_box_info;
                pphone_box_info.set_man(postureInfo);

                if (postureInfo.invaild_hand_kpnum() < 2 && postureInfo.invaild_face_kpnum() < 2)
                {
                    //detect phones
                    auto playphone_det_region_rect = postureInfo.get_playphone_det_region();
                    auto playphone_det_region = safty_cut(image, playphone_det_region_rect);
                    std::vector<ObjBox> phone_list = phone_pipilne_->detect(playphone_det_region, playphone_det_region_rect.tl(), phone_conf_thres, phone_nms_thres);

                    for (auto phoneObj : phone_list)
                    {
                        //cv::rectangle(image, phoneObj.get_rect(), { 0, 0, 255 }, 2);
                        auto phoneRect = phoneObj.get_rect();
                        auto hands_region = postureInfo.get_playphone_hands_region();
                        // phone traversing match hands
                        for (auto& hand_region : hands_region)
                        {

                            auto is_overlap = overlap(phoneRect, hand_region);
                            if (is_overlap)
                            {
                                pphone_box_info.set_phone(phoneObj);
                                break;
                            }
                        }
                    }
                }
                else 
                {
                    //body error
                    pphone_box_info.set_body_error(postureInfo);
                }

                result.push_back(exposing::make_as_first<box_info_impl>(pphone_box_info));
            }

            return result;
        }


        // if rectA intersect rectB
        bool overlap(cv::Rect rectA, cv::Rect rectB)
        {
            auto minx = std::max(rectA.x, rectB.x);
            auto miny = std::max(rectA.y, rectB.y);
            auto maxx = std::min(rectA.x + rectA.width, rectB.x + rectB.width);
            auto maxy = std::min(rectA.y + rectA.height, rectB.y + rectB.height);

            if (minx > maxx || miny > maxy)
                return false;
            else
                return true;
        }

        std::string version()
        {
			const std::string algo_module_version = "2.1.1";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			//#if 0
			std::string nn_frame_version = phone_pipilne_->version();
#else
			std::string nn_frame_version = phone_pipilne_->version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);

        }

        inline cv::Mat safty_cut(cv::Mat& img, cv::Rect roi)
        {
            int width = roi.width;
            int height = roi.height;
            int x = roi.x;
            int y = roi.y;

            cv::Mat mat(height, width, img.type(), cv::Scalar(0));
            int _x = x;
            int _y = y;
            int _width = width;
            int _height = height;
            if (x < 0)
            {
                _x = 0;
                _width = width + x;
            }

            if (_x + _width > img.cols)
                _width = img.cols - _x;

            if (y < 0)
            {
                _y = 0;
                _height = height + y;
            }

            if (_y + _height > img.rows)
                _height = img.rows - _y;

            img(cv::Rect(_x, _y, _width, _height)).copyTo(mat(cv::Rect(_x - x, _y - y, _width, _height)));
            return mat;
        }

    private:
        std::string model_directory_;
        int device_;
        posture::detect_code posture_instance_;
        std::unique_ptr<RknnYolov8Pipline> phone_pipilne_;

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

    exposing::param_vector<playphone::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}
