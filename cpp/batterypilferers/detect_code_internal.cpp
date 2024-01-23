#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

// #include <opencv2/highgui.hpp>
// #include <opencv2/core.hpp>
// #include <opencv2/imgproc.hpp>
// #include <opencv2/dnn.hpp>
// #include "hardcode.hpp"
// #include "Excalibur/pipeline.hpp"
// #include "Primitives/tensor_conversions.hpp"

#include "general.hpp"

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
    #include <RKNN2Wrapper/rknn2_wrapper.hpp>
#endif
#include <abi/param_vector.hpp>
#include <utility>
#include <tuple>

namespace glasssix::batterypilferers
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
            : model_directory_{ std::string(model_directory) }, device_{ device }
        {

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            net_detect_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("batterypilferers", false),
            std::string(model_directory) + "/" +"batterypilferers_detect.rknn", device);      

            net_classify_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("batterypilferers", false),
            std::string(model_directory) + "/" +"batterypilferers_class.rknn", device);      
#else
            net_detect_ = std::make_unique<glasssix::excalibur::pipeline<float>>(get_model_params("batterypilferers", false),
            std::string(model_directory) + "/" +"batterypilferers_detect.racy", device);      

            net_classify_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params("batterypilferers", false),
            std::string(model_directory) + "/" +"batterypilferers_class.racy", device);   
#endif  
            init_data();
        }

        std::string version()
        {
			const std::string algo_module_version = "1.0.1";
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			std::string nn_frame_version = net_detect_->version();
#else
			std::string nn_frame_version = net_detect_->version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

        void init_data()
        {
            for (size_t i = 0; i < 33600; i++)
            {
                if(i<25600)
                {
                    add_weight[i] = i%160;
                    add_weight[i+33600] = i/160 ;
                    mul_weight[i] =8.f;
                }
                else if( i<32000)
                {
                    add_weight[i] = (i -25600)% 80;
                    add_weight[i+33600] = (i-25600)/80;
                    mul_weight[i] = 16.f;
                }
                else
                {
                    add_weight[i] = (i -32000)% 40;
                    add_weight[i+33600] = (i-32000)/40;
                    mul_weight[i] = 32.f;
                }
            }
        }

        std::vector<Bbox> yolo_detect(cv::Mat& image,float conf_threshold ,float iou_threshold)
        {     
            auto new_shape = cv::Size(1280,  1280);
            cv::Mat blob;
            float ratio = 0;
            int pad_h=0;  
            int pad_w=0;
            std::tie(blob, ratio) = preprocess_detection( image, pad_h, pad_w, new_shape ) ;
            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;

            std::shared_ptr<memory::tensor<float>> real_forwards;
            auto  network_result = net_detect_->forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);

            std::vector<std::string>  out_names = {"845","844","843" };

            for (size_t i=0;i< out_names.size(); i++)
            {
                forwards.push_back(network_result[out_names[i]]);
            }

            int candicate_num=0;
            std::vector<int> class_mask;
            auto real_output = Yolov8s_Concat(forwards, conf_threshold, candicate_num,add_weight,mul_weight,class_mask);

            auto nms_input640  = XYXY2WH(real_output, pad_h, pad_w, 1.f/ratio, candicate_num, class_mask);

            box_result_move_to_disjoint_region( nms_input640, class_mask, 100000);

            auto nms_result_index = nms_process(nms_input640, conf_threshold, iou_threshold);

            box_result_move_to_disjoint_region( nms_input640, class_mask, -100000);

            std::vector<Bbox> current_frame_result;

            for (size_t i = 0; i < nms_result_index.size(); i++)
            {   
                int index = nms_result_index[i];
                current_frame_result.emplace_back(nms_input640[index][0],nms_input640[index][1],nms_input640[index][0]+nms_input640[index][2],nms_input640[index][1]+nms_input640[index][3],class_mask[index],nms_input640[index][4],0 );
            }
            return current_frame_result;

        }

        exposing::param_vector<batterypilferers::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width,
                                                        int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.3f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;

            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 24);

            // CHECK_EQ(bitmap.size(), batch_size*channels * height * width);

            std::vector<cv::Mat> images;
            int pic_size =  3 * height * width;
            for (size_t i = 0; i < batch_size; i++)
            {
                cv::Mat image(cv::Size(width, height), CV_8UC3);
                std::memcpy(image.data, bitmap.data()+ pic_size, sizeof(uint8_t) * 3 * height * width);   
                images.push_back(image);
            }
            
            if (roi_x<0 || roi_x>width || roi_y > height || roi_y < 0 || roi_height<0 || (roi_height + roi_y) >height || roi_width<0 || (roi_width + roi_x) > width)
            {
                throw exposing::abi_invalid_argument("incorrect roi in batterypilferers");
            }

            std::vector<std::vector<car_person_batery>> frames_info;
            std::vector<cv::Mat> candicate_images;

            for (size_t i = 0; i < batch_size; i++)
            {
                cv::Mat cropped_image = images[i](cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));
                auto one_frame_result = yolo_detect(cropped_image, con_thres, iou_thres);
                auto result = deal_one_frame(one_frame_result);
                frames_info.push_back(result);
            }

            auto compareVectors = [](const std::vector<car_person_batery>& a, const std::vector<car_person_batery>& b) {
                return a.size() > b.size(); };

            std::sort(frames_info.begin(), frames_info.end(), compareVectors);

            std::vector<Bbox> crop_rect = get_candicate_rect(frames_info[0],frames_info[1]);

            std::vector<int> is_battery_pilferers(crop_rect.size());
            std::vector<float> scores(crop_rect.size());
            for (int i=0;i<crop_rect.size();i++) 
            {   
                // std::cout<<crop_rect[0].x1<<" "<<crop_rect[0].x2<<" "<<crop_rect[0].y1<<" "<<crop_rect[0].y2<<std::endl;
                for (size_t j = 0; j < batch_size; j++)
                {
                    cv::Mat candicate_detect = images[j](cv::Range(crop_rect[0].y1, crop_rect[0].y2), cv::Range(crop_rect[0].x1, crop_rect[0].x2));
                    cv::resize(candicate_detect, candicate_detect, cv::Size(256, 256));
                        candicate_images.push_back(candicate_detect);    
                }

                std::vector<float> candicate_steal(65536*24);
                concat_pic(candicate_images,candicate_steal.data());
                auto  network_result = net_classify_->forward(candicate_steal.data(), { 1, 256, 256, 3*batch_size}, RKNN_TENSOR_NHWC );
                const float* batterypilferers_result = network_result["output"]->cpu_data();//0 -> steal  
                is_battery_pilferers[i] = batterypilferers_result[0]>batterypilferers_result[1] ? 1 : 0 ;
                scores[i]=batterypilferers_result[1-0];

            }

            auto fin_result= exposing::make_param_vector<box_info>();
            std::vector<box_info_internal> result;
            for (int i=0; i < crop_rect.size(); i++)
            {
                box_info_internal temp_result;
                temp_result.x1=crop_rect[i].x1;
                temp_result.y1=crop_rect[i].y1;
                temp_result.x2=crop_rect[i].x1 + crop_rect[i].x2;
                temp_result.y2=crop_rect[i].y1 + crop_rect[i].y2;
                temp_result.score=  scores[i];
                temp_result.category = is_battery_pilferers[i];
                result.push_back( temp_result  );
            }
  
            for (auto& i : result)
            {
                fin_result.push_back(exposing::make_as_first<box_info_impl> (i));
            }

            return fin_result;
        }

    private:

        int batch_size=8; 

        std::string model_directory_;
        int device_; 
        std::array<float,33600*2> add_weight;
        std::array<float,33600>   mul_weight;

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::unique_ptr < rknnwrapper::rknn_wrapper> net_detect_;       
        std::unique_ptr < rknnwrapper::rknn_wrapper> net_classify_;    
#else
       std::unique_ptr < glasssix::excalibur::pipeline<float>> net_detect_;  
       std::unique_ptr < glasssix::excalibur::pipeline<float>> net_classify_;  
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

    exposing::param_vector<batterypilferers::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap,
        int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}
