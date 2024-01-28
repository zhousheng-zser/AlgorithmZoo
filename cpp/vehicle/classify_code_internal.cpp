#include "classify_code_internal.hpp"
#include "box_info_internal.hpp"

#include <algorithm>
#include <numeric>

#include <Excalibur/pipeline.hpp>
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include "Primitives/tensor_conversions.hpp"
#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_resize.hpp"
#include "Excalibur/operation_rgb2gray.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "box_info_impl.hpp"
#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

#include "GenPipline.hpp"
#include "postprocessing_functions.hpp"

//#include "dbg.h"


namespace glasssix::vehicle
{
    class classify_code_internal::impl
    {
    public:
		impl(std::string_view model_directory, int device) :pipline(exposing::to_narrow_string(model_directory) + "/" + "vehicle" + ".rknn", 0)
        {
        }

        exposing::param_vector<vehicle::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string,float>& param_map_std)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * channels * height * width);

            if (roi_x<0 || roi_x>width || roi_y > height || roi_y < 0 || roi_height<0 || (roi_height + roi_y) >height || roi_width<0 || (roi_width + roi_x) > width)
            {
                throw exposing::abi_invalid_argument("incorrect roi in vehicle");
            }
            cv::Point roi_start(roi_x, roi_y);
            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width)).clone();

            std::vector<box_info_internal> results;
            auto result = exposing::make_param_vector<vehicle::box_info>();

            run_detect(results, cropped_image, roi_start, param_map_std);

            for (auto& i : results)
            {
                result.push_back(exposing::make_as_first<box_info_impl>(i));
            }
            return result;
        }

        void run_detect(std::vector<box_info_internal>& results, cv::Mat& image, cv::Point& roi_start, std::map<std::string, float>& param_map) {
            float conf_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.4f;
            float nms_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.7f;

            int reShapeSide = 640;
            auto letter_img = letter_image(image, reShapeSide, reShapeSide);
            cv::cvtColor(letter_img, letter_img, cv::COLOR_BGR2RGB);

            auto rst_map = pipline.forward(letter_img);
            auto rst_map_sorted = postprocessing::sort_yolo_rst(rst_map);

            auto tensor_out = postprocessing::yolov8_concat(rst_map_sorted);

            std::vector<ObjBox> vehicle_list;
            int targetnum = tensor_out->height();
            int infonum = tensor_out->width();
            for (size_t idx = 0; idx < targetnum; idx++) {
                float* pdata = tensor_out->mutable_cpu_data() + idx * infonum;
                float conf = pdata[4];

                if (conf > conf_thres)
                {
                    //ObjBox phonebox(pdata[0], pdata[1], pdata[2], pdata[3], conf); // no-cut model
                    ObjBox vehiclebox(pdata[0] * reShapeSide, pdata[1] * reShapeSide, pdata[2] * reShapeSide, pdata[3] * reShapeSide, conf);
                    vehicle_list.push_back(vehiclebox);
                }
            }

            int pad = std::abs(image.cols - image.rows) / 2;
            bool is_vertical_pad = image.cols > image.rows;
            float mapping_ratio = static_cast<float>(std::max(image.cols, image.rows)) / reShapeSide;

            for (auto& bbox : vehicle_list) {
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

            NMS_CPU(vehicle_list, nms_thres);

            for (auto box : vehicle_list) {
                box_info_internal in_box_info;
				in_box_info.x1 = box.xmin + roi_start.x;
                in_box_info.y1 = box.ymin + roi_start.y;
                in_box_info.x2 = box.xmax + roi_start.x;
                in_box_info.y2 = box.ymax + roi_start.y;
                in_box_info.score = box.score;
                in_box_info.category = 0;

                results.push_back(in_box_info);
            }
        }


        static inline cv::Mat letter_image(cv::Mat img, int hope_w, int hope_h)
        {
            int H = img.rows;
            int W = img.cols;

            if (H == hope_h && W == hope_w) return img;

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
                cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
            }
            return resize_img;
        }

        void NMS_CPU(std::vector<ObjBox>& bboxes, float iou_thres) {
            if (bboxes.empty()) return;
            std::sort(bboxes.begin(), bboxes.end(), [&](ObjBox b1, ObjBox b2) {return b1.score > b2.score; });
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
            std::sort(bboxes.begin(), bboxes.end(), [&](ObjBox b1, ObjBox b2) {return b1.score > b2.score; });
        }

        std::string version()
        {
            const std::string algo_module_version = "1.0.0";

            std::string nn_frame_version = pipline.version();

            return fmt::format(R"({ {"nn_frame_version":"{}", "algo_module_version" : "{}"} })", nn_frame_version, algo_module_version);
        }

    private:
        GenPipline pipline;
    };

    classify_code_internal::classify_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    classify_code_internal::~classify_code_internal()
    {
    }

    exposing::param_vector<vehicle::box_info> classify_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string,float>& param_map_std)
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map_std);
    }

    std::string classify_code_internal::version()
    {
        return impl_->version();
    }
}
