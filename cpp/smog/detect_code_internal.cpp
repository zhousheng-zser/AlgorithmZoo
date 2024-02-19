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
                :net_detect_(phai,  model_directory + std::string("/smog_sim.rknn"), device)
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
        const std::string algo_module_version = "2.0.3";

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
        std::vector<NMSBox> concat(std::vector<std::shared_ptr<glasssix::memory::tensor<float>>>& prediction, float conf_thres, cv::Mat& blobs)
        {
            std::vector<NMSBox> bboxes_result;

            const float *score_data_ptr = prediction[1]->cpu_data(); // ["onnx::Sigmoid_380"] = 1*1*8400*1

            const float * pt_data_ptr = prediction[0]->cpu_data();

            for(int index = 0; index < 8400; index++)
            {
                float cx = 0;
                float cy = 0;
                float w = 0;
                float h = 0;

                float score = sigmoid(score_data_ptr[index]);
                if (score > conf_thres) 
                {
					if (index < 6400)
					{
                        constexpr int scale = 8;
                        cx = pt_data_ptr[index + 0 * 8400] * scale;
						cy = pt_data_ptr[index + 1 * 8400] * scale;
						w = pt_data_ptr[index + 2 * 8400] * scale;
						h = pt_data_ptr[index + 3 * 8400] * scale;

					}
					else if ((6399 < index) && (index < 8000))
					{
						constexpr int scale = 16;
                        cx = pt_data_ptr[index + 0 * 8400] * scale;
						cy = pt_data_ptr[index + 1 * 8400] * scale;
						w = pt_data_ptr[index + 2 * 8400] * scale;
						h = pt_data_ptr[index + 3 * 8400] * scale;
					}
					else if ((8000 < index) && (index < 8399))
					{
                        constexpr int scale = 32;
                        cx = pt_data_ptr[index + 0 * 8400] * scale;
                        cy = pt_data_ptr[index + 1 * 8400] * scale;
						w = pt_data_ptr[index + 2 * 8400] * scale;
						h = pt_data_ptr[index + 3 * 8400] * scale;
					}

                    NMSBox _box{ cx,cy,w,h,score };
					bboxes_result.push_back(_box);
                }
            }

            return bboxes_result;
        }   
        

        void nms_cpu(std::vector<NMSBox>& bboxes, float iou_thres) {
            if (bboxes.empty()) return;
            std::sort(bboxes.begin(), bboxes.end(), [&](NMSBox b1, NMSBox b2) {return b1.score > b2.score; });
            std::vector<float> area(bboxes.size());
            for (int i = 0; i < bboxes.size(); ++i) {
                area[i] = (bboxes[i].xmax - bboxes[i].xmin + 1) * (bboxes[i].ymax - bboxes[i].ymin + 1);
            }
            for (int i = 0; i < bboxes.size(); ++i) {
                for (int j = i + 1; j < bboxes.size(); ) {
                    float left = std::max(bboxes[i].xmin, bboxes[j].xmin);
                    float right = std::min(bboxes[i].xmax, bboxes[j].xmax);
                    float top = std::max(bboxes[i].ymin, bboxes[j].ymin);
                    float bottom = std::min(bboxes[i].ymax, bboxes[j].ymax);
                    float width = std::max(right - left + 1, 0.f);
                    float height = std::max(bottom - top + 1, 0.f);
                    float u_area = height * width;
                    float iou = (u_area) / (area[i] + area[j] - u_area);
                    if (iou >= iou_thres) {
                        bboxes.erase(bboxes.begin() + j);
                        area.erase(area.begin() + j);
                    }
                    else {
                        ++j;
                    }
                }
            }
            if (bboxes.size() < 2) return;
            std::sort(bboxes.begin(), bboxes.end(), [&](NMSBox b1, NMSBox b2) {return b1.score > b2.score; });
        }

        void scale_coord(NMSBox& bbox, int reShapeSide, cv::Size origin_shape)
        {
            int pad = std::abs(origin_shape.width- origin_shape.height) / 2;
            bool is_vertical_pad = origin_shape.width > origin_shape.height;
            float mapping_ratio = static_cast<float>(std::max(origin_shape.width , origin_shape.height)) / reShapeSide;

			bbox.mul_ratio(mapping_ratio);
			if (is_vertical_pad) {
				bbox.ymin -= pad;
				bbox.ymax -= pad;
			}
			else {
				bbox.xmin -= pad;
				bbox.xmax -= pad;
			}

            auto clamp = [](int x, int min, int max) {if (x < min) return min; else if (x > max) return max; else return x; };
			bbox.xmin = clamp(bbox.xmin, 0, origin_shape.width - 1);
			bbox.xmax = clamp(bbox.xmax, 0, origin_shape.width - 1);
			bbox.ymin = clamp(bbox.ymin, 0, origin_shape.height - 1);
			bbox.ymax = clamp(bbox.ymax, 0, origin_shape.height - 1);
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

            float conf_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.65f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.65f;

            // preprocess
            constexpr int reShapeSide = 640;
            
            cv::Mat blobs;
            float ratio = 0;
            std::tie (blobs, ratio) = letterbox(image, reShapeSide);

            cv::cvtColor(blobs, blobs, cv::COLOR_BGR2RGB);

            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forwards;

            auto  network_results = net_detect_.forward(blobs.data, { 1, blobs.rows, blobs.cols, blobs.channels() }, RKNN_TENSOR_NHWC);

            forwards.push_back(network_results["onnx::Mul_423"]);
            forwards.push_back(network_results["onnx::Sigmoid_380"]);

            auto bboxes_list = concat(forwards, conf_thres, blobs);

            // NMS
            nms_cpu(bboxes_list, iou_thres);

            for(auto &bbox : bboxes_list)
            {   
				scale_coord(bbox, reShapeSide, cv::Size(image.cols, image.rows));
                if (bbox.xmin >= bbox.xmax) continue;
                if (bbox.ymin >= bbox.ymax) continue;

                smog::box_info_internal box_info;

                box_info.x1 = bbox.xmin;
                box_info.y1 = bbox.ymin;
                box_info.x2 = bbox.xmax;
                box_info.y2 = bbox.ymax;
                box_info.category = 1;
                box_info.confidence = bbox.score;

                detect_result.push_back(box_info);
            }

            return detect_result;
        }



    private:
        rknnwrapper::rknn_wrapper net_detect_;
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
