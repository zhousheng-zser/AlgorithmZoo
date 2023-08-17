#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>
#include <unordered_map>
#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include "hardcode.hpp"

namespace glasssix::smoke
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{get_model_params("flame", false),  exposing::to_narrow_string(model_directory), device} 
        {

        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
                :net_detect_(phai,  model_directory + std::string("/person_sim.rknn"), device), net_category_(phai, model_directory + std::string("/smoke_sim.rknn"), device), model_directory_(model_directory)
        {   
           

        }

        exposing::param_vector<smoke::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
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
                  throw exposing::abi_invalid_argument("incorrect roi in smoke");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width)).clone();

            auto detect_result = run_detect(cropped_image, roi_x, roi_y, roi_width, roi_height, param_map);
            auto cate_result=categorys(cropped_image,detect_result);

            auto results = exposing::make_param_vector<smoke::box_info>();

            for(auto& it:cate_result) 
            {
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
        std::string nn_frame_version = net_detect_.version();
#else
        std::string nn_frame_version = net_detect_.version();
#endif
        return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    private:

        /**  @fun letterbox
         *   @param image scaleFill
         *   @return letterbox(image)
         *   @details Resize and pad image while meeting stride-multiple constrain
         */
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
        };
        static bool sort_score(Bbox box1,Bbox box2) {
            return box1.score > box2.score ? true : false;
        }

       
        /**
         * @fun preprocess
         * @param src, new_shape
         * @return tensor(preprocess(image))
         * @details image preprocess and make tensor from images
         */
        std::tuple<cv::Mat, float> preprocess_categroy(cv::Mat& src,const cv::Size& input_shape = cv::Size(224, 224))
        {
            float scale = std::min((float)input_shape.width/(float)src.cols, (float)input_shape.height/(float)src.rows);
            cv::Mat cut_image;
            cv::cvtColor(src, cut_image, cv::COLOR_BGR2RGB);
            // src.copyTo(cut_image);     
            cv::Mat mask_image;
            unsigned char *data1=src.ptr<unsigned char>();
            if (src.rows != 224 || src.cols !=224)
            {
                cv::resize(cut_image, cut_image, cv::Size(224, 224), cv::INTER_CUBIC);   //no centre crop
                mask_image=cut_image;
            }
            else 
            {
                mask_image= cut_image(cv::Range(16, 224 + 16), cv::Range(16, 224 + 16));
            }
            return {mask_image, scale};
        }

        std::tuple<cv::Mat, float> preprocess_detection(cv::Mat src,int& pad_h,int& pad_w,  cv::Size input_shape = cv::Size(640, 640) )
        {
            float scale = std::min((float)input_shape.width/(float)src.cols, (float)input_shape.height/(float)src.rows);
            cv::Mat cut_image;
            cv::Mat mask_image(input_shape, CV_8UC3, cv::Scalar(114, 114, 114));
            if( src.rows != input_shape.height || src.cols != input_shape.width)
            {      
                cv::resize(src, cut_image, cv::Size((int)(src.cols * scale), (int)(src.rows * scale)), cv::INTER_LINEAR);

                pad_h = int((input_shape.height - cut_image.rows) /2 ) ; 
                pad_w = int((input_shape.width - cut_image.cols) /2 ) ; 

                cv::copyMakeBorder(cut_image, mask_image, pad_h, input_shape.height-cut_image.rows-pad_h, pad_w, input_shape.width-cut_image.cols-pad_w, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
        
            }
            else 
            {
                src.copyTo(mask_image);     
            }
            cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);
            unsigned char * data=mask_image.ptr<uchar>();
            return {mask_image,scale};
        }
    
		/**
		 * @fun sigmoid_x
		 * @param x
		 * @return sigmoid(x)
		 */
		static inline float sigmoid_x(float x)
		{
			return static_cast<float>(1.f / (1.f + exp(-x)));
		}

		/**
		 * @fun concat
		 * @param infer_out, conf_thres
		 * @return source
		 * @details concat 3 into 1
		 */
        std::vector<std::vector<float>> concat(std::vector<std::shared_ptr<memory::tensor<float>>>& outs, float conf_thres)
        {
            const float anchors[3][6] = { {36,75, 76,55, 72,146}, {142,110, 192,243, 459,401}, {12,16, 19,36, 40,28} };//yolov7用
        const float stride[3] = { 16.0, 32.0, 8.0 };//40 20 80->   30 15 60

        std::vector<std::vector<float>> result;
        for(int n = 0; n < 3; n++)
        {
            int num_grid_x = (int)(640 / stride[n]);
            int num_grid_y = (int)(640 / stride[n]);

            int ind = 0;
            const float *ptr_out=outs[n]->cpu_data();

            for(int q = 0; q < 3; q++)
            {

                const float anchor_w = anchors[n][q * 2];
                const float anchor_h = anchors[n][q * 2 + 1];
                for(int i = 0; i < num_grid_x; i++)
                {
                    for(int j = 0; j < num_grid_y; j++)
                    {
                        // float* pdata = (float*)outs[n].data + ind *  outs[n].size[4];
                         const float* pdata = ptr_out + ind *  85;
                        float box_score = sigmoid_x(pdata[4]);
                        if(box_score > 0.35f)
                        {
                            float cx = (sigmoid_x(pdata[0]) * 2.f - 0.5f + j) * stride[n];  //cx
                            float cy = (sigmoid_x(pdata[1]) * 2.f - 0.5f + i) * stride[n];  //cy
                            float w = powf(sigmoid_x(pdata[2]) * 2.f, 2.f) * anchor_w;      //w
                            float h = powf(sigmoid_x(pdata[3]) * 2.f, 2.f) * anchor_h;      //h

                            std::vector<float> element(85); 
                            element[0]=cx;
                            element[1]=cy;
                            element[2]=w;
                            element[3]=h;
                            element[4]=box_score;
                            for(int i=5; i<85;i++ )
                            {
                                element[i]=sigmoid_x(pdata[i] );
                            }
  
                            result.push_back(element);
                        }
                        ind++;
                    }
                }
            }
        }
        return result;
        }
    /**
        * @fun computeNx6
        * @param anchor, conf_thres
        * @return [box,confidence,category]
        * @details concat xywh into nx6
        */
        struct boxes_conf
        {
            float top_x;
            float top_y;
            float bot_x;
            float bot_y;
            float conf;
            int category;
        };

        struct label_confidence
        {   
            int x1;
            int y1;
            int x2;
            int y2;
            int label;
            float confidence;
        };

       

        static std::vector<boxes_conf> yolo2xyxy(std::vector<std::vector<float>>& src, float conf_thres=0.f)
        {
            std::vector<boxes_conf> res;
            for(auto it: src)
            {
                float top_x = it[0] - it[2] / 2;
                float top_y = it[1] - it[3] / 2;
                float bot_x = it[0] + it[2] / 2;
                float bot_y = it[1] + it[3] / 2;
                float conf  = it[4];
                int maxPosition = std::max_element(it.begin()+5, it.end()) - it.begin();
                if(conf > conf_thres)
                {
                    boxes_conf temp{};
                    temp.top_x = top_x;
                    temp.top_y = top_y;
                    temp.bot_x = bot_x;
                    temp.bot_y = bot_y;
                    temp.conf = conf;
                    temp.category =  maxPosition - 5;
                    //if (temp.category == 0)
                    {
                        res.push_back(temp);
                    }

                }
            }
            return res;
        }

        static float iou(Bbox box1, Bbox box2) 
        {
            int x1 = std::max(box1.x, box2.x);
            int y1 = std::max(box1.y, box2.y);
            int x2 = std::min(box1.x + box1.w, box2.x + box2.w);
            int y2 = std::min(box1.y + box1.h, box2.y + box2.h);
            int w = std::max(0, x2 - x1);
            int h = std::max(0, y2 - y1);
            float over_area = w * h;
            return over_area / (box1.w*box1.h + box2.w*box2.h - over_area);
        }
 
        static std::vector<Bbox> nms(std::vector<Bbox>&boxes, float threshold)
        {
            std::vector<Bbox>resluts;
            std::sort(boxes.begin(), boxes.end(), sort_score);
            while (boxes.size()> 0) 
            {
                resluts.push_back(boxes[0]);
                int index = 1;
                while (index < boxes.size()) {
                    float iou_value = iou(boxes[0], boxes[index]);
                    if (iou_value > threshold) {
                        boxes.erase(boxes.begin() + index);
                    }
                    else {
                        index++;
                    }
                }
                boxes.erase(boxes.begin());
            }
            return  resluts;
        }



		/**
		 * @fun computNmsInput
		 * @param src, max_wh
		 * @return std::pair<bboxes, confidence>
		 * @details slice src into bboxes and confidence, which need by dnn::NMS
		 */
		static std::vector<Bbox> computeNmsInput(std::vector<boxes_conf>& src, int max_wh,float ratio,int pad_h,int pad_w)
        {
            std::vector<Bbox> boxes;
            std::vector<float> scores;
            std::vector<int> category;
            for(auto const &it: src)
            {
         
                int c = max_wh * it.conf;
                Bbox temp;
                temp.x      = static_cast<double>(it.top_x -pad_w)*ratio;
                temp.y      = static_cast<double>(it.top_y-pad_h)*ratio;
                temp.w  = static_cast<double>(it.bot_x - it.top_x)*ratio;
                temp.h  = static_cast<double>(it.bot_y - it.top_y)*ratio;
                temp.score=it.conf;
                temp.category = it.category;
                //if (temp.category == 0 || temp.category == 4)
                //{
                    boxes.push_back(temp);
                //}
            }
            return boxes;
        }

		/**
		 * @fun non_max_suppression
		 * @param prediction, conf_thres, iou_thres
		 * @return std::vector(boxes, classes)
		 * @details Non-Maximum Suppression (NMS) on inference results
		 */
		static std::vector<location_char> non_max_suppression(std::vector<std::vector<float>>& prediction, float conf_thres, float iou_thres, float ratio,int pad_h,int pad_w)
        {

            auto compute_box = yolo2xyxy(prediction, conf_thres);  
            // Batched NMS
            int max_wh = 4096;
            std::vector<Bbox> boxes;
            std::vector<float> scores;
            std::vector<int> classes;

            boxes= computeNmsInput(compute_box, max_wh,ratio,pad_h,pad_w);

            std::vector<Bbox> class_num;
            std::vector<Bbox> class_metra;
            for (auto &box:boxes)
            {
                if (box.category == 0) 
                {
                    class_num.emplace_back(box);
                }
                else 
                {
                    class_metra.emplace_back(box);
                }
            }
            auto bboxes_num=nms(class_num, iou_thres);
            auto bboxes_metra = nms(class_metra, iou_thres);
            std::vector<location_char> output_num;
            std::vector<location_char> output_metra;

            //auto f = [](int x){if(x<0) return 0; else return x;};

            for (auto it : bboxes_num)
            {   
                location_char temp;
                temp.x1=it.x;
                temp.x2=it.x+it.w;
                temp.y1=it.y;
                temp.y2=it.y+it.h;
                temp.category = it.category;
                output_num.emplace_back(temp);
            }


            return output_num;
        }
        /**
           * @fun run_detect
           * @param image param_map
           * @return std::vector<smoke::box_info_internal>
           * @details run detect (maybe in multithreading)
         */
 
             
        std::vector<box_info_internal> categorys(cv::Mat& image,std::vector<location_char>& cate_input)
        {
            std::vector<box_info_internal> l_c;
            for(auto x:cate_input)
            {   
                cv::Mat cate_blob;
                float ratio=1.f;
                x.x1=x.x1<0?0:x.x1;
                x.y1=x.y1<0?0:x.y1;
                x.y2=x.y2>image.rows?image.rows:x.y2;
                x.x2=x.x2>image.cols?image.cols:x.x2;
                // cv::Mat cropped_image = image(cv::Range(321, 1438), cv::Range(140, 860));
                cv::Mat cropped_image = image(cv::Range(x.y1, x.y2), cv::Range(x.x1, x.x2));


                std::tie(cate_blob, ratio) =preprocess_categroy(cropped_image);

                unsigned char *iptr=cate_blob.ptr<uchar>();


                auto network_result1 = net_category_.forward(cate_blob.data, 
                            { 1, cate_blob.rows, cate_blob.cols,cate_blob.channels() }, RKNN_TENSOR_NHWC);
  
                box_info_internal result;
                float smoke=network_result1["output"]->cpu_data()[0];
                float not_smoke=network_result1["output"]->cpu_data()[1];
                not_smoke=exp(not_smoke)/(exp(not_smoke)+exp(smoke));
                smoke=exp(smoke)/(exp(not_smoke)+exp(smoke));
   

                result.x1=x.x1;
                result.y1=x.y1;
                result.x2=x.x2;
                result.y2=x.y2;
                if(not_smoke>smoke)
                {
                    result.category=1;
                    result.confidence=not_smoke;
                } 
                else
                {  
                    result.category=0;
                    result.confidence=smoke;
                }
                l_c.emplace_back(result);

            }
            return l_c;
        }

        std::vector<location_char> run_detect(cv::Mat& image, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            
            float conf_threshold= param_map.count("conf_thres") ? param_map["conf_thres"] : 0.35f;
            float iou_threshold = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.45f;      

			auto old_shape = cv::Size(roi_width, roi_height);

			auto new_shape = cv::Size(640,  640);

            cv::Mat blob;
            float ratio = 0;

            int pad_h;  
            int pad_w;
            std::tie(blob, ratio) = preprocess_detection( image,pad_h,pad_w, new_shape ) ;
            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;

            auto  network_result = net_detect_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);

            // std::vector<std::string>  out_names={"359","379","output"};
            std::vector<std::string>  out_names={"528","548","output"};

            for (size_t i=0;i< 3; i++)
            {
                forwards.push_back(network_result[out_names[i]]);
            }

			// float conf_threshold = 0.35f;
			// float iou_threshold = 0.45f;

			auto result = concat(forwards, conf_threshold );
            auto nms_result = non_max_suppression(result, conf_threshold, iou_threshold, 1/ratio,pad_h,pad_w);
            std::vector<box_info_internal> output;
            
            return nms_result;
        }



    private:
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)

		rknnwrapper::rknn_wrapper net_detect_;
        rknnwrapper::rknn_wrapper net_category_;
#else
		std::unique_ptr<excalibur::pipeline<float>> net_detect_;
        std::unique_ptr<excalibur::pipeline<float>> net_category_;
#endif
        std::shared_ptr<glasssix::memory::tensor<float>> weight_Gemm_87;
        std::string model_directory_;
        int device_ ;

    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;


    exposing::param_vector<smoke::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }

    std::string detect_code_internal::version()
	{
		return impl_->version();
	}

}
