#include "detect_code_internal.hpp"
#include "box_info_internal.hpp"
#include "box_info_impl.hpp"

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
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"

#include "../posture/detect_code.hpp"
#include "../head/detect_code.hpp"

namespace glasssix::vesthelmet
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
        {
            std::vector<std::string> phai;

            posture_instance_ = glasssix::exposing::make_exported_interface<posture::detect_code>(exposing::param_string(model_directory), device, 1);
            head_instance_ = glasssix::exposing::make_exported_interface<head::detect_code>(exposing::param_string(model_directory), device);
            vest_cls_instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(phai,std::string(model_directory) + "/" + "vesthelmet_vest_cls.rknn", device);
            helmet_cls_instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(phai,std::string(model_directory) + "/" + "vesthelmet_helmet_cls.rknn", device);
        }

        exposing::param_vector<vesthelmet::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, std::map<std::string, float>& param_map_std)
        {
            auto result = exposing::make_param_vector<vesthelmet::box_info>();
            std::vector<box_info_internal> output;

            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            cv::Mat image(cv::Size(width, height), CV_8UC3);

            std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * channels * height * width);

            auto temp_param_abi = exposing::make_param_hash_map<exposing::param_string, float>();
            //temp_param_abi.add_or_update("conf_thres", 0.1);
            exposing::param_vector<posture::box_info> posture_info_list_raw = posture_instance_.detect(bitmap, channels, height, width, 0, 0, width, height, temp_param_abi);

            for (auto pinfo : posture_info_list_raw)
            {
                PostureInfo postureInfo{ pinfo };

                if (postureInfo.if_vesthelmet_bodyerr()) {
                    continue; //body error
                }

                auto vest_cls_rect = postureInfo.get_vest_det_region();
                auto vest_cls_region = safty_cut(image, vest_cls_rect);
                vest_cls_region = letterbox(vest_cls_region, 128, 128);
                cv::cvtColor(vest_cls_region, vest_cls_region, cv::COLOR_BGR2RGB);

                auto vest_cls_rst_map = vest_cls_instance_->forward(vest_cls_region.data, { 1, vest_cls_region.rows, vest_cls_region.cols, vest_cls_region.channels() }, RKNN_TENSOR_NHWC);
                auto vest_cls_rst = vest_cls_rst_map.begin()->second;
                auto vest_cls_scores = vest_cls_rst->cpu_data();

				if (vest_cls_scores[0] > 0.2 || vest_cls_scores[1] < 0.8) continue; //no vest

                auto people_img_rect = postureInfo.get_rect();
                //people_img_rect.width *= 1.4;
                //people_img_rect.height *= 1.3;
                //people_img_rect.x -= people_img_rect.width * 0.2;
                //people_img_rect.y -= people_img_rect.width * 0.2;
                auto people_img = safty_cut(image, people_img_rect);
                auto people_start = people_img_rect.tl();

				exposing::param_span<std::uint8_t> people_img_image_span(const_cast<std::uint8_t*>(people_img.data), people_img.cols * people_img.rows * people_img.channels());
                exposing::param_vector<head::box_info> head_info_list = head_instance_.detect(people_img_image_span, 3, people_img.rows, people_img.cols, 0, 0, people_img.cols, people_img.rows, temp_param_abi);

                std::vector<headInfo> head_info;
                for (auto pinfo : head_info_list)
                {
                    headInfo postureInfo{ pinfo };
                    head_info.push_back(postureInfo);
                }

                for (auto& head : head_info)
                {
                    auto helmet_cls_region = safty_cut(people_img, head.get_rect());

                    auto helmet_cls = letterbox(helmet_cls_region, 96, 96);
					cv::cvtColor(helmet_cls, helmet_cls, cv::COLOR_BGR2RGB);

					auto helmet_cls_rst_map = helmet_cls_instance_->forward(helmet_cls.data, { 1, helmet_cls.rows, helmet_cls.cols, helmet_cls.channels() }, RKNN_TENSOR_NHWC);
                    auto helmet_cls_rst = helmet_cls_rst_map.begin()->second;
                    auto helmet_cls_scores = helmet_cls_rst->cpu_data();

                    ////YHC
                    //std::array<float, 3> helmet_cls_arr{ helmet_cls_scores[0], helmet_cls_scores[1],helmet_cls_scores[2] };
                    //dbg(helmet_cls_arr);

                    std::array<std::pair<float, int>, 3> socre_idx_list{
                        std::pair{helmet_cls_scores[0],0},
                        std::pair{helmet_cls_scores[1],1},
                        std::pair{helmet_cls_scores[2],2},
                    };
                    std::sort(socre_idx_list.begin(), socre_idx_list.end(), [](std::pair<float, int>& a, std::pair<float, int>& b) {
                        return a.first > b.first;
                        });

                    box_info_internal rst_uint;
                    //rst_uint.x1 = head.x1 + people_start.x;
                    //rst_uint.y1 = head.y1 + people_start.y;
                    //rst_uint.x2 = head.x2 + people_start.x;
                    //rst_uint.y2 = head.y2 + people_start.y;
                    rst_uint.x1 = people_img_rect.x;
                    rst_uint.y1 = people_img_rect.y;
					rst_uint.x2 = people_img_rect.x + people_img_rect.width;
					rst_uint.y2 = people_img_rect.y + people_img_rect.height;
                    rst_uint.score = socre_idx_list[0].first;
                    rst_uint.category = socre_idx_list[0].second;
                    output.push_back(rst_uint);

                }

            }

            for (auto& it : output)
            {
                result.push_back(glasssix::exposing::make_as_first<box_info_impl>(it));
            }

			return result;
        }

        std::string version()
        {
            const std::string algo_module_version = "1.0.0";
            std::string nn_frame_version = "rknn";
            return fmt::format(R"({ {"nn_frame_version":"{}", "algo_module_version" : "{}"} })", nn_frame_version, algo_module_version);
        }


        inline cv::Mat safty_cut(cv::Mat& img, cv::Rect roi)
        {
            int width = roi.width;
            int height = roi.height;
            int x = roi.x;
            int y = roi.y;

            cv::Mat mat(height, width, img.type(), cv::Scalar(0));
            int _x = x;
            int _y = y;
            int _width = width;
            int _height = height;
            if (x < 0)
            {
                _x = 0;
                _width = width + x;
            }

            if (_x + _width > img.cols)
                _width = img.cols - _x;

            if (y < 0)
            {
                _y = 0;
                _height = height + y;
            }

            if (_y + _height > img.rows)
                _height = img.rows - _y;

            img(cv::Rect(_x, _y, _width, _height)).copyTo(mat(cv::Rect(_x - x, _y - y, _width, _height)));
            return mat;
        }

        static inline cv::Mat letterbox(cv::Mat img, int hope_w = 640, int hope_h = 640)
        {
            int H = img.rows;
            int W = img.cols;
            float ratio_w = (float)W / (float)hope_w;
            float ratio_h = (float)H / (float)hope_h;
            cv::Mat resize_img;
            if (ratio_w == ratio_h)
            {
                cv::resize(img, resize_img, cv::Size2i{ hope_w, hope_h });
            }
            else if (ratio_w > ratio_h)
            {
                int new_x = hope_w;
                int new_y = (int)(H / ratio_w);
                int pad1 = (int)((hope_h - new_y) / 2);
                int pad2 = hope_h - new_y - pad1;
                cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
            }
            else
            {
                int new_y = hope_h;
                int new_x = (int)(W / ratio_h);
                int pad1 = (int)((hope_w - new_x) / 2);
                int pad2 = hope_w - new_x - pad1;
                cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
            }
            return resize_img;
        }

    private:
        posture::detect_code posture_instance_;
        head::detect_code head_instance_;
        std::unique_ptr<rknnwrapper::rknn_wrapper> vest_cls_instance_;
        std::unique_ptr<rknnwrapper::rknn_wrapper> helmet_cls_instance_;

    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal()
    {
    }

    exposing::param_vector<vesthelmet::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, std::map<std::string, float>& param_map_std)
    {
        return impl_->detect(bitmap, channels, height, width, param_map_std);
    }

    std::string detect_code_internal::version()
    {
        return impl_->version();
    }
}
