#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>

#include "../head/detect_code.hpp"
#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <tuple>

#include "Excalibur/pipeline.hpp"
#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_safty_cut.hpp"
#include "Primitives/tensor_conversions.hpp"
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#endif
#include <iomanip>
#include <tuple>

namespace glasssix::helmet
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
            : impl{ get_model_params("helmet", false),  exposing::to_narrow_string(model_directory), device }
        {
        }

        impl(const std::vector<std::string>& phai, std::string model_directory, int device)
            :net_class_(phai, model_directory + std::string("/helmet_sim.rknn"), device)
        {
        }

        exposing::param_vector<helmet::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, 
            int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<head::box_info> head_info_list_raw, std::map<std::string, float>& param_map)
        {

            //float MIN_HEAD = param_map.count("min_size") ? param_map["min_size"] : 24.f;
            //float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.5f;
            //float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;

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
                throw exposing::abi_invalid_argument("incorrect roi in helmet");
            }
            std::vector<headInfo> head_info;
            for (auto hinfo : head_info_list_raw) {
                headInfo temp;
                temp.x1 = hinfo.x1();
                temp.x2 = hinfo.x2();
                temp.y1 = hinfo.y1();
                temp.y2 = hinfo.y2();
                temp.score = hinfo.score();
                head_info.push_back(temp);
            }

            std::vector<helmet::box_info_internal> result = helmet_detect(bitmap, height, width, roi_x, roi_y, roi_width, roi_height, head_info, param_map);

            auto results = exposing::make_param_vector<helmet::box_info>();

            for (auto& it : result)
            {
                it.x1 += roi_x;
                it.x2 += roi_x;
                it.y1 += roi_y;
                it.y2 += roi_y;
                results.push_back(glasssix::exposing::make_as_first<box_info_impl>(it));
            }


            return results;
        }

        std::string version()
        {
            const std::string algo_module_version = "2.2.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            //#if 0
            std::string nn_frame_version = net_class_.version();
#else
            std::string nn_frame_version = net_class_.version();
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
        struct detect_list
        {
            int x1;
            int y1;
            int x2;
            int y2;
            int category;
        };



        void  Softmax(float* data, int num)
        {

            double L2_Sum = 0.f;
            for (size_t i = 0; i < num; i++)
            {
                data[i] = (exp(data[i]));
                L2_Sum += data[i];
            }
            for (size_t i = 0; i < num; i++)
            {
                data[i] = data[i] / L2_Sum;
            }
        }


        cv::Mat preprocess_detection(cv::Mat& src, cv::Size input_shape = cv::Size(96, 96))
        {
            float scale = std::min((float)input_shape.width / (float)src.cols, (float)input_shape.height / (float)src.rows);
            cv::Mat cut_image;
            cv::Mat mask_image(input_shape, CV_8UC3, cv::Scalar(114, 114, 114));
            if (src.rows != input_shape.height || src.cols != input_shape.width)
            {
                cv::resize(src, cut_image, cv::Size((int)(src.cols * scale), (int)(src.rows * scale)), cv::INTER_LINEAR);

                auto pad_h = int((input_shape.height - cut_image.rows) / 2);
                auto pad_w = int((input_shape.width - cut_image.cols) / 2);
                cv::copyMakeBorder(cut_image, mask_image, pad_h, input_shape.height - cut_image.rows - pad_h, pad_w, input_shape.width - cut_image.cols - pad_w, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
            }
            else
            {
                src.copyTo(mask_image);
            }
            cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);
            return mask_image;
        }


        std::vector<helmet::box_info_internal> helmet_detect(const exposing::param_span<std::uint8_t>& bitmap, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height,
            const std::vector<headInfo> &head_info ,std::map<std::string, float>& param_map)
        {
            std::vector<box_info_internal> output;

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * 3 * height * width);

            for (auto& head : head_info)
            {

                cv::Mat crop = image(cv::Range(head.y1, head.y2), cv::Range(head.x1, head.x2));
                if (crop.cols < 24 || crop.rows < 24)
                {
                    continue;
                }

                // cv::Mat headimg;
                crop = hisEqulColor(crop);

                auto headimg = preprocess_detection(crop);

                auto  network_result = net_class_.forward(headimg.data, { 1, headimg.rows, headimg.cols,headimg.channels() }, RKNN_TENSOR_NHWC);

                float* helmet_conf = network_result["output0"]->mutable_cpu_data();

                Softmax(helmet_conf, 3);

                box_info_internal  headp(head.x1, head.x2, head.y1, head.y2);
                if (helmet_conf[0] > helmet_conf[1] && helmet_conf[0] > helmet_conf[2])
                {
                    headp.category = 2;
                    headp.score = helmet_conf[0];
                    output.push_back(headp);
                }
                else if (helmet_conf[1] > helmet_conf[0] && helmet_conf[1] > helmet_conf[2])
                {
                    headp.category = 0;
                    headp.score = helmet_conf[1];
                    output.push_back(headp);
                }
            }
            return output;
        }

        cv::Mat hisEqulColor(const cv::Mat& img)
        {
            cv::Mat ycrcb;
            cv::cvtColor(img, ycrcb, cv::COLOR_BGR2YCrCb);
            std::vector<cv::Mat> channels;
            cv::split(ycrcb, channels);

            cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
            clahe->setClipLimit(2.0);
            clahe->setTilesGridSize(cv::Size(8, 8));
            clahe->apply(channels[0], channels[0]);
            cv::merge(channels, ycrcb);
            cv::cvtColor(ycrcb, img, cv::COLOR_YCrCb2BGR);

            return img;
        }

    private:
        std::string model_directory_;
        int device_;
        // glasssix::rknnwrapper::rknn_wrapper net_detect_;
        glasssix::rknnwrapper::rknn_wrapper net_class_;
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

    exposing::param_vector<helmet::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, 
        int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<head::box_info> head_info_list, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, head_info_list, param_map);
    }
}
