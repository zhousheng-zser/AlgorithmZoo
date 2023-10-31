#include <iostream>
#include <cmath>
#include <tuple>

#include "../posture/detect_code.hpp"

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <abi/param_vector.hpp>
#include <utility>

namespace glasssix::smoke
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{get_model_params("smoke", false),  exposing::to_narrow_string(model_directory), device} 
        {

        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device) 
            :cigarette_detect_instance_(phai,  model_directory + std::string("/cigarette_detect.rknn"), device), model_directory_(model_directory)
        {
            static bool ready = glasssix::exposing::get_component_loader().add_module_by_name("posture");
            posture_instance_ = glasssix::exposing::make_exported_interface<posture::detect_code>(exposing::param_string(model_directory), device);
            posture_param_abi = exposing::make_param_hash_map<exposing::param_string, float>();
			posture_param_abi.add_or_update("conf_thres", 0.0f);
			posture_param_abi.add_or_update("nms_thres", 0.30f);
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

            std::vector<smoke::box_info_internal> results;
            auto result = exposing::make_param_vector<box_info>();

            auto empty_map_abi = exposing::make_param_hash_map<exposing::param_string, float>();
            exposing::param_vector<posture::box_info> posture_info_list = posture_instance_.detect(bitmap, channels, height, width, 0, 0, width, height, posture_param_abi);
            
            std::vector<PostureInfo> persons_info;

            for (auto pinfo : posture_info_list) {
                PostureInfo postureInfo{ pinfo };

                // show pinfo score
                std::cout << "pinfo score: " << postureInfo.score << std::endl;

                // get key 0 - key 4 min xy max xy
                std::vector<float> x_vec;
                std::vector<float> y_vec;

                std::vector<float> pred_vec = {postureInfo.Kpoints[0].second , postureInfo.Kpoints[1].second , postureInfo.Kpoints[2].second};

                int sum = 0;

                for(auto &it: pred_vec)
                {
                    if(it < 0.8) 
                    { 
                        sum += 1;
                    }
                }

                if(sum < 2)
                {
                    constexpr auto limit = [](float x) {if (x < 0) return 0.0f ; else return x; };

                    // 嘴巴部分框体计算
                    for(int i = 0; i < 5; i++)
                    {   
                        x_vec.push_back(postureInfo.Kpoints[i].first.x);
                        y_vec.push_back(postureInfo.Kpoints[i].first.y);
                    }

                    // 取0-4号点的框体范围的左上右下
                    float min_local_x = *std::min_element(x_vec.begin(), x_vec.end());
                    float max_local_x = *std::max_element(x_vec.begin(), x_vec.end());
                    float min_local_y = *std::min_element(y_vec.begin(), y_vec.begin() + 2);
                    float max_local_y = *std::max_element(y_vec.begin(), y_vec.begin() + 2);

                    // 口框体左上右下
                    float mouth_left_x  = limit(postureInfo.Kpoints[0].first.x - (max_local_x - min_local_x) / 3.5f);
                    float mouth_left_y  = limit(postureInfo.Kpoints[0].first.y);
                    float mouth_right_x = limit(postureInfo.Kpoints[0].first.x + (max_local_x - min_local_x) / 3.5f);
                    float mouth_right_y = limit(postureInfo.Kpoints[0].first.y + (max_local_y - min_local_y) * 2);

                    // cigarette detect
                    x_vec.clear();
                    y_vec.clear();
            
                    for(int i = 5; i < 9; i++)
                    {   
                        x_vec.push_back(postureInfo.Kpoints[i].first.x);
                        y_vec.push_back(postureInfo.Kpoints[i].first.y);
                    }

                    x_vec.push_back(postureInfo.Kpoints[0].first.x);
                    y_vec.push_back(postureInfo.Kpoints[0].first.y);

                    // show 5 - 9 circle on image 
                    for(int i = 0; i < 4; i++)
                    {
                        cv::circle(image, cv::Point(x_vec[i], y_vec[i]), 2, cv::Scalar(0, 255, 0), 2);
                    }

                    cv::imwrite("posture.jpg", image);

                    // min_x = 左上人体框体的x坐标, min_y = 鼻子0号点的y坐标
                    float min_x = 0;

                    if(postureInfo.x1 < postureInfo.x2) 
                        min_x = postureInfo.x1;
                    else
                        min_x = postureInfo.x2;

                    float min_y = postureInfo.Kpoints[0].first.y;

                    // max_x = 右下人体坐标框体的x坐标, max_y = 坐标点中最大y的坐标
                    float max_x = 0;

                    if(postureInfo.x1 < postureInfo.x2) 
                        max_x = postureInfo.x2;
                    else
                        max_x = postureInfo.x1;

                    float max_y = *std::max_element(y_vec.begin(), y_vec.end());

                    min_x = limit(min_x);
                    min_y = limit(min_y);                 
                    max_x = limit(max_x);
                    max_y = limit(max_y);

                    if(min_x > (image.cols - 1))
                    {
                        min_x = image.cols - 1;
                    }	
                    if(min_y > (image.rows - 1))
                    {
                        min_y = image.rows - 1;
                    }
                    if(max_x > (image.cols - 1))
                    {
                        max_x = image.cols - 1;	
                    }
                    if(max_y > (image.rows - 1))
                    {
                        max_y = image.rows - 1;
                    }

                    if((max_x - min_x < 0) || (max_y - min_y < 0) || (max_x - min_x == 0) ||(max_y - min_y == 0))
                    {
                        continue;
                    }
                    else
                    {
                        cv::Point min_pt(min_x, min_y);
                        cv::Point max_pt(max_x, max_y);
                        
                        cv::Mat cropped_image = image(cv::Range(min_pt.y, max_pt.y), cv::Range(min_pt.x, max_pt.x)).clone();
                        // cropped_image check 
                        cv::imwrite("cropped_image.jpg", cropped_image);

                        // cigarette detect
                        auto detect_result = run_detect(cropped_image, param_map);

                        // check area 
                        float mouth_area = (mouth_right_x - mouth_left_x) * (mouth_right_y - mouth_left_y);

                        smoke::box_info_internal box_info;

                        if(detect_result.empty())
                        {
                            box_info.x1 = min_x;
                            box_info.y1 = min_y;  
                            box_info.x2 = max_x;
                            box_info.y2 = max_y;
                            box_info.confidence = postureInfo.score;
                            box_info.category = 1;

                            results.push_back(box_info);
                        }
                        else 
                        {
                            for(auto &it: detect_result)
                            {
                                
                                float cigarette_area = (it[3] - it[1]) * (it[2] - it[0]);

                                // darw rectangle on cropped_image
                                cv::rectangle(cropped_image, cv::Point(it[0], it[1]), cv::Point(it[2], it[3]), cv::Scalar(0, 255, 0), 2);

                                cv::imwrite("cigarette_detect_cropped_image.jpg", cropped_image);

                                if(cigarette_area > mouth_area)
                                {
                                    box_info.x1 = 0;
                                    box_info.y1 = 0;  
                                    box_info.x2 = 0;
                                    box_info.y2 = 0;
                                    box_info.confidence = 0;
                                    box_info.category = 0;

                                    results.push_back(box_info);
                                }
                                else 
                                {
                                    box_info.x1 = it[0] + min_x;
                                    box_info.y1 = it[1] + min_y;  
                                    box_info.x2 = it[2] + min_x;
                                    box_info.y2 = it[3] + min_y;
                                    box_info.confidence = it[4];

                                    int mouth_x1 = static_cast<int>(mouth_left_x);
                                    int mouth_y1 = static_cast<int>(mouth_left_y);
                                    int mouth_x2 = static_cast<int>(mouth_right_x);
                                    int mouth_y2 = static_cast<int>(mouth_right_y);

                                    box_info.category = !is_rect_cross(box_info.x1, box_info.y1, box_info.x2, box_info.y2, mouth_x1, mouth_y1, mouth_x2, mouth_y2);
                                }

                                results.push_back(box_info);
                            }
                        }
                    }
                }
            }

            for (auto& i : results)
            {
                i.x1+=roi_x;
                i.x2+=roi_x;
                i.y1+=roi_y;
                i.y2+=roi_y;
                
                i.x1= i.x1>0?i.x1:0;
                i.x1= i.x1<width?i.x1:width;

                i.x2= i.x2>0?i.x2:0;
                i.x2= i.x2<width?i.x2:width;

                i.y1= i.y1>0?i.y1:0;
                i.y1= i.y1<height?i.y1:height;

                i.y2= i.y2>0?i.y2:0;
                i.y2= i.y2<height?i.y2:height;

                result.push_back(exposing::make_as_first<box_info_impl>(i));
            }   

            return result;
        }

       
        std::string version()
        {
            const std::string algo_module_version = "1.0.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        //#if 0
            std::string nn_frame_version = cigarette_detect_instance_.version();
#else
            std::string nn_frame_version = cigarette_detect_instance_.version();
#endif
        return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    private:
        /**
         * @fun letterbox
         * @param src, new_shape
         * @return tensor(preprocess(image))
         * @details image preprocess and make tensor from images
         */
        std::tuple<cv::Mat, float> letterbox(cv::Mat img, int hope_size = 640)
        {
            int H = img.rows;
            int W = img.cols;
            float ratio_w = (float)W / (float)hope_size;
            float ratio_h = (float)H / (float)hope_size;
            float ratio = ratio_w;
            cv::Mat resize_img;
            if(H==hope_size && W==hope_size )
            {
                resize_img=img;
            }
            else
            {
                if (ratio_w == ratio_h)
                {

                    cv::resize(img, resize_img, cv::Size2i{ hope_size, hope_size });}
                else if (ratio_w > ratio_h) {

                    int new_x = hope_size;
                    int new_y = (int)(H / ratio_w);
                    int pad1 = (int)((hope_size - new_y) / 2);
                    int pad2 = hope_size - new_y - pad1;
                    cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });

                    cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
                }
                else {

                    ratio = ratio_h;
                    int new_y = hope_size;
                    int new_x = (int)(W / ratio_h);
                    int pad1 = (int)((hope_size - new_x) / 2);
                    int pad2 = hope_size - new_x - pad1;

                    cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });

                    cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
                }
            }

            return { resize_img, ratio };
        }

        /**
        * @fun sigmoid_x
        * @param 
        * @return sigmoid(x)
        */
        inline float sigmoid_x(float x)
        {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

        /**
         * @fun Softmax
         * @param data, num
         * @return softmax(data) between stride 
         * @detail
         */
        void  Softmax(float *data, int num )
        {
            float sum = 0.f;
            float temp[16] = {0};

            // find max value in data
            float max = data[0];
            for(int i = 1; i < num; i++)
            {
                if(data[i] > max)
                {
                    max = data[i];
                }
            }

            for(int i = 0; i < num; i++)
            {
                temp[i] = exp(data[i] - max);

                sum += temp[i];
            }
            for(int i = 0; i < num; i++)
            {
                data[i] = temp[i] / sum;
            }
        }

        /**
         * @fun 1D-Conv1x1
         * @param data, result, num
         * @return 1D-Conv1x1(data, kernel, stride)
         * @detail
         */
        void Conv1x1(float* data, float* result, int num)
        {
            float kernel[16] = {0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f,
                                    8.f, 9.f, 10.f, 11.f, 12.f, 13.f, 14.f, 15.f};
            
            // reset resultPtr to zero
            float res = 0.f;
            // 1D-Conv1x1
            for(int i = 0; i < num; i++)
            {
                res += data[i] * kernel[i];
            }
            *result = res;
        }

        /**
         * @fun concat
         * @param outs, conf_thres
         * @return bbox 
         * @details concat data into bbox xywh
         */
        std::vector<std::array<float,5>> concat(std::vector<std::shared_ptr<glasssix::memory::tensor<float>>>& outs, float conf_thres)
        {
            std::vector<float> concat_array(65 * 8400, 0);
            
            const float* Ptr80  = outs[0]->cpu_data();
            const float* Ptr40  = outs[1]->cpu_data();
            const float* Ptr20  = outs[2]->cpu_data();

            for(int i = 0; i < 65; i++)
            {
                int j = 0;
                // step + offset
                for(;j < 6400; j++)
                    concat_array[i * 8400 + j] = Ptr80[i * (80 * 80) + j];
                for(;j < 8000; j++)
                    concat_array[i * 8400 + j] = Ptr40[ i * (40 * 40) + j - 6400];
                for(;j < 8400; j++)
                    concat_array[i * 8400 + j] = Ptr20[ i * (20 * 20) + j - 8000];
            }

            // spilt
            // simgoid for threshold
            // create candidate_index vector
            std::vector<int>   candidate_index;
            std::vector<float> candidate_thres;

            for(int i = 0; i < 8400; i++)
            {
                float temp  = sigmoid_x(concat_array[64 * 8400 + i]);
                if(temp > conf_thres)
                {
                    candidate_index.push_back(i);
                    candidate_thres.push_back(temp);
                }
            }

            if(candidate_index.empty())
            {
                return std::vector<std::array<float, 5>>();
            }
            else 
            {
                // candidate_num
                int candidate_num = candidate_index.size();

                std::cout << "candidate_num: " << candidate_num << std::endl;

                // select candidate_array from concat_array
                std::vector<float> candidate_array(64 * candidate_num, 0);
                for(int i = 0; i < 64; i++)
                {
                    for(int j = 0; j < candidate_num; j++)
                    {
                        candidate_array[i * candidate_num + j] = concat_array[i * 8400 + candidate_index[j]];
                    }
                }

                // transpose candidate_array into reshape_array
                std::vector<float> reshape_array(candidate_num * 64, 0);
                // transpose
                for(int i = 0; i < 64; i++)
                {
                    for(int j = 0; j < candidate_num; j++)
                    {
                        reshape_array[j * 64 + i] = candidate_array[i * candidate_num + j];
                    }
                }

                // softmax 16 stride for reshape_array
                for(int i = 0; i < candidate_num; i++)
                {
                    for(int j = 0; j < 4; j++)
                    {
                        Softmax(&reshape_array[i * 64 + j * 16], 16);
                    }
                }

                // 1D-Conv1x1
                std::vector<float> conv_array(candidate_num * 4, 0);
                for(int i = 0; i < candidate_num; i++)
                {
                    for(int j = 0; j < 4; j++)
                    {
                        Conv1x1(&reshape_array[i * 64 + j * 16], &conv_array[i * 4 + j], 16);
                    }
                }
                // conv_array is equeal to onnx-inference

                // create anchor array
                std::vector<float> anchor_array(8400 * 2);
                
                for(int i=0; i<6400; i++)
                {
                    anchor_array[i]=i%80-0.5f+1.f;
                }
                for(int i=0; i<1600; i++)
                {
                    anchor_array[6400+i]=i%40-0.5f+1.f;
                }
                for(int i=0; i<400; i++)
                {
                    anchor_array[8000+i] = i%20-0.5f+1.f;
                }

                for(int i=0; i<6400; i++)
                {
                    anchor_array[8400+i]=i/80-0.5f+1.f;
                }
                for(int i=0; i<1600; i++)
                {
                    anchor_array[8400+6400+i]=i/40-0.5f+1.f;
                }
                for(int i=0; i<400; i++)
                {
                    anchor_array[8400+8000+i] = i/20-0.5f+1.f;
                }


                // create bbox array
                // create sub array and add array
                std::vector<float> sub_array(candidate_num * 2);
                std::vector<float> add_array(candidate_num * 2);

                for(int i = 0; i < candidate_num * 4; i += 4)
                {
                    sub_array[i / 4]                 = anchor_array[candidate_index[i / 4]] - conv_array[i];
                    sub_array[i / 4 + candidate_num] = anchor_array[candidate_index[i / 4]  + 8400] - conv_array[i + 1];

                    add_array[i / 4]                 = anchor_array[candidate_index[i / 4]] + conv_array[i + 2];
                    add_array[i / 4 + candidate_num] = anchor_array[candidate_index[i / 4]  + 8400] + conv_array[i + 3];
                }

                // sub add data checked
                std::vector<float> sub_array_2(candidate_num * 2);
                std::vector<float> add_array_2(candidate_num * 2);

                for(int i = 0; i < candidate_num; i++)
                {
                    add_array_2[i]                 = sub_array[i] + add_array[i];
                    add_array_2[i + candidate_num] = sub_array[i + candidate_num] + add_array[i + candidate_num];

                    sub_array_2[i]                 = add_array[i] - sub_array[i];
                    sub_array_2[i + candidate_num] = add_array[i + candidate_num] - sub_array[i + candidate_num];
                }

                // create concat array
                std::vector<float> concat_array_2(candidate_num * 4);

                for(int i = 0; i < candidate_num; i++)
                {
                    concat_array_2[i]                     = add_array_2[i] / 2.f;
                    concat_array_2[i + candidate_num]     = add_array_2[i + candidate_num] / 2.f;
                    concat_array_2[i + candidate_num * 2] = sub_array_2[i];
                    concat_array_2[i + candidate_num * 3] = sub_array_2[i + candidate_num];
                }
                // concat_array checked

                // Mul
                std::vector<float> mul(8400);

                for(int i = 0; i < 8400; i++)
                {
                    // 80 * 80
                    if(i < 6400)
                        mul[i] = 8.f;
                    // 40 * 40
                    else if(i < 8000)
                        mul[i] = 16.f;
                    // 20 * 20
                    else
                        mul[i] = 32.f;
                }

                std::array<float, 5> bbox;
                std::vector<std::array<float, 5>> bboxs;

                for(int i = 0; i < candidate_num; i++)
                {
                    bbox[0] = concat_array_2[i]                     * mul[candidate_index[i]];
                    bbox[1] = concat_array_2[i + candidate_num]     * mul[candidate_index[i]];
                    bbox[2] = concat_array_2[i + candidate_num * 2] * mul[candidate_index[i]];
                    bbox[3] = concat_array_2[i + candidate_num * 3] * mul[candidate_index[i]];
                    bbox[4] = candidate_thres[i];
                    bboxs.push_back(bbox);
                }

                return bboxs;
            }
        }

        /**
         * @fun post_process
         * @param outs, conf_thres, iou_thres
         * @return nms_bbox
         */
        std::vector<std::array<float, 5>> post_process(std::vector<std::array<float, 5>>& outs, float conf_thres, float iou_thres)
        {
            // use dnn::NMSBoxes
            std::vector<cv::Rect2d> bboxes;
            std::vector<float> confidences;
            std::vector<int> indices(outs.size());

            for(int i = 0; i < outs.size(); i++)
            {    
                cv::Rect2d box;
                box.x      = static_cast<double>(outs[i][0] - outs[i][2] / 2.f);
                box.y      = static_cast<double>(outs[i][1] - outs[i][3] / 2.f);
                box.width  = static_cast<double>(outs[i][2]);
                box.height = static_cast<double>(outs[i][3]);
                bboxes.push_back(box);
                confidences.push_back(outs[i][4]);
            }

            cv::dnn::NMSBoxes(bboxes, confidences, conf_thres, iou_thres, indices, 1.f, 0);

            std::array<float, 5> bbox;
            std::vector<std::array<float, 5>> nms_bbox;

            for(int i = 0; i < indices.size(); i++)
            {
                bbox[0] = outs[indices[i]][0] - outs[indices[i]][2] / 2.f;
                bbox[1] = outs[indices[i]][1] - outs[indices[i]][3] / 2.f;
                bbox[2] = outs[indices[i]][0] + outs[indices[i]][2] / 2.f;
                bbox[3] = outs[indices[i]][1] + outs[indices[i]][3] / 2.f;

                bbox[4] = confidences[indices[i]];

                nms_bbox.push_back(bbox);
            }

            // check finish
            return nms_bbox;
        }
        
        /**
        * @fun scale_coord
        * @param coords,input_shape,output_shape
        * @return 
        */
        std::array<float, 5> scale_coord(const std::array<float, 5>& coords, cv::Size& input_shape, cv::Size& output_shape)
        {
            auto clamp = [](int x, int min, int max) {if (x < min) return min; else if (x > max) return max; else return x; };

            // gain
            float gain = std::min(input_shape.width / (float)output_shape.width, input_shape.height / (float)output_shape.height);

            // pad
            float pad_w = (input_shape.width - output_shape.width * gain) / 2.0;
            float pad_h = (input_shape.height - output_shape.height * gain) / 2.0;

            // x padding
            // y padding
            float x1 = (coords[0] - pad_w) / gain;
            float y1 = (coords[1] - pad_h) / gain;
            float x2 = (coords[2] - pad_w) / gain;
            float y2 = (coords[3] - pad_h) / gain;

            clamp(x1, 0, output_shape.width);
            clamp(y1, 0, output_shape.height);
            clamp(x2, 0, output_shape.width);
            clamp(y2, 0, output_shape.height);

            std::array<float, 5> scale_pt = { x1, y1, x2, y2, coords[4]};

            return scale_pt;
        }

        /**
         * @fun reset
         * @param x, size
         * @return reset(x) into cv::Size
        */
        int reset(float x, int size)
        {
            if(x < 0)
                return 0;
            else if (x > size)
                return x;
            else 
                return static_cast<int>(x);
        }

        /**
         * @fun run_detect
         * @param image, param_map
         * @return bbox
         */
        std::vector<std::array<float,5>> run_detect(cv::Mat& image, std::map<std::string, float>& param_map)
        {
            
            float conf_threshold= param_map.count("conf_thres") ? param_map["conf_thres"] : 0.75f;
            float iou_threshold = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.45f;      

            // preprocess
            auto input_shape = cv::Size(640,  640);

            auto output_shape = cv::Size(image.cols, image.rows);
            
            cv::Mat blobs;
            float ratio = 0;
            std::tie (blobs, ratio) = letterbox(image, 640);

            cv::cvtColor(blobs, blobs, cv::COLOR_BGR2RGB);

            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forwards;
            std::vector<std::string>  phais;

            auto  network_results = cigarette_detect_instance_.forward(blobs.data, { 1, blobs.rows, blobs.cols,blobs.channels() }, RKNN_TENSOR_NHWC);

            forwards.push_back(network_results["output0"]);
            forwards.push_back(network_results["340"]);
            forwards.push_back(network_results["355"]);

            auto concat_output = concat(forwards, conf_threshold);

            if(concat_output.empty())
            {
                return  std::vector<std::array<float,5>>();
            } 
            else
            {
                // post_process
                auto nms_result = post_process(concat_output, conf_threshold, iou_threshold);

                // scale_coords
                std::vector<std::array<float, 5>> detect_result;

                for(auto &it: nms_result)
                {
                    std::array<float, 5> box_info;

                    auto scale_coords = scale_coord(it, input_shape, output_shape);

                    box_info[0] = reset(scale_coords[0], image.cols); 
                    box_info[1] = reset(scale_coords[1], image.rows); 
                    box_info[2] = reset(scale_coords[2], image.cols);
                    box_info[3] = reset(scale_coords[3], image.rows);
                    box_info[4] = scale_coords[4];

                    detect_result.push_back(box_info);
                }
                
                return detect_result;
            }
        }
        
        /**
         * @fun is_rect_cross
         * @param box1_x1, box1_y1, box1_x2, box1_y2, box2_x1, box2_y1, box2_x2, box2_y2, 
         * 
         */
        bool is_rect_cross(int box1_x1, int box1_y1, int box1_x2, int box1_y2, int box2_x1, int box2_y1, int box2_x2, int box2_y2) {
            // 判断矩形是否相交
            if (std::max(box1_x1, box2_x1) > std::min(box1_x2, box2_x2) || std::max(box1_y1, box2_y1) > std::min(box1_y2, box2_y2))
                return false;
            else
                return true;
        }

    private:
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)

		rknnwrapper::rknn_wrapper cigarette_detect_instance_;
#else
		std::unique_ptr<excalibur::pipeline<float>> cigarette_detect_instance_;
#endif
        std::string model_directory_;
        posture::detect_code posture_instance_;
        exposing::param_hash_map<exposing::param_string, float> posture_param_abi;
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
