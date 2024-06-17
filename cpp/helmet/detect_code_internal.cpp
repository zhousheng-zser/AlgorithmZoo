#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"
#include <abi/param_vector.hpp>
#include <utility>

#include "../head/detect_code.hpp"
#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <tuple>

#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include <GenPipeline/GenPipeTools.hpp>

namespace glasssix::helmet
{
    class detect_code_internal::impl
    {
    public:

        impl() {}
        impl(const exposing::param_string model_directory, int device) :impl()
        {
            std::string model_dir = exposing::to_narrow_string(model_directory);
            if (*model_dir.rbegin() != '/') model_dir += '/';
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            net_class_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "helmet_sim.rknn", 0);
#elif defined(USE_BMNN)
            net_class_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "helmet_sim.bmodel", 0);
#else
            net_class_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "helmet_sim.onnx", 0);
#endif
            net_class_->manual_possible_normalization(0, 1.f / 255);
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
#if defined(USE_BMNN)
                head_info.push_back(temp);
#else
                //按照最新安全帽的协议:周杨瑞算法工程师,长宽需扩大1.2倍
                float multiple = 0.2;
                headInfo temp_new;
                std::int32_t w = temp.x2 - temp.x1,h = temp.y2 - temp.y1;//人头的数据:宽/高
                temp_new.x1 = std::max(static_cast<float>(roi_x)     ,temp.x1 - w * multiple / 2 );
                temp_new.x2 = std::min(static_cast<float>(roi_width) ,temp.x2 + w * multiple / 2 );
                temp_new.y1 = std::max(static_cast<float>(roi_y)     ,temp.y1 - h * multiple / 2 );
                temp_new.y2 = std::min(static_cast<float>(roi_height),temp.y2 + h * multiple / 2 );
	            // printf("debug_zj--line=%d::%d,%d,%d,%d,%d,%d\n",__LINE__,temp.x1,temp.x2,temp.y1,temp.y2,w,h);
	            // printf("debug_zj__line=%d::%d,%d,%d,%d,%d,%d\n",__LINE__,temp_new.x1,temp_new.x2,temp_new.y1,temp_new.y2,temp_new.x2 - temp_new.x1,temp_new.y2 - temp_new.y1);
                head_info.push_back(temp_new);
#endif
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
            const std::string algo_module_version = "2.2.1";
            std::string nn_frame_version = net_class_->version();
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

#if defined(USE_BMNN)
        cv::Mat preprocess_detection(cv::Mat& src, cv::Size input_shape = cv::Size(96, 96))
#else
        cv::Mat preprocess_detection(cv::Mat& src, cv::Size input_shape = cv::Size(80, 80))
#endif
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

            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
            for (auto& head : head_info)
            {
                cv::Rect headrect(head.x1, head.y1, head.x2 - head.x1, head.y2 - head.y1);
                cv::Mat crop = GenPipTools::safty_cut(image, headrect);

                if (crop.cols < 24 || crop.rows < 24)
                    continue;

                // cv::Mat headimg;
                crop = hisEqulColor(crop);

                auto headimg = preprocess_detection(crop);

                auto  tensor_out = net_class_->forward(headimg).begin()->second;//net has only single node out

                float* helmet_conf = tensor_out->mutable_cpu_data();
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
                Softmax(helmet_conf, 3);
#endif
                box_info_internal  headp(head.x1, head.x2, head.y1, head.y2);
                if (helmet_conf[0] > helmet_conf[1] && helmet_conf[0] > helmet_conf[2])
                {
                    headp.category = 2;
                    headp.score = helmet_conf[0];//人头
                    output.push_back(headp);
                }
                else if (helmet_conf[1] > helmet_conf[0] && helmet_conf[1] > helmet_conf[2])
                {
                    headp.category = 0;
                    headp.score = helmet_conf[1];//安全帽
                    output.push_back(headp);
                }
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
                else if (helmet_conf[2] > helmet_conf[0] && helmet_conf[2] > helmet_conf[1])
                {
                    headp.category = 1;
                    headp.score = helmet_conf[2];//非人头
                    output.push_back(headp);
                }
#endif
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
        std::shared_ptr<PrePostProcessGenPipeline> net_class_;
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
