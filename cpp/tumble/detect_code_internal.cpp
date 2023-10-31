#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <RKNN2Wrapper/rknn2_wrapper.hpp>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>
#include <abi/param_vector.hpp>
#include <utility>

namespace glasssix::tumble
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{get_model_params("tumble", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device) 
            :detect_instance_(phai,  model_directory + std::string("/tumble_sim.rknn"), device), model_directory_(model_directory)
        {

        }       

        exposing::param_vector<tumble::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);

            auto result = exposing::make_param_vector<box_info>();

            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
            {
                  throw exposing::abi_invalid_argument("incorrect roi in tumble");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width)).clone();
            
            auto detect_result = run_detect(cropped_image, param_map);

            for (auto& i : detect_result)
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
			std::string nn_frame_version = detect_instance_.version();
#else
			std::string nn_frame_version = detect_instance_.version();
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
         * @param data, stride
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
         * @param data, stride
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

        struct bbox
        {
            float x;
            float y;
            float w;
            float h;
            float conf;
        };

        /**
         * @fun concat
         * @param outs, conf_thres
         * @return bbox 
         * @details concat data into bbox xywh
         */
        std::vector<std::array<float,6>> concat(std::vector<std::shared_ptr<glasssix::memory::tensor<float>>>& outs, float conf_thres)
        {
            std::vector<float> concat_array(66 * 8400, 0);
            
            const float* Ptr80  = outs[0]->cpu_data();
            const float* Ptr40  = outs[1]->cpu_data();
            const float* Ptr20  = outs[2]->cpu_data();

            for(int i = 0; i < 66; i++)
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
			std::vector<int>   candidate_types;

            for(int i = 0; i < 8400; i++)
            {
                float temp1  = sigmoid_x(concat_array[64 * 8400 + i]);
				float temp2  = sigmoid_x(concat_array[65 * 8400 + i]);
				
				float thres = 0.f;
				if(temp1 > temp2)
				{
					thres = temp1;
					type  = 0;
				}
				else
				{
					thres = temp2;
					type  = 1;
				}
				
                if(thres > conf_thres)
                {
                    candidate_index.push_back(i);
                    candidate_thres.push_back(thres);
					candidate_types.push_back(type);
                }
            }

            if(candidate_index.empty())
            {
                return std::vector<std::array<float, 6>>();
            }
            else 
            {
                int candidate_num = candidate_index.size();

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

                std::array<float, 6> bbox;
                std::vector<std::array<float, 6>> bboxs;

                for(int i = 0; i < candidate_num; i++)
                {
                    bbox[0] = concat_array_2[i]                     * mul[candidate_index[i]];
                    bbox[1] = concat_array_2[i + candidate_num]     * mul[candidate_index[i]];
                    bbox[2] = concat_array_2[i + candidate_num * 2] * mul[candidate_index[i]];
                    bbox[3] = concat_array_2[i + candidate_num * 3] * mul[candidate_index[i]];
                    bbox[4] = candidate_thres[i];
					bbox[5] = candidate_types[i];
                    bboxs.push_back(bbox);
                }

                return bboxs;
            }
        }

        /*
        @fun non_max_suppression
        */
        std::vector<std::array<float, 6>> non_max_suppression(std::vector<std::array<float, 6>> pred, float conf_thres, float iou_thres)
        {
            // generate NMS data
            std::vector<cv::Rect2d> stand_boxes;
            std::vector<float>      stand_scores;

            std::vector<cv::Rect2d> tumble_boxes;
            std::vector<float>      tumble_scores;

            for(auto &it : pred)
            {
                cv::Rect2d rect;
                
                if(it[5] == 0)
                {
                    rect.x = it[0] - it[2] / 2;
                    rect.y = it[1] - it[3] / 2;
                    rect.width = it[2];
                    rect.height = it[3];
                    stand_boxes.push_back(rect);
                    stand_scores.push_back(it[4]);
                }
                else if(it[5] == 1)
                {
                    rect.x = it[0] - it[2] / 2;
                    rect.y = it[1] - it[3] / 2;
                    rect.width = it[2];
                    rect.height = it[3];
                    tumble_boxes.push_back(rect);
                    tumble_scores.push_back(it[4]);
                }

            }

            std::vector<std::array<float, 6>> bboxes;

            if(stand_scores.size() != 0)
            {
                // stand_scores
                std::vector<int> stand_indices;
                cv::dnn::NMSBoxes(stand_boxes, stand_scores, conf_thres, iou_thres, stand_indices, 1.f, 0);

                for(int i = 0; i < stand_indices.size(); i++)
                {
                    std::array<float, 6> box;           // box: x1, y1, x2, y2

                    box[0] = stand_boxes[stand_indices[i]].x;
                    box[1] = stand_boxes[stand_indices[i]].y;
                    box[2] = stand_boxes[stand_indices[i]].x + stand_boxes[stand_indices[i]].width;
                    box[3] = stand_boxes[stand_indices[i]].y + stand_boxes[stand_indices[i]].height;

                    box[4] = stand_scores[stand_indices[i]];

                    box[5] = 0;

                    bboxes.push_back(box);
                }
            }

            if(tumble_scores.size() != 0)
            {
                // tumble_scores
                std::vector<int> tumble_indices;
                cv::dnn::NMSBoxes(tumble_boxes, tumble_scores, conf_thres, iou_thres, tumble_indices, 1.f, 0);

                for(int i = 0; i < tumble_indices.size(); i++)
                {
                    std::array<float, 6> box;           // box: x1, y1, x2, y2

                    box[0] = tumble_boxes[tumble_indices[i]].x;
                    box[1] = tumble_boxes[tumble_indices[i]].y;
                    box[2] = tumble_boxes[tumble_indices[i]].x + tumble_boxes[tumble_indices[i]].width;
                    box[3] = tumble_boxes[tumble_indices[i]].y + tumble_boxes[tumble_indices[i]].height;

                    box[4] = tumble_scores[tumble_indices[i]];

                    box[5] = 1;

                    bboxes.push_back(box);
                }
            }

            return bboxes;
        }

        /**
        * @fun scale_coord
        */
        std::array<float, 6> scale_coord(const std::array<float, 6>& coords, cv::Size& input_shape, cv::Size& output_shape)
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

            std::array<float, 6> scale_pt = { x1, y1, x2, y2, coords[4], coords[5]};

            return scale_pt;
        }

        /**
        * @fun reset
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
        */
        std::vector<tumble::box_info_internal> run_detect(cv::Mat& image, std::map<std::string, float>& param_map)
        {
            float conf_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.25f;
            float iou_thres  = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.65f;     
            // preprocess
            auto input_shape = cv::Size(640,  640);

            auto output_shape = cv::Size(image.cols, image.rows);
            
            cv::Mat blobs;
            float ratio = 0;
            std::tie (blobs, ratio) = letterbox(image, 640);

            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forwards;
            std::vector<std::string>  phais;

            auto  network_results = detect_instance_.forward(blobs.data, { 1, blobs.rows, blobs.cols,blobs.channels() }, RKNN_TENSOR_NHWC);

            // forwards 
            forwards.push_back(network_results["onnx::Concat_363"]);
			forwards.push_back(network_results["onnx::Concat_370"]);
			forwards.push_back(network_results["onnx::Concat_377"]);

            auto concat_result = concat(forwards, conf_thres, blobs);

            // NMS
            auto nms_result = non_max_suppression(concat_result, conf_thres, iou_thres);

            // scale_coords
            std::vector<tumble::box_info_internal> detect_result;

            for(auto &it: nms_result)
            {
                tumble::box_info_internal box_info;

                auto scale_coords = scale_coord(it, input_shape, output_shape);

                box_info.x1 = reset(scale_coords[0], image.cols); 
                box_info.y1 = reset(scale_coords[1], image.rows); 
                box_info.x2 = reset(scale_coords[2], image.cols);
                box_info.y2 = reset(scale_coords[3], image.rows);

                box_info.score = scale_coords[4];
                box_info.category = static_cast<int>(scale_coords[5]); 

                detect_result.push_back(box_info);
            }

            return detect_result;
        }

    private:
        std::string model_directory_;
        int device_;
        glasssix::rknnwrapper::rknn_wrapper detect_instance_;
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

    exposing::param_vector<tumble::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}
