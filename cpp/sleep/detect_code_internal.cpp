#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>

#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include "general.hpp"

namespace glasssix::sleep
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{get_model_params("sleep", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
                :net_instance_(phai,  model_directory + std::string("/sleeping.rknn"), device)
        {
            init_data();
        }

        exposing::param_vector<sleep::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            
            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);
                 
            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
            {
                  throw exposing::abi_invalid_argument("incorrect roi in sleep");
            }


            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width));

            auto result = run_detect(cropped_image, roi_x, roi_y, roi_width, roi_height, param_map);

            auto results = exposing::make_param_vector<sleep::box_info>();

            for(auto& it:result) {
                it.x1+=roi_x;
                it.x2+=roi_x;
                it.y1+=roi_y;
                it.y2+=roi_y;
                results.push_back(glasssix::exposing::make_as_first<box_info_impl>(it));
            }

            return results;
        }

        std::string version()
		{
			const std::string algo_module_version = "1.0.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			//#if 0
			std::string nn_frame_version = net_instance_.version();
#else
			std::string nn_frame_version = net_instance_.version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
		}

    private:

        /**  @fun letterbox
         *   @param image scaleFill
         *   @return letterbox(image)
         *   @details Resize and pad image while meeting stride-multiple constrain
         */


        void init_data()
        {
            posture_add_weight.resize(8400*2);
            posture_mul_weight.resize(8400);
            for(int i=0;i<8400;i++)
            {
                if( i<6400)
                {
                    posture_add_weight[i]=i%80;
                    posture_add_weight[i+8400]=i/80;
                    posture_mul_weight[i]=8.f;
                }
                else if(i<8000)
                {
                    posture_add_weight[i]=(i -6400)%40;
                    posture_add_weight[i+8400]=(i -6400)/40;
                    posture_mul_weight[i]=16.f;
                }
                else
                {
                    posture_add_weight[i]=(i -8000)%20;
                    posture_add_weight[i+8400]=(i -8000)/20;
                    posture_mul_weight[i]=32.f;
                }
            }
        }


        typedef struct Bbox 
        {
            int x;
            int y;
            int w;
            int h;
            float score;
            int category;
        }Bbox;

        struct location_char
        {
            int x1;
            int y1;
            int x2;
            int y2;
            int category;
            float confidence;
        };
        static bool sort_score(Bbox box1,Bbox box2) 
        {
            return box1.score > box2.score ? true : false;
        }


        std::tuple<cv::Mat, float> preprocess_detection(cv::Mat src, int &pad_h, int &pad_w, cv::Size input_shape = cv::Size(640, 640))
        {
            float scale = std::min((float)input_shape.width / (float)src.cols, (float)input_shape.height / (float)src.rows);
            cv::Mat cut_image;
            cv::Mat mask_image(input_shape, CV_8UC3, cv::Scalar(114, 114, 114));
            if (src.rows != input_shape.height || src.cols != input_shape.width)
            {
                cv::resize(src, cut_image, cv::Size((int)(src.cols * scale), (int)(src.rows * scale)), cv::INTER_LINEAR);

                pad_h = int((input_shape.height - cut_image.rows) / 2);
                pad_w = int((input_shape.width - cut_image.cols) / 2);
                cv::copyMakeBorder(cut_image, mask_image, pad_h, input_shape.height - cut_image.rows - pad_h, pad_w, input_shape.width - cut_image.cols - pad_w, cv::BORDER_CONSTANT, cv::Scalar{114, 114, 114});
            }
            else
            {
                src.copyTo(mask_image);
            }
            cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);
            return {mask_image, scale};
        }

        //input xywh location and the class, move different class to Disjoint region
        void box_result_move_to_disjoint_region(std::vector<std::vector<float>>&sou_data, std::vector<int>& category_mask, int bias=100000 )
        {
            for (size_t i = 0; i < sou_data.size(); i++)
                sou_data[i][0] =  sou_data[i][0]+ category_mask[i]*bias;      
        }

        std::vector<int> nms_process(std::vector<std::vector<float>>& nms_input, float threshold=0.0,float iou_thres=0.9 )
        {
            std::vector<cv::Rect2d> xywh_boxes(nms_input.size());;
            std::vector<float> scores(nms_input.size());
            std::vector<int> indices_body(nms_input.size());;//候选框顺序

            for (size_t i = 0; i < nms_input.size(); i++)
            {
                cv::Rect2d boxwh;
                boxwh.x      =  nms_input[i][0];
                boxwh.y      =  nms_input[i][1];
                boxwh.width  =  nms_input[i][2];
                boxwh.height =  nms_input[i][3];   
                xywh_boxes[i]=boxwh;
                scores[i] = nms_input[i][4];   
                indices_body[i]=i;
            }
            std::vector<int> indices_body_copy( indices_body.size() );
            for(int i=0;i<indices_body_copy.size();i++)           
                indices_body_copy[i]=i;
            cv::dnn::NMSBoxes(xywh_boxes, scores, threshold, iou_thres, indices_body_copy, 1.f, 0);
          
            return indices_body_copy;
        }

        struct boxes_conf
        {
            float top_x;
            float top_y;
            float bot_x;
            float bot_y;
            float conf;
            int category;
        };

        /**
           * @fun run_detect
           * @param image param_map
           * @return std::vector<sleep::box_info_internal>
           * @details run detect (maybe in multithreading)
        */
        std::vector<sleep::box_info_internal> run_detect(cv::Mat& image, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
		
            float conf_threshold= param_map.count("conf_thres") ? param_map["conf_thres"] : 0.85f;
            float iou_threshold = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.45f;      
            int  device_id = std::round(param_map.count("device_id") ? param_map["device_id"] : 0.f);      
            int frame_count_thres = std::round(param_map.count("frame_count_thres") ? param_map["frame_count_thres"] : 10.f);      

			auto old_shape = cv::Size(roi_width, roi_height);

			auto new_shape = cv::Size(640,  640);

            cv::Mat blob;
            float ratio = 0;
            int pad_h=0;
            int pad_w=0;

            std::tie(blob, ratio) = preprocess_detection(image,pad_h,pad_w,new_shape);

            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;

            auto  network_result = net_instance_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);

            std::vector<std::string>  out_names={"onnx::Concat_439", "onnx::Concat_432", "onnx::Concat_425", "onnx::Mul_502"};

            for (size_t i=0;i< out_names.size(); i++)//对输出数据做处理
            {
                forwards.push_back(network_result[out_names[i]]);
            }

            int candicate_box_num = 0;
            std::vector<int> category_mask;
            auto real_forwards = Posture_Concat640(forwards,17, 0.01 ,candicate_box_num ,posture_add_weight, posture_mul_weight, category_mask);

            auto nms_input640  = XYXY2WH(real_forwards, pad_h, pad_w, 1.f/ratio, 17, candicate_box_num, category_mask);

            box_result_move_to_disjoint_region( nms_input640, category_mask, 100000);

            auto nms_result_index = nms_process(nms_input640, conf_threshold, iou_threshold);

            box_result_move_to_disjoint_region( nms_input640, category_mask, -100000);

           
            std::vector<box_info_internal> output;
            //获取当前帧判定为睡觉的丢到特征库去匹配
            std::vector<Sleep_trace> current_sleep_infos;
            for (size_t i = 0; i < nms_result_index.size(); i++)
            {
                int index = nms_result_index[i];
                if(category_mask[index])
                {
                    Sleep_trace S_t( nms_input640[index][0], nms_input640[index][1], nms_input640[index][2], nms_input640[index][3],nms_input640[index][4] ,frame_count_thres );
                    current_sleep_infos.push_back(S_t);
                }
            }

            auto sleep_status = sleep_trace(sleep_trace_, device_id, current_sleep_infos);

            for (size_t i = 0; i < sleep_status.size(); i++)
            {
                box_info_internal temp;
                temp.x1 = current_sleep_infos[i].m_left;
                temp.y1 = current_sleep_infos[i].m_top;
                temp.x2 = (current_sleep_infos[i].m_left+current_sleep_infos[i].m_width);
                temp.y2 = (current_sleep_infos[i].m_top+current_sleep_infos[i].m_height);
                temp.category = sleep_status[i] ;  
                temp.confidence = current_sleep_infos[i].conf;//置信度需要解决一哈
                output.push_back(temp);
            }
            

            
            //仅处理当前帧模型原生输出为未睡觉的
            for (size_t i = 0; i < nms_result_index.size(); i++)
            {   
                int index = nms_result_index[i];
                if( !category_mask[index] )
                {
                    box_info_internal temp;
                    temp.x1 = nms_input640[index][0];
                    temp.y1 = nms_input640[index][1];
                    temp.x2 = (nms_input640[index][0]+nms_input640[index][2]);
                    temp.y2 = (nms_input640[index][1]+nms_input640[index][3]);
                    temp.category = 0 ;   //1是睡岗 0是其他
                    temp.confidence = nms_input640[index][0];
                    output.push_back(temp);
                }
            }
            return output;
        }


    private:
        std::vector<float> posture_add_weight;
        std::vector<float> posture_mul_weight;
        std::string model_directory_;
        int device_;
        rknnwrapper::rknn_wrapper net_instance_;

        static std::map<int, std::vector<Sleep_trace>>  sleep_trace_;

    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;

    exposing::param_vector<sleep::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }

    std::string detect_code_internal::version()
	{
		return impl_->version();
	}
    
    std::map<int, std::vector<Sleep_trace>>  detect_code_internal::impl::sleep_trace_;

}
