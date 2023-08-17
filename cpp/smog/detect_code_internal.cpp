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

namespace glasssix::smog
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{get_model_params("smog", false),  exposing::to_narrow_string(model_directory), device} 
        {

        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
                :net_detect_(phai,  model_directory + std::string("/smog_sim.rknn"), device), model_directory_(model_directory)
        {   
           

        }

        exposing::param_vector<smog::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
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
                  throw exposing::abi_invalid_argument("incorrect roi in smog");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width)).clone();

            auto detect_result = run_detect(cropped_image, roi_x, roi_y, roi_width, roi_height, param_map);

            auto results = exposing::make_param_vector<smog::box_info>();

            for(auto& it:detect_result) 
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
    	
        /**
		 * @fun sigmoid_x
		 * @param x
		 * @return sigmoid(x)
		 */
		static inline float sigmoid(float x)
		{
			return static_cast<float>(1.f / (1.f + exp(-x)));
		}

        
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
		 * @fun concat
		 * @param infer_out, conf_thres
		 * @return source
		 * @details concat 3 into 1
		 */
        std::vector<std::array<float, 5>> concat(std::vector<std::shared_ptr<glasssix::memory::tensor<float>>>& prediction, float conf_thres, cv::Mat& blobs)
        {
            std::vector<std::array<float, 5>> concat_result;

            // ["onnx::Sigmoid_380"] = NCHW
            int channels = 1;
            int width    = 1;
            int height   = 8400;
            int num      = 1;
            const float *thres_data_ptr = prediction[1]->cpu_data();

            std::vector<int>   select_vec;
            std::vector<float> score_vec;

            int stride = channels * width;
            
            for(int i = 0; i < 8400; i++)
            {
                float score = sigmoid(thres_data_ptr[0]);
                
                if(score > conf_thres)
                {
                    select_vec.push_back(i);
                    score_vec.push_back(score);
                }

                thres_data_ptr += stride;
            }

            if(select_vec.size() == 0)
            {   
                std::array<float, 5> zero = {0,0,0,0,0};
                concat_result.push_back(zero);
                return concat_result;
            } 

            // use for position data ["onnx::Mul_423"]
            channels = 4;

            stride = channels * width;


            for(int i = 0; i < select_vec.size(); i++)
            {
                const float *pt_data_ptr = prediction[0]->cpu_data();
                
                std::array<float, 5> pt = {0, 0, 0, 0, 0};
                
                int index = select_vec[i];

                pt_data_ptr = pt_data_ptr + index;

                if(index < 6400)
                {
                    int scale = 8;
                    pt[0] = pt_data_ptr[0 + 0 * 8400] * scale;
                    pt[1] = pt_data_ptr[0 + 1 * 8400] * scale;
                    pt[2] = pt_data_ptr[0 + 2 * 8400] * scale;
                    pt[3] = pt_data_ptr[0 + 3 * 8400] * scale;

                } else if ((6399 < index) && ( index < 8000))
                {
                    int scale = 16;
                    pt[0] = pt_data_ptr[0 + 0 * 8400] * scale;
                    pt[1] = pt_data_ptr[0 + 1 * 8400] * scale;
                    pt[2] = pt_data_ptr[0 + 2 * 8400] * scale;
                    pt[3] = pt_data_ptr[0 + 3 * 8400] * scale;
                }
                else if ((8000 < index)&&( index < 8399))
                {   
                    int scale = 32;
                    pt[0] = pt_data_ptr[0 + 0 * 8400] * scale;
                    pt[1] = pt_data_ptr[0 + 1 * 8400] * scale;
                    pt[2] = pt_data_ptr[0 + 2 * 8400] * scale;
                    pt[3] = pt_data_ptr[0 + 3 * 8400] * scale;
                }

                pt[4] = score_vec[i];

                concat_result.push_back(pt);
            }

            return concat_result;
        }   
        
        std::vector<std::array<float, 5>> non_max_suppression(std::vector<std::array<float, 5>> pred, float conf_thres, float iou_thres)
        {
            // generate NMS data
            std::vector<cv::Rect2d> boxes;
            std::vector<float>      scores;
            for(auto &it : pred)
            {
                cv::Rect2d rect;
                rect.x = it[0] - it[2] / 2;
                rect.y = it[1] - it[3] / 2;
                rect.width = it[2];
                rect.height = it[3];
                boxes.push_back(rect);
                scores.push_back(it[4]);
            }

            // opencv dnn nms
            std::vector<int> indices;
            cv::dnn::NMSBoxes(boxes, scores, conf_thres, iou_thres, indices, 1.f, 1);

            // select indices 
            std::vector<std::array<float, 5>> bboxes;

            for(int i = 0; i < indices.size(); i++)
            {
                std::array<float, 5> box;

                box[0] = pred[indices[i]][0] - pred[indices[i]][2] / 2;
                box[1] = pred[indices[i]][1] - pred[indices[i]][3] / 2;

                box[2] = pred[indices[i]][0] + pred[indices[i]][2] / 2;
                box[3] = pred[indices[i]][1] + pred[indices[i]][3] / 2;

                box[4] = pred[indices[i]][4];

                bboxes.push_back(box);
            }

            return bboxes;
        }


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
           * @fun run_detect
           * @param image param_map
           * @return std::vector<smog::box_info_internal>
           * @details run detect (maybe in multithreading)
         */
        std::vector<smog::box_info_internal> run_detect(cv::Mat& image, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {   
            std::vector<smog::box_info_internal> detect_result;

            float smog_thres = param_map.count("smog_thres") ? param_map["smog_thres"] : 0.65f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.65f;

            // preprocess
            auto input_shape = cv::Size(640,  640);

            auto output_shape = cv::Size(image.cols, image.rows);
            
            cv::Mat blobs;
            float ratio = 0;
            std::tie (blobs, ratio) = letterbox(image, 640);

            cv::cvtColor(blobs, blobs, cv::COLOR_BGR2RGB);

            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forwards;

            auto  network_results = net_detect_.forward(blobs.data, { 1, blobs.rows, blobs.cols, blobs.channels() }, RKNN_TENSOR_NHWC);

            forwards.push_back(network_results["onnx::Mul_423"]);
            forwards.push_back(network_results["onnx::Sigmoid_380"]);

            int conf_thres = 0.25f;

            auto concat_result = concat(forwards, conf_thres, blobs);
            
            // select which bigger than smog_thres
            std::vector<std::array<float, 5>> select_result;
            for(auto & it: concat_result)
            {
                if(it[4] > smog_thres)
                {
                    select_result.push_back(it);
                }
            }
            
            if(select_result.size() == 0)
            {
                return detect_result;
            }

            // NMS
            auto nms_result = non_max_suppression(select_result, conf_thres, iou_thres);

            // scale_coords
            for(auto &it: nms_result)
            {   
                auto scale_coords = scale_coord(it, input_shape, output_shape);

                smog::box_info_internal box_info;

                box_info.x1 = scale_coords[0];
                box_info.y1 = scale_coords[1];
                box_info.x2 = scale_coords[2];
                box_info.y2 = scale_coords[3];
                box_info.category = 1;
                box_info.confidence = scale_coords[4];

                detect_result.push_back(box_info);
            }

            return detect_result;
        }



    private:
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)

		rknnwrapper::rknn_wrapper net_detect_;
#else
		std::unique_ptr<excalibur::pipeline<float>> net_detect_;
#endif
        std::string model_directory_;
        int device_ ;

    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;


    exposing::param_vector<smog::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }

    std::string detect_code_internal::version()
	{
		return impl_->version();
	}

}
