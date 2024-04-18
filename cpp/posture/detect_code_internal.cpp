#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "general.hpp"
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
    #include <RKNN2Wrapper/rknn2_wrapper.hpp>
#endif
#include <abi/param_vector.hpp>
#include <utility>
#include <tuple>


namespace glasssix::posture
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device, int model_type)
            : model_directory_{ std::string(model_directory) }, device_{ device },model_type_{model_type}
        {

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            if(model_type_)
                net_posture_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/posture1280_17.rknn", device);
            else
                net_posture_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/posture1280_12.rknn", device);

#elif defined(USE_BMNN)
            if(model_type_==1)
                net_posture_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/posture1280_17.bmodel", device);
            else
                net_posture_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/posture1280_12.bmodel", device);
#endif
            net_posture_->manual_possible_normalization(std::array<float,3>{0.f,0.f,0.f},std::array<float,3>{1.f / 255.f,1.f / 255.f,1.f / 255.f});
            yolov8_instance = std::make_shared<Yolov8<GenPipeline,false,true>>(1280, 1280, net_posture_);
        }

        std::string version()
        {
			const std::string algo_module_version = "3.1.0";
			std::string nn_frame_version = net_posture_->version();
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }


        std::vector<ObjectInfo> postrue_detect_yolo(cv::Mat &detect_img,float bias,bool horizontal, float ratio, int throw_right, int throw_left,  float con_thres,float iou_thres)
        {
            bias = bias/ratio;
            cv::imwrite( "posture_detact"+ std::to_string(bias)+".jpg", detect_img);
            auto objects = yolov8_instance->get_objects( detect_img, con_thres, iou_thres );

            auto delete_border_objects = throw_border_resulttest(objects, horizontal, throw_right,  throw_left,  detect_img.rows);  
            for(auto& object : delete_border_objects)
            {
                    object.x1 = object.x1/ratio + (horizontal? bias:0); 
                    object.x2 = object.x2/ratio + (horizontal? bias:0); 
                    object.y1 = object.y1/ratio + (horizontal? 0:bias); 
                    object.y2 = object.y2/ratio + (horizontal? 0:bias); 
                    for (size_t i = 0; i < object.key_points.size() ; i++)
                    {
                        object.key_points[i].x = object.key_points[i].x/ratio  + (horizontal? bias:0);
                        object.key_points[i].y = object.key_points[i].y/ratio  + (horizontal? 0:bias);
                    }
            }
            return delete_border_objects;

        }

        

        exposing::param_vector<posture::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width,
            int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.45f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;

            if (bitmap.empty())
                throw exposing::abi_invalid_argument("current frame is empty");

            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * channels * height * width);

            if (roi_x<0 || roi_x>width || roi_y > height || roi_y < 0 || roi_height<0 || (roi_height + roi_y) >height || roi_width<0 || (roi_width + roi_x) > width)
                throw exposing::abi_invalid_argument("incorrect roi in posture");

            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));

            cv::imwrite( "cropped_image.jpg", cropped_image);

            slide_pics_params pics_and_bias  = Sliding_Cut_Pic(cropped_image,640);

            std::vector<ObjectInfo> Need_to_filter;
            // for (size_t i = 0; i < pics_and_bias.imgs.size(); i++)
            // {   
            //     if(  pics_and_bias.detect)
            //     {
            //         auto results = postrue_detect_yolo(pics_and_bias.imgs[i], pics_and_bias.bias[i], pics_and_bias.horizontal, pics_and_bias.ratio, 
            //                                                 pics_and_bias.throw_result_border[i*2],pics_and_bias.throw_result_border[i*2+1], con_thres,iou_thres );
            //         for(auto& result:results )
            //             Need_to_filter.push_back(result);
            //     }
            // }

            auto objects_of_full_figure = yolov8_instance->get_objects( cropped_image, con_thres, iou_thres );

            for(auto& object_of_full_figure : objects_of_full_figure)
                Need_to_filter.push_back(object_of_full_figure);

            std::vector<std::vector<float>> nms_input;  
            for (const auto& var : Need_to_filter) 
                nms_input.push_back({var.x1, var.y1, var.x2 - var.x1, var.y2 - var.y1, var.score});  

            auto nms_result_index = object_nms(nms_input,iou_thres);

            auto fin_result= exposing::make_param_vector<box_info>();

            std::vector<box_info_internal> result;

            for (auto& id : nms_result_index)
            {
                box_info_internal temp_result;
                temp_result.x1 = Need_to_filter[id].x1 + roi_x;
                temp_result.y1 = Need_to_filter[id].y1 + roi_y;
                temp_result.x2 = Need_to_filter[id].x2 + roi_x;
                temp_result.y2 = Need_to_filter[id].y2 + roi_y;
                temp_result.score = Need_to_filter[id].score;
                temp_result.key_points = exposing::make_param_vector<float>();
                for(int j=0;j< Need_to_filter[id].key_points.size(); j++)
                {
                    temp_result.key_points.push_back(Need_to_filter[id].key_points[j].x + roi_x);
                    temp_result.key_points.push_back(Need_to_filter[id].key_points[j].y + roi_y);
                    temp_result.key_points.push_back(Need_to_filter[id].key_points[j].score);
                }
                result.push_back(temp_result);
            }
  
            for (auto& i : result)
                fin_result.push_back(exposing::make_as_first<box_info_impl> (i));

            return fin_result;
        }

    private:
        std::string model_directory_;
        int device_; 
        int model_type_=1;

        std::shared_ptr<GenPipeline> net_posture_;
        std::shared_ptr<Yolov8<GenPipeline, false,true>> yolov8_instance;

    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device,int model_type)
        : impl_{ std::make_unique<impl>(model_directory, device, model_type) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;

    std::string detect_code_internal::version()
    {
        return impl_->version();
    }

    exposing::param_vector<posture::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap,
        int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}
