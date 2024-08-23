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

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
    #include <GenPipeline/GenPipeline.hpp>
    #include <YoloFamily/Yolo_wrapper.hpp>
#elif defined(USE_BMNN)
    #include <sophonyolov8/SophonYolov8Wrapper.hpp>
#endif

#include <chrono>



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
    #ifdef ENABLE_PUMP//泵业的行人
            const int width = 640;
            const int height = 384;
            net_pedestrian_ = std::make_shared<GenPipeline>(model_dir + "/pedestrian_pump.rknn", device);
            // std::cout << "pedestrian_pump.rknn\n";
    #else//通用行人(默认使用)
            const int width = 1280;
            const int height = 736;
            net_pedestrian_ = std::make_shared<GenPipeline>(model_dir + "/pedestrian.rknn", device);
            //std::cout << "pedestrian.rknn\n";
#endif
            Yolov8_Complement_instance = std::make_shared<Yolov8<GenPipeline>>(width, height, net_pedestrian_);
#elif defined(USE_BMNN)

            Yolov8_Complement_instance = std::make_shared<SophonYolov8Wrapper>(model_dir + "/pedestrian.bmodel");
            Yolov8_Complement_instance->init();
#endif       
           
        }

        exposing::param_vector<pedestrian::box_info> detect(const exposing::param_span<std::uint8_t> &bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float> &param_map)
        {

            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.5f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;
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

            // std::cout<<"in pedestrian detect\n"<<std::endl;
            
            // auto start = std::chrono::high_resolution_clock::now();

            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));

            // cv::imwrite("image.jpg",image );

            cv::Mat imagedevice;//默认分配soc上内存

            // cv::Mat image_mat_temp(image.rows,image.cols,CV_8UC3,image.data,image.step[0]);

            // image.copyTo(imagedevice);
            // image.copyTo(image_mat_temp);

            auto pedestrian_list =  Yolov8_Complement_instance->get_objects(image, con_thres,iou_thres);

            // auto end = std::chrono::high_resolution_clock::now();
            // std::chrono::duration<float> duration = end - start; //记录经过了多长时间
            // std::cout << duration.count() << "sssss" << std::endl; //输出运行时间

            // std::cout<<"pedestrian_list size: "<< pedestrian_list.size()<< std::endl;
            for (auto person : pedestrian_list) {
                box_info_internal box_info;
                box_info.x1 = person.x1;
                box_info.x2 = person.x2;
                box_info.y1 = person.y1;
                box_info.y2 = person.y2;
                box_info.score = person.score;
                box_info.category = person.category;
                results_box_info.push_back(glasssix::exposing::make_as_first<box_info_impl>(box_info));
            }
            return results_box_info;
        }

        // std::vector<PersonBBox> run_detect(cv::Mat& image, std::map<std::string, float>& param_map) {
        //     float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.5f;
        //     float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;
        //     const int letter_h = 736;
        //     const int letter_w = 1280;

        //     GenPipTools::LetterInfo letter_op;
        //     auto letter_img = GenPipTools::letter_image(image, letter_w, letter_h, letter_op, true);
        //     auto tensor_out = ioprocess_pipeline_->forward(letter_img).begin()->second;
        //     const int vf_nums = tensor_out->height(); //vf, visual field
        //     const int per_vf_len = tensor_out->width();
        //     std::vector<PersonBBox> box_list;
        //     for (size_t idx = 0; idx < vf_nums; idx++) {
        //         float* pdata = tensor_out->mutable_cpu_data() + idx * per_vf_len;
        //         float conf = pdata[4];
        //         if (conf > con_thres) {
        //             PersonBBox obj_box(pdata[0] * letter_w, pdata[1] * letter_h, pdata[2] * letter_w, pdata[3] * letter_h, conf, 0);
        //             box_list.push_back(obj_box);
        //         }
        //     }
        //     GenPipTools::nms_cpu(box_list, iou_thres);
        //     GenPipTools::letter_map_origin_location(box_list, letter_op);

        //     return box_list;
        // }

        std::string version()
        {
            const std::string algo_module_version = "4.1.1";
            std::string nn_frame_version = "122";
            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    private:

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::shared_ptr<GenPipeline> net_pedestrian_;
        std::shared_ptr<Yolov8<GenPipeline>> Yolov8_Complement_instance;
#elif defined(USE_BMNN)
        std::shared_ptr<SophonYolov8Wrapper> Yolov8_Complement_instance;
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

    exposing::param_vector<pedestrian::box_info> classify_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}
