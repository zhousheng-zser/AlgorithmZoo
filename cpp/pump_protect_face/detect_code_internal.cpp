#include <iostream>
#include <cmath>
#include <tuple>
#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"
#include <chrono>
#include <GenPipeline/GenPipeline.hpp>
#include <YoloFamily/Yolo_wrapper.hpp>
#include "general.hpp"
#include <abi/param_vector.hpp>
#include <utility>

#if defined(USE_BMNN)
    #include <sophonyolov8/SophonYolov8Wrapper.hpp>
#endif

namespace glasssix::pump_protect_face
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device, int model_type)
            : model_directory_{ std::string(model_directory) }, device_{ device },model_type_{model_type}
        {
            std::string model_ins;
            int width, height;
            if(model_type == 0){
                model_ins = std::string( "_640" );
                width  = 640;
                height = 384;
            }
            else{
                model_ins = std::string( "_1280" );
                width  = 1280;
                height = 736;
            }
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            //罗健翔的模型
            net_face_ = std::make_shared<GenPipeline>(model_directory_ + "/pump_protect_face_det" + model_ins + ".rknn" , device);
            yolov8_instance_face = std::make_shared<Yolov8_Complement<GenPipeline>>(width, height, net_face_); //2个模板变量分别对应 GenPipeline ，(通用yolov8)是否是李鑫尧的yolo  第三个参数默认为false

            net_detect_face = std::make_shared<GenPipeline>(model_directory_ + "/pump_protect_face_cls.rknn", device);

            net_detect_goggle = std::make_shared<GenPipeline>(model_directory_ + "/pump_protect_face_goggle.rknn", device);
#elif defined(USE_BMNN)
            yolov8_instance_face = std::make_shared<SophonYolov8Wrapper>(model_directory_ + "/pump_protect_face_det" + model_ins + ".bmodel");
            yolov8_instance_face->init();

            net_detect_face = std::make_shared<GenPipeline>(model_directory_ + "/pump_protect_face_cls.bmodel", device);
            net_detect_face->manual_possible_normalization(0, 1.f / 255);
            net_detect_goggle = std::make_shared<GenPipeline>(model_directory_ + "/pump_protect_face_goggle.bmodel", device);
            net_detect_face->manual_possible_normalization(0, 1.f / 255);
#endif  
        }

        std::string version()
        {
            const std::string algo_module_version = "2.0.0";
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::string nn_frame_version = "1.0.0";
#else
            std::string nn_frame_version = "1.0.0";
#endif
            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }


        exposing::param_vector<pump_protect_face::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width,
            std::map<std::string, float>& param_map)
        {
            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.6f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.4f;
            float detect_thres = 0.3;

            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);

            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
            // cv::imwrite("IV.jpg", image);
            auto frame_result = yolov8_instance_face->get_objects(image, con_thres, iou_thres);   //检测人脸
            // std::cout << "thres: " << con_thres << " ; iou_thres: " << iou_thres << std::endl;
            std::cout << "frame_result'size: " << frame_result.size() << std::endl;
            std::vector<Bbox> person_box_list;
            std::vector<Bbox> head_box_list;
            std::vector<Bbox> valid_head_box_list;    ///在人体框里的有效人头
            for (auto& it : frame_result) {
                    head_box_list.push_back(Bbox{ it.x1,it.y1,it.x2,it.y2,it.category,it.score,0 });
            }
            std::vector<box_info_internal> result;
            for (int i = 0; i < head_box_list.size(); ++i) {
                    valid_head_box_list.push_back(head_box_list[i]);
            }
            auto fin_result = exposing::make_param_vector<pump_protect_face::box_info>();
            if (valid_head_box_list.size() == 0)
                return  fin_result;      ///没有人头直接返回空数组

            for (auto& val : valid_head_box_list)
            {
                if (val.score >= con_thres) {
                    box_info_internal temp_result;
                    temp_result.x1 = val.x1;
                    temp_result.y1 = val.y1;
                    temp_result.x2 = val.x2;
                    temp_result.y2 = val.y2;
                    temp_result.category = val.category;
                    temp_result.score = val.score;
                    int category = -1;
                    int category_goggle = -1;
                    //人脸/护目镜分类
                    //满足以上条件,就开始:
                    cv::Rect roi(val.x1, val.y1, val.x2 - val.x1, val.y2 - val.y1);
                    cv::Mat crop = image(roi);
                    cv::resize(crop, crop, {128, 128});
                    auto result = net_detect_face->forward(crop).begin()->second->cpu_data();

                    if(*result > *(result+1))
                    {
                        category = 0;
                        auto result_goggle = net_detect_goggle->forward(crop).begin()->second->cpu_data();
                        if(*result_goggle > *(result_goggle+1))
                            category_goggle = 0;
                    }
                    if(category ==0 && category_goggle == 0)
                    {
                        temp_result.category = 1;
                    }
                    else
                        temp_result.category = -1;
                    fin_result.push_back(exposing::make_as_first<box_info_impl>(temp_result));

                }
            }

            return fin_result;
        }

    private:
        std::string model_directory_;
        int device_;
        int model_type_;
        int img_size = 1280;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::shared_ptr<GenPipeline> net_face_;
        std::shared_ptr<Yolov8_Complement<GenPipeline>> yolov8_instance_face;//人脸检测
#elif defined(USE_BMNN)
        std::shared_ptr<SophonYolov8Wrapper> yolov8_instance_face;//人脸检测
#endif
        std::shared_ptr<GenPipeline> net_detect_face;// 人脸分类
        std::shared_ptr<GenPipeline> net_detect_goggle; //护目镜
    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device, int model_type)
        : impl_{ std::make_unique<impl>(model_directory, device, model_type) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;

    std::string detect_code_internal::version()
    {
        return impl_->version();
    }

    exposing::param_vector<pump_protect_face::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap,
        int channels, int height, int width, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, param_map);
    }
}
