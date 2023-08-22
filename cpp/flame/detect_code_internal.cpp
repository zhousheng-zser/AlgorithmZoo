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

//YHC
// #include <opencv2/highgui/highgui.hpp>
#include "flame_function.hpp"

namespace glasssix::flame
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{get_model_params("flame", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
                :net_instance_(phai,  model_directory + std::string("/flame.rknn"), device)
        {
        }

        exposing::param_vector<flame::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
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
                  throw exposing::abi_invalid_argument("incorrect roi in flame");
            }
            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width));

			std::vector<box_info_internal> results;
			auto result = exposing::make_param_vector<flame::box_info>();

			run_detect(results, cropped_image, roi_x, roi_y, roi_width, roi_height, param_map);

			for (auto& i : results)
			{
				result.push_back(exposing::make_as_first<box_info_impl>(i));
			}
			return result;
        }

        std::string version()
        {
			const std::string algo_module_version = "2.0.1";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			//#if 0
			std::string nn_frame_version = net_instance_.version();
#else
			std::string nn_frame_version = net_instance_.version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);

        }

    private:

        /**
         * @fun preprocess
         * @param src, new_shape
         * @return tensor(preprocess(image))
         * @details image preprocess and make tensor from images
         */
        cv::Mat preprocess(cv::Mat img, int hope_w = 640, int hope_h = 640)
        {
            int H = img.rows;
            int W = img.cols;
            float ratio_w = (float)W / (float)hope_w;
            float ratio_h = (float)H / (float)hope_h;
            cv::Mat resize_img;
            if (ratio_w == ratio_h)
                cv::resize(img, resize_img, cv::Size2i{ hope_w, hope_h });
            else if (ratio_w > ratio_h) {
                int new_x = hope_w;
                int new_y = (int)(H / ratio_w);
                int pad1 = (int)((hope_h - new_y) / 2);
                int pad2 = hope_h - new_y - pad1;
                cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
            }
            else {
                int new_y = hope_h;
                int new_x = (int)(W / ratio_h);
                int pad1 = (int)((hope_w - new_x) / 2);
                int pad2 = hope_w - new_x - pad1;
                cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 127,127,127 });
            }
            return resize_img;
        }


        template<typename Dtype>
        std::vector<std::shared_ptr<memory::tensor<Dtype>>> sort_yolo_rst(const std::unordered_map<std::string, std::shared_ptr<memory::tensor<Dtype>>>& result) {
            std::vector<std::shared_ptr<memory::tensor<Dtype>>> outRst;
            for (auto& out : result) {
                outRst.push_back(out.second);
            }
            std::sort(outRst.begin(), outRst.end(), [](const std::shared_ptr<memory::tensor<float>>& A, const std::shared_ptr<memory::tensor<float>>& B) {
                auto countA = A->count();
                auto countB = B->count();
                return countA > countB;
                });
            return outRst;
        }


        /**
           * @fun run_detect
           * @param image param_map
           * @details run detect (maybe in multithreading)
        */
        void run_detect(std::vector<box_info_internal>& results, cv::Mat& image, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            float conf_threshold= param_map.count("conf_thres") ? param_map["conf_thres"] : 0.8f;
            float nms_threshold = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.5f;      
			
            float mapping_ratio = float(std::max(image.cols, image.rows)) / 640;

            cv::Mat blob = preprocess(image, 640, 640);
            cv::cvtColor(blob, blob, cv::COLOR_BGR2RGB);

            auto network_result = net_instance_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);
            auto rstSort = sort_yolo_rst(network_result);

            std::vector<Bbox> sub_bboxes = concat_yolo(rstSort, conf_threshold);

            int pad = std::abs(image.cols - image.rows)/2;
            bool is_vertical_pad = image.cols > image.rows;
            for (auto& bbox : sub_bboxes) {
                bbox.mul_ratio(mapping_ratio);
                if(is_vertical_pad){
                    bbox.ymin -= pad;
                    bbox.ymax -= pad;
                }
                else{
                    bbox.xmin -= pad;
                    bbox.xmax -= pad; 
                }
            }

            nms_cpu(sub_bboxes, nms_threshold);

            for (auto box : sub_bboxes) {
                box_info_internal in_box_info;
                in_box_info.x1 = box.xmin;
                in_box_info.y1 = box.ymin;
                in_box_info.x2 = box.xmax;
                in_box_info.y2 = box.ymax;
                in_box_info.score = box.score;
				in_box_info.category = 1;

                results.push_back(in_box_info);
            }
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

    std::string detect_code_internal::version()
    {
        return impl_->version();
    }

    exposing::param_vector<flame::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}
