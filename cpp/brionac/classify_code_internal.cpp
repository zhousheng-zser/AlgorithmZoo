#include <iostream>
#include <cmath>
#include <tuple>


#include "classify_code_internal.hpp"
#include "../hardcode/hardcode.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

//#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "Excalibur/pipeline.hpp"
#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_safty_cut.hpp"
#include "Excalibur/operation_resize.hpp"
#include "Excalibur/operation_rgb2gray.hpp"
#include "Excalibur/operation_rotate.hpp"
#include "Primitives/tensor_conversions.hpp"
#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>

namespace glasssix::brionac
{
    class classify_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
            : model_directory_{ std::string(model_directory) }, device_{ device }
        {

            dash_detect = std::make_unique<excalibur::pipeline<float>>(get_model_params(std::string("brionac"), false), std::string(model_directory) + "\\" + "brionac.racy", device);
        }

        exposing::param_vector<brionac::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width,
            int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            std::map<std::string, float> params = {
            {"confidence", param_map.count("conf_thres") ? param_map["conf_thres"] : 0.3f},
            {"iou_threshold", param_map.count("iou_thres") ? param_map["iou_thres"] : 0.3f},
            {"hlow", param_map.count("hlow") ? param_map["hlow"] : 78.f},
            {"slow", param_map.count("slow") ? param_map["slow"] : 43.f},
            {"vlow", param_map.count("vlow") ? param_map["vlow"] : 46.f},
            {"hup", param_map.count("hup") ? param_map["hup"] : 124.f},
            {"sup", param_map.count("sup") ? param_map["sup"] : 255.f},
            {"vup", param_map.count("vup") ? param_map["vup"] : 255.f},
            {"hsv_area", param_map.count("valid_area_min") ? param_map["valid_area_min"] : 38000.f}
            };

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
                  throw exposing::abi_invalid_argument("incorrect roi in brionac");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width));
            //phase 1:detect the dash symbol
            cv::Mat blob;
            float resize_ratio;
            
            float cover_thres1 = params["hsv_area"];
            //bool is_cover=brionac_cover_judge(cropped_image, cover_thres1, params);

            auto new_shape = cv::Size(640,  640);
            std::tie(blob, resize_ratio) = preprocess_detection(cropped_image,new_shape);
 
            auto nms_result= phase_detect(blob, resize_ratio, params);//检测输入的是crop,resize 并cvtcolor的值
            auto fin_result = postprocess_of_cate(nms_result);

            std::string return_result_string;
         
            for(auto& x :fin_result)
            {
                return_result_string+= number_char_dict[x.category];

            }

            auto results = exposing::make_param_vector<brionac::box_info>();

            brionac::box_info_internal detect_info;
            if (fin_result.size()) 
            {
                detect_info.x1 = fin_result[0].x1;
                detect_info.y1 = fin_result[0].y1;
                detect_info.x2 = fin_result[fin_result.size() - 1].x2;
                detect_info.y2 = fin_result[fin_result.size() - 1].y2;   
            }
      
            detect_info.strinfos=glasssix::exposing::param_string(return_result_string);

            detect_info.strinfos=glasssix::exposing::param_string(return_result_string);
            results.push_back(glasssix::exposing::make_as_first<box_info_impl>(detect_info));

            return results;
        }

        static std::string version()
        {
            return "1.0.0";
        }

    private:

    std::vector<char >number_char_dict={ '0','1','2','3','4','5','6','7','8','9','.' };
    
    static inline float sigmoid_x(float x)
        {
        return static_cast<float>(1.f / (1.f + exp(-x)));
        }

        struct box_info_internal
        {
            float x1;
            float y1;
            float x2;
            float y2;
            std::string strinfos;
        };

        struct location_char
        {
        int x1;
        int y1;
        int x2;
        int y2;
        char symbol;
        int category;
        };

        typedef struct Bbox
        {
        int x;
        int y;
        int w;
        int h;
        float score;
        int category;
        };

        struct symbol_category
        {
        char  symbol;
        int   category;
        };



        struct boxes_conf
        {
        float top_x;
        float top_y;
        float bot_x;
        float bot_y;
        float conf;
        int category;
        };

        static bool sort_loca(location_char& l_c1, location_char& l_c2) {
        return (l_c1.x1 + l_c1.x2) < (l_c2.x1 + l_c2.x2);
        }

        static bool sort_score(Bbox box1, Bbox box2) {
        return box1.score > box2.score ? true : false;
        }



        std::vector<location_char> postprocess_of_cate(std::vector<location_char>& time_info)
        {
            std::vector<location_char> result;
            std::sort(time_info.begin(), time_info.end(), sort_loca);

            for (int i = 0; i < time_info.size(); i++)
            {
                location_char temp;
                temp = time_info[i];
                result.emplace_back(temp);
            }
            return result;
        }

        bool eledash_cover_judge(cv::Mat src, const float cover_thresh1, std::map<std::string, float>& param)
        {
        cv::Mat hsv;
        cv::Mat mask;
        std::vector<std::vector<cv::Point> > msk_con;
        cv::cvtColor(src, hsv, cv::COLOR_BGR2HSV);
        auto lower_blue = cv::Scalar(static_cast<int>(param["hlow"]), static_cast<int>(param["slow"]), static_cast<int>(param["vlow"]));
        auto upper_blue = cv::Scalar(static_cast<int>(param["hup"]), static_cast<int>(param["sup"]), static_cast<int>(param["vup"]));
        cv::inRange(hsv, lower_blue, upper_blue, mask);
        cv::findContours(mask, msk_con, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
        double max_area = 0;
        int max_area_idx = 0;
        for (int i = 0; i < msk_con.size(); i++)
        {
            double area = cv::contourArea(msk_con[i]);
            if (area >= max_area)
            {
                max_area = area;
                max_area_idx = i;
            }
        }
        int digi_area = 0;
        if (msk_con.size() > 0)
        {
            auto rec_box = cv::boundingRect(msk_con[max_area_idx]);
            digi_area = rec_box.width * rec_box.height;
        }

        if (digi_area >= cover_thresh1) //未遮挡
        {
            return false;
        }
        else
        {
            return true;
        }
        }


        std::pair<cv::Mat, float> preprocess_detection(cv::Mat src, const cv::Size& input_shape = cv::Size(640, 640))
        {
        float scale = std::min((float)input_shape.width / (float)src.cols, (float)input_shape.height / (float)src.rows);
        cv::Mat cut_image;
        cv::Mat mask_image(input_shape, CV_8UC3, cv::Scalar(114, 114, 114));

        if (src.rows != input_shape.height || src.cols != input_shape.width)
        {
            cv::resize(src, cut_image, cv::Size((int)(src.cols * scale), (int)(src.rows * scale)), cv::INTER_LINEAR);
            cv::copyMakeBorder(cut_image, mask_image, 0, input_shape.height - cut_image.rows, 0, input_shape.width - cut_image.cols, cv::BORDER_CONSTANT, cv::Scalar{ 114, 114, 114 });
        }
        else
        {
            src.copyTo(mask_image);
        }
        cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);

        return { mask_image,scale };
        }


        static std::vector<boxes_conf> yolo2xyxy(std::vector<std::vector<float>>& src, float conf_thres = 0.f)
        {
            std::vector<boxes_conf> res;
            for (auto it : src)
            {
                float top_x = it[0] - it[2] / 2;
                float top_y = it[1] - it[3] / 2;
                float bot_x = it[0] + it[2] / 2;
                float bot_y = it[1] + it[3] / 2;
                float conf = it[4];
                int maxPosition = std::max_element(it.begin() + 5, it.end()) - it.begin();
                if (conf > conf_thres)
                {
                    boxes_conf temp{};
                    temp.top_x = top_x;
                    temp.top_y = top_y;
                    temp.bot_x = bot_x;
                    temp.bot_y = bot_y;
                    temp.conf = conf;
                    temp.category = maxPosition - 5;
                    //if (temp.category == 0)
                    {
                        res.push_back(temp);
                    }

                }
            }
            return res;
        }





        static std::vector<Bbox> computeNmsInput(std::vector<boxes_conf>& src, int max_wh, float ratio)
        {
            std::vector<Bbox> boxes;
            std::vector<float> scores;
            std::vector<int> category;
            for (auto const& it : src)
            {
                int c = max_wh * it.conf;
                Bbox temp;
                temp.x = static_cast<double>(it.top_x) * ratio;
                temp.y = static_cast<double>(it.top_y) * ratio;
                temp.w = static_cast<double>(it.bot_x - it.top_x) * ratio;
                temp.h = static_cast<double>(it.bot_y - it.top_y) * ratio;
                temp.score = it.conf;
                temp.category = it.category;
                // if (temp.category == 0 || temp.category == 4)
                // {
                boxes.push_back(temp);
                // }
            }
            return boxes;
        }

        static float iou(Bbox box1, Bbox box2) {
        int x1 = std::max(box1.x, box2.x);
        int y1 = std::max(box1.y, box2.y);
        int x2 = std::min(box1.x + box1.w, box2.x + box2.w);
        int y2 = std::min(box1.y + box1.h, box2.y + box2.h);
        int w = std::max(0, x2 - x1);
        int h = std::max(0, y2 - y1);
        float over_area = w * h;
        return over_area / (box1.w * box1.h + box2.w * box2.h - over_area);
        }

        static std::vector<Bbox> nms(std::vector<Bbox>& boxes, float threshold)
        {
            std::vector<Bbox>resluts;
            std::sort(boxes.begin(), boxes.end(), sort_score);
            while (boxes.size() > 0)
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


        static std::vector<location_char> non_max_suppression(cv:: Mat& blob, std::vector<std::vector<float>>& prediction, float conf_thres, float iou_thres, float ratio)
        {
            // std::cout<<"nms inpu size "<<prediction.size()<<std::endl;

            auto compute_box = yolo2xyxy(prediction, conf_thres);
            // Batched NMS

            int max_wh = 4096;
            std::vector<Bbox> boxes;
            std::vector<float> scores;
            std::vector<int> classes;

            boxes = computeNmsInput(compute_box, max_wh, ratio);

            std::vector<Bbox> class_num;
            std::vector<Bbox> class_metra;
            for (auto& box : boxes)
            {
              
                    class_num.emplace_back(box);
              
            }
            auto bboxes_num = nms(boxes, iou_thres);
            std::vector<location_char> output_num;
            std::vector<location_char> output_metra;

            //auto f = [](int x){if(x<0) return 0; else return x;};

            for (auto it : bboxes_num)
            {
                location_char temp;
                temp.x1 = it.x;
                temp.x2 = it.x + it.w;
                temp.y1 = it.y;
                temp.y2 = it.y + it.h;
                temp.category = it.category;
                output_num.emplace_back(temp);
            }


            return output_num;
        }



        std::vector<std::vector<float>> concat(std::vector<std::shared_ptr<memory::tensor<float>>>& outs, float conf_thres)
        {
            const float anchors[3][6] = { {30,61, 62,45, 59,119},{116,90, 156,198, 373,326},{10,13, 16,30, 33,23} };
            const float stride[3] = { 16.0, 32.0, 8.0 };//40 20 80->   30 15 60
            std::vector<std::vector<float>> result;

            for (int n = 0; n < 3; n++)
            {
                int num_grid_x = (int)(640 / stride[n]);
                int num_grid_y = (int)(640 / stride[n]);

                int ind = 0;
                const float* ptr_out = outs[n]->cpu_data();

                for (int q = 0; q < 3; q++)
                {

                    const float anchor_w = anchors[n][q * 2];
                    const float anchor_h = anchors[n][q * 2 + 1];
                    for (int i = 0; i < num_grid_x; i++)
                    {
                        for (int j = 0; j < num_grid_y; j++)
                        {
                            // float* pdata = (float*)outs[n].data + ind *  outs[n].size[4];
                            const float* pdata = ptr_out + ind * 16;
                            float box_score = sigmoid_x(pdata[4]);
                            // if(box_score > 0.f)
                            // {
                            float cx = (sigmoid_x(pdata[0]) * 2.f - 0.5f + j) * stride[n];  //cx
                            float cy = (sigmoid_x(pdata[1]) * 2.f - 0.5f + i) * stride[n];  //cy
                            float w = powf(sigmoid_x(pdata[2]) * 2.f, 2.f) * anchor_w;      //w
                            float h = powf(sigmoid_x(pdata[3]) * 2.f, 2.f) * anchor_h;      //h

                            std::vector<float> element = { cx, cy, w, h, box_score };
                            for (size_t i = 0; i < 11; i++)
                            {
                                element.push_back(sigmoid_x(pdata[i + 5]));
                            }
                            result.push_back(element);
                            // }
                            ind++;
                        }
                    }
                }
            }
            return result;
        }

    std::vector<location_char> phase_detect(cv::Mat& blob,float ratio, std::map<std::string, float>& param)
    {
        std::shared_ptr<memory::tensor<uint8_t>> input(new memory::tensor<uint8_t>(3, blob.rows, blob.cols, -1, memory::NHWC, nullptr));//注意一般图片应该是NHWC  后面需要调整顺序 
    
        std::copy(blob.data, blob.data + blob.channels()* blob.rows*blob.cols, input->mutable_cpu_data());
        input->convert_order();
        auto input_tensor = input | glasssix::memory::tensor_convert_to<float>;
        
        std::vector<std::shared_ptr<memory::tensor<float>>> Phase1_Results;
        auto result_detect =(*dash_detect).forward(input_tensor);    

        float conf_threshold =0.5;
        float iou_threshold = 0.4;

        Phase1_Results.push_back(result_detect["292"]);//480
        Phase1_Results.push_back(result_detect["304"]);
        Phase1_Results.push_back(result_detect["output"]);

        auto result = concat(Phase1_Results, conf_threshold);

        
        auto nms_result = non_max_suppression(blob,result, conf_threshold, iou_threshold, 1 / ratio);
        return nms_result;
    }

    private:
        std::string model_directory_;
        int device_;
        std::unique_ptr < excalibur::pipeline<float>> dash_detect;

    };

    classify_code_internal::classify_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    classify_code_internal::~classify_code_internal() = default;

    std::string classify_code_internal::version()
    {
        return impl::version();
    }

    exposing::param_vector<brionac::box_info> classify_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, 
        int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}
