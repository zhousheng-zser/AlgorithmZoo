#include "yolov7_net.hpp"
#include "hardcode.hpp"
#include "face_info_impl.hpp"
#include <opencv2/opencv.hpp>

#include "tensor_conversions.hpp"
#include "Excalibur/operation_safty_cut.hpp"



namespace glasssix::longinus
{
 
    yolov7_net::yolov7_net(std::string_view models_directory, int model_type, float nms_threshold, int device)
        : facedetector_base{ models_directory, model_type, nms_threshold, device },
        model_type_{ model_type },
        nms_threshold_{ nms_threshold }
    {
        int width = 0;
        int height = 0;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        switch (model_type)
        {
            case 0:
                yolov7_face_ = std::make_shared<GenPipeline>(std::string(models_directory) + "/yolov7_320.rknn", device);
                width = 320;
                height = 320;
                break;
            case 1:
                yolov7_face_ = std::make_shared<GenPipeline>(std::string(models_directory) + "/yolov7_640.rknn", device);
                width = 640;
                height = 640;
                break;
        }
#elif defined(USE_BMNN)
        switch (model_type)
        {
            case 0:
                yolov7_face_ = std::make_shared<GenPipeline>(std::string(models_directory) + "/yolov7_320.bmodel", device);
                width = 320;
                height = 320;
                break;
            case 1:
                yolov7_face_ = std::make_shared<GenPipeline>(std::string(models_directory) + "/yolov7_640.bmodel", device);
                width = 640;
                height = 640;
                break;
        }
#endif
            // yolov7_face_->manual_possible_normalization(std::array<float,3>{104.f,117.f,124.f},std::array<float,3>{0.00961538f / 255.f,0.008547f / 255.f,0.00806451f / 255.f});

            yolov7_face_->manual_possible_normalization(std::array<float,3>{0.f,0.f,0.f},std::array<float,3>{1.f / 255.f,1.f / 255.f,1.f / 255.f});

            yolov7_instance = std::make_shared<Yolov7<GenPipeline,false,true>>(width,height, yolov7_face_);
    }

    yolov7_net::~yolov7_net()
    {
    }

    exposing::param_vector<longinus::face_info> yolov7_net::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int min_size, float threshold, int order, bool do_attributing)
    {
        if (bitmap.empty())
            throw exposing::abi_invalid_argument("current frame is empty");

        if (order != 1)
            throw exposing::abi_invalid_argument("Not supported order");

        CHECK_EQ(channels, 3);
        CHECK_EQ(bitmap.size(), channels * height * width);

        cv::Mat cache_temp(height, width, CV_8UC3, bitmap.data());


        auto face_object = yolov7_instance->get_objects( cache_temp, threshold, nms_threshold_ );


        std::vector<face_info_internal> face_infos;
        for (auto it : face_object)
        {
            face_info_internal temp_face_info;
            temp_face_info.ori_rect.x = temp_face_info.rect.x = it.x1;
            temp_face_info.ori_rect.y = temp_face_info.rect.y = it.y1;
            temp_face_info.ori_rect.w = temp_face_info.rect.w = it.x2 - it.x1;
            temp_face_info.ori_rect.h = temp_face_info.rect.h = it.y2 - it.y1;
            temp_face_info.score = it.score;
            face_infos.push_back(temp_face_info);
        }

        std::vector<face_info_internal> temp_vec;
        for (auto& face : face_infos)
        {
            refine(face, height, width, true);//生成新的正方形

            if (do_attributing)
            {
                if (face.rect.h * face.rect.w <= 0)
                    throw exposing::abi_invalid_argument("face.rect.h * face.rect.w <= 0");

                face.headpose[0] = face.headpose[1] = face.headpose[2] = std::numeric_limits<float>::min();
                face.clarity = std::numeric_limits<float>::min();
                face.is_alive = false;
                face.has_mask = std::numeric_limits<float>::min();//初始化结构体信息

                cv::Rect rect(face.rect.x, face.rect.y, face.rect.w, face.rect.h);
                cv::Mat faceROI_in_frame_mat;
                mat_safty_cut(cache_temp, faceROI_in_frame_mat, rect);//从原图cache_temp截图截出人脸faceROI_in_frame_mat
                tracking_landmark(faceROI_in_frame_mat, face, rect.x, rect.y,true);//利用新的关键点模型计算出更准确的人脸位置 rect.x是原人脸截图在新生成的正方形的的x坐标

                
        // for (size_t i = 0; i < 5; i++)
        // {
        //     cv::circle(cache_temp, cv::Point(int( face.pts.x[i] ), int(face.pts.y[i] ) ), 3, CV_RGB(125, 255, 0), 2);
        // }
        // cv::imwrite("faceROI_in_frame_mat.jpg", cache_temp);


                refine(face, height, width, true);
            }

            cv::Point2f center_eye((face.pts.x[0] + face.pts.x[1]) / 2, (face.pts.y[0] + face.pts.y[1] / 2));
            cv::Point2f center_mouth((face.pts.x[3] + face.pts.x[4]) / 2, (face.pts.y[3] + face.pts.y[4]) / 2);
            double distance = std::sqrt((center_eye.x - center_mouth.x) * (center_eye.x - center_mouth.x) + (center_eye.y - center_mouth.y) * (center_eye.y - center_mouth.y));

            if (face.score > threshold && distance > std::numeric_limits<double>::epsilon())
                temp_vec.push_back(face);
        }

        std::sort(temp_vec.begin(), temp_vec.end(), [](const face_info_internal& a, const face_info_internal& b)
            { return a.rect.h * a.rect.w > b.rect.h * b.rect.w; });

        if (temp_vec.size() > 0)
        {
            cache0_ = cache1_;
            cache1_ = cache_temp;
        }

        auto faces = exposing::make_param_vector<longinus::face_info>();
        for (auto& i : temp_vec)
            faces.push_back(exposing::make_as_first<face_info_impl>(i));

        return faces;
    }
    
    std::string yolov7_net::version() const
    {
        return "1.0.0";
    }
}