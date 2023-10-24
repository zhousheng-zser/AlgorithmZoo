#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>

#include <Primitives/tensor_conversions.hpp>

#ifdef USE_RKNNAPI
//#if 0
#include "../../common/include/RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#endif
#include "RknnYolov8Wrapper.hpp"

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>



namespace glasssix::flame
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
        {
            detect_instance_ = std::make_unique<RknnYolov8Wrapper>(exposing::to_narrow_string(model_directory) + "/" + "flame_v8_cut" + ".rknn", device);
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
            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width)).clone();

			std::vector<box_info_internal> results;
			auto result = exposing::make_param_vector<flame::box_info>();

			run_detect(results, cropped_image, param_map);

			for (auto& i : results)
			{
				result.push_back(exposing::make_as_first<box_info_impl>(i));
			}
			return result;
        }

        std::string version()
        {
			const std::string algo_module_version = "3.0.0";

			std::string nn_frame_version = detect_instance_->version();

			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    private:

        /**
         * @fun imgPreProcess
         * @param src, new_shape
         * @return tensor(preprocess(image))
         * @details image preprocess and make tensor from images
         */
        cv::Mat imgPreProcess(cv::Mat img, int hope_w = 640, int hope_h = 640)
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

        /**
           * @fun run_detect
           * @param image param_map
           * @details run detect (maybe in multithreading)
        */
        void run_detect(std::vector<box_info_internal>& results, cv::Mat& image, std::map<std::string, float>& param_map)
        {
            //float conf_threshold= param_map.count("conf_thres") ? param_map["conf_thres"] : 0.7f;
            //float nms_threshold = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.5f;
            float conf_threshold= 0.4f;
            float nms_threshold = 0.5f;

            int reShapeSide = 640;
            auto letter_img = imgPreProcess(image, reShapeSide, reShapeSide);
            cv::cvtColor(letter_img, letter_img, cv::COLOR_BGR2RGB);

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            auto det_rst_map = detect_instance_->forward(letter_img);
            auto tensor_out = det_rst_map.begin()->second;
#else
            auto input_tensor = matConverTensor(letter_img);
            // normalization
            auto data = input_tensor->mutable_cpu_data();
            for (int i = 0; i < input_tensor->count(); i++) {
                data[i] = data[i] / 255;
            }
            auto det_rst_map = detect_instance_->forward(input_tensor);
            auto tensor_out = det_rst_map.begin()->second;
#endif
            // transepose
            if (tensor_out->width() == 8400) {
                printf("tensor_transpose_0132\n");
                tensor_out = tensor_transpose_0132(tensor_out); // interagte
            }

            //dbg(tensor_out->data_shape());
            std::vector<FlameBox> flame_list;

            int targetnum = tensor_out->height();
            int infonum = tensor_out->width();
            for (size_t idx = 0; idx < targetnum; idx++) {
                float* pdata = tensor_out->mutable_cpu_data() + idx * infonum;
                float conf = pdata[4];
                float flame_tag= pdata[5];

				if (conf > conf_threshold && flame_tag < 0.1) {
                    //dbg(conf);
                    //std::cout << "pdata m640: " << pdata[0] * 640 << " " << pdata[1] * 640 << " " << pdata[1] * 640 << " " << pdata[1] * 640 << std::endl;

                    FlameBox flamebox(pdata[0] * 640, pdata[1] * 640, pdata[2] * 640, pdata[3] * 640, conf);
                    flame_list.push_back(flamebox);
                }
            }

            int pad = std::abs(image.cols - image.rows) / 2;
            bool is_vertical_pad = image.cols > image.rows;
            float mapping_ratio = static_cast<float>(std::max(image.cols, image.rows)) / reShapeSide;

            for (auto& bbox : flame_list) {
                bbox.mul_ratio(mapping_ratio);
                if (is_vertical_pad) {
                    bbox.ymin -= pad;
                    bbox.ymax -= pad;
                }
                else {
                    bbox.xmin -= pad;
                    bbox.xmax -= pad;
                }
            }

            nms_cpu(flame_list, nms_threshold);

            for (auto box : flame_list) {
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

        std::shared_ptr<glasssix::memory::tensor<float>> matConverTensor(cv::Mat input_image) {
            std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, input_image.rows, input_image.cols, 3}, -1, glasssix::memory::NHWC));
            std::copy(input_image.data, input_image.data + input_image.step[0] * input_image.rows, input_tensor_u8->mutable_cpu_data());
            input_tensor_u8->convert_order();
            auto input_tensor = input_tensor_u8 | glasssix::memory::tensor_convert_to<float>;
            return input_tensor;
        }

        void nms_cpu(std::vector<FlameBox>& bboxes, float iou_thres) {
            if (bboxes.empty()) return;
            std::sort(bboxes.begin(), bboxes.end(), [&](FlameBox b1, FlameBox b2) {return b1.score > b2.score; });
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
            std::sort(bboxes.begin(), bboxes.end(), [&](FlameBox b1, FlameBox b2) {return b1.score > b2.score; });
        }

    private:
        std::string model_directory_;
        int device_;

        std::unique_ptr<RknnYolov8Wrapper> detect_instance_;

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
