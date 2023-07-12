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


namespace glasssix::sleep
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{hardcode::get_model_params("sleep", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
                :net_instance_(phai,  model_directory + std::string("/sleeping.rknn"), device)
        {

        }

        exposing::param_vector<sleep::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            // std::cout<<"sleep\n";
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
        static bool sort_score(Bbox box1,Bbox box2) {
            return box1.score > box2.score ? true : false;
        }

        static std::pair<cv::Mat, float> letterbox(cv::Mat& img, cv::Size new_shape)
        {
            int H = img.rows;
            int W = img.cols;
            float ratio_w = (float)W / (float)new_shape.width;
            float ratio_h = (float)H / (float)new_shape.height;
            float ratio = ratio_w;

            cv::Mat resize_img;
            if(H==new_shape.height && W==new_shape.width)
            {
                resize_img=img;
            }
            else
            {
                if (ratio_w == ratio_h)
                {
                    cv::resize(img, resize_img, cv::Size2i{ new_shape.width, new_shape.height });}
                else if (ratio_w > ratio_h)
                {

                    int new_x = new_shape.width;
                    int new_y = (int)(H / ratio_w);
                    int pad1 = (int)((new_shape.height - new_y) / 2);
                    int pad2 = new_shape.height - new_y - pad1;
                    cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                    cv::copyMakeBorder(resize_img, resize_img, 0, pad1 + pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
                }
                else
                {
                    ratio = ratio_h;
                    int new_y = new_shape.height;
                    int new_x = (int)(W / ratio_h);
                    int pad1 = (int)((new_shape.width - new_x) / 2);
                    int pad2 = new_shape.width - new_x - pad1;
                    cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                    cv::copyMakeBorder(resize_img, resize_img, 0, 0, 0, pad1 + pad2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
                }
            }

            return { resize_img, ratio };
        }

        /**
         * @fun preprocess
         * @param src, new_shape
         * @return tensor(preprocess(image))
         * @details image preprocess and make tensor from images
         */

        std::tuple<cv::Mat, float> preprocess_detection(cv::Mat& src, const cv::Size& input_shape = cv::Size(640, 640))
    {
        bool fullcon = 0;
        float scale = std::min((float)input_shape.width / (float)src.cols, (float)input_shape.height / (float)src.rows);
        cv::Mat cut_image;
        cv::Mat mask_image(input_shape, CV_8UC3, cv::Scalar(114, 114, 114));

        if (src.rows != input_shape.height || src.cols != input_shape.width)
        {
            cv::resize(src, cut_image, cv::Size((int)(src.cols * scale), (int)(src.rows * scale)), cv::INTER_LINEAR);

            int pad_h = int((input_shape.height - cut_image.rows) / 2);
            int pad_w = int((input_shape.width - cut_image.cols) / 2);
            if (fullcon)
            {
                int auto_width = ((cut_image.cols + 31) / 32) * 32;
                int auto_height = ((cut_image.rows + 31) / 32) * 32;

                cv::resize(mask_image, mask_image, cv::Size(auto_width, auto_height), cv::INTER_LINEAR);

                pad_h = (((cut_image.rows + 31) / 32) * 32 - cut_image.rows) / 2;
                pad_w = (((cut_image.cols + 31) / 32) * 32 - cut_image.cols) / 2;
                cv::copyMakeBorder(cut_image, mask_image, pad_h, auto_height - pad_h - cut_image.rows, pad_w, auto_width - pad_w - cut_image.cols, cv::BORDER_CONSTANT, cv::Scalar{ 114, 114, 114 });
            }
            else
            {
                cv::copyMakeBorder(cut_image, mask_image, 0, input_shape.height  - cut_image.rows, 0, input_shape.width  - cut_image.cols, cv::BORDER_CONSTANT, cv::Scalar{ 114, 114, 114 });
            }

        }
        else
        {
            src.copyTo(mask_image);
        }
        cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);
        return {mask_image,scale };
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
        const float anchors[3][6] = { {36,75, 76,55, 72,146}, {142,110, 192,243, 459,401}, {12,16, 19,36, 40,28} };
        const float stride[3] = { 16.0, 32.0, 8.0 };//40 20 80->   30 15 60
        std::vector<std::vector<float>> result;
        for(int n = 0; n < 3; n++)
        {
            int num_grid_x = (int)(640 / stride[n]);
            int num_grid_y = (int)(640 / stride[n]);

            int ind = 0;
               const float *ptr_out=outs[n]->cpu_data();
            // std::cout<<outs[n].size[4]<<"gdf"<<std::endl;
            // channel
            for(int q = 0; q < 3; q++)
            {

                const float anchor_w = anchors[n][q * 2];
                const float anchor_h = anchors[n][q * 2 + 1];
                for(int i = 0; i < num_grid_x; i++)
                {
                    for(int j = 0; j < num_grid_y; j++)
                    {
                        const float* pdata = ptr_out + ind *  9;
                        float box_score = sigmoid_x(pdata[4]);

                        float cx = (sigmoid_x(pdata[0]) * 2.f - 0.5f + j) * stride[n];  //cx
                        float cy = (sigmoid_x(pdata[1]) * 2.f - 0.5f + i) * stride[n];  //cy
                        float w = powf(sigmoid_x(pdata[2]) * 2.f, 2.f) * anchor_w;      //w
                        float h = powf(sigmoid_x(pdata[3]) * 2.f, 2.f) * anchor_h;      //h

                        std::vector<float> element = {cx, cy, w, h, box_score, sigmoid_x(pdata[5]), sigmoid_x(pdata[6]),sigmoid_x(pdata[7]), sigmoid_x(pdata[8])};
                        result.push_back(element);

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
                if(it[maxPosition] * conf > conf_thres)
                {
                    boxes_conf temp{};
                    temp.top_x = top_x;
                    temp.top_y = top_y;
                    temp.bot_x = bot_x;
                    temp.bot_y = bot_y;
                    temp.conf = conf;
                    temp.category =  maxPosition - 5;
                    // std::cout<<"class: "<< temp.category<<std::endl;
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
		static std::vector<Bbox> computeNmsInput(std::vector<boxes_conf>& src, int max_wh,float ratio)
        {
            std::vector<Bbox> boxes;
            std::vector<float> scores;
            std::vector<int> category;
            for(auto const &it: src)
            {
         
                int c = max_wh * it.conf;
                Bbox temp;
                temp.x      = static_cast<double>(it.top_x )*ratio;
                temp.y      = static_cast<double>(it.top_y)*ratio;
                temp.w  = static_cast<double>(it.bot_x - it.top_x)*ratio;
                temp.h  = static_cast<double>(it.bot_y - it.top_y)*ratio;
                temp.score=it.conf;
                temp.category = it.category;
                // std::cout<<it.top_x<<" "<< temp.x<<std::endl;
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
		static std::vector<location_char> non_max_suppression(std::vector<std::vector<float>>& prediction, float conf_thres, float iou_thres, float ratio)
        {
            // std::cout<<"nms inpu size "<<prediction.size()<<std::endl;
            //std::cout<<ratio<<std::endl;
            auto compute_box = yolo2xyxy(prediction, conf_thres);  

            // Batched NMS
            int max_wh = 4096;
            std::vector<Bbox> boxes;
            std::vector<float> scores;
            std::vector<int> classes;

            boxes= computeNmsInput(compute_box, max_wh,ratio);//此处做分类处理，因为有四类 而非以前的单类
            std::vector<Bbox> class_work;
            std::vector<Bbox> class_other;
            for (auto &box:boxes)
            {
                if (box.category == 0) 
                {
                    class_work.emplace_back(box);
                }
                else 
                {
                    class_other.emplace_back(box);
                }
            }
            auto bboxes_work=nms(class_work, iou_thres);
            auto bboxes_other = nms(class_other, iou_thres);
            std::vector<location_char> output;

            for (auto it : bboxes_work)
            {   
                location_char temp;
                temp.x1=it.x;
                temp.x2=it.x+it.w;
                temp.y1=it.y;
                temp.y2=it.y+it.h;
                temp.category = it.category;
                temp.confidence=it.score;
                output.emplace_back(temp);
            }

            for (auto it : bboxes_other)
            {
                location_char temp;
                temp.x1 = it.x;
                temp.x2 = it.x + it.w;
                temp.y1 = it.y;
                temp.y2 = it.y + it.h;
                temp.category = it.category;
                temp.confidence=it.score;
                output.emplace_back(temp);
            }

            return output;
        }
        /**
           * @fun run_detect
           * @param image param_map
           * @return std::vector<sleep::box_info_internal>
           * @details run detect (maybe in multithreading)
        */
        std::vector<sleep::box_info_internal> run_detect(cv::Mat& image, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
		
            float conf_threshold= param_map.count("conf_thres") ? param_map["conf_thres"] : 0.25f;
            float iou_threshold = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.45f;      

			auto old_shape = cv::Size(roi_width, roi_height);

			auto new_shape = cv::Size(640,  640);

            cv::Mat blob;
            float ratio = 0;

            // std::tie(blob, ratio) = preprocess(image,new_shape);
            std::tie(blob, ratio) = preprocess_detection(image,new_shape);
            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;

            auto  network_result = net_instance_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);

            std::vector<std::string>  out_names={"522","534","output"};


            for (size_t i=0;i< 3; i++)//对输出数据做处理
            {
                forwards.push_back(network_result[out_names[i]]);
            }

			// float conf_threshold = 0.25f;
			// float iou_threshold = 0.45f;

			auto result = concat(forwards, conf_threshold);

			auto nms_result = non_max_suppression(result, conf_threshold, iou_threshold, 1/ratio);
            // std::cout<<"nms_size:"<<nms_result.size()<<std::endl;
            std::vector<box_info_internal> output;

            for(auto const &it: nms_result)
            {
                box_info_internal temp;
                temp.x1 = it.x1;
                temp.y1 = it.y1;
                temp.x2 = it.x2;
                temp.y2 = it.y2;
                // temp.category = std::get<2>(it);
                temp.category=it.category;
                temp.confidence=it.confidence;
                // std::cout<< temp.category<<std::endl;
                output.push_back(temp);
            }
            // std::cout<<"result_size:"<<output.size()<<std::endl;
            return output;
        }


    private:
        std::string model_directory_;
        int device_;
        rknnwrapper::rknn_wrapper net_instance_;

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
}
