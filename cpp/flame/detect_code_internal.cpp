#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include <abi/param_vector.hpp>

#include <opencv2/opencv.hpp>

#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include <GenPipeline/GetPostprocessing.hpp>
#include "../genpipeline/market/yolov8_GEN.hpp"

namespace glasssix::flame
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
        {
            std::string model_dir = exposing::to_narrow_string(model_directory) + "/";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "flame_v8_cut.rknn", 0);
#elif defined(USE_BMNN)
			ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "flame_v8_cut.bmodel", 0);
#else
			ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "flame_v8_cut.onnx", 0);
#endif
            ioprocess_pipeline_->manual_possible_normalization(0, 1.f / 255);
            ioprocess_pipeline_->set_postprocessing(yolov8_GEN<2, 1>);
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
			const std::string algo_module_version = "4.0.0";

			std::string nn_frame_version = ioprocess_pipeline_->version();

			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    private:

        struct FlameBox :public GenPipTools::YoloBoxBase {
        public:
            using YoloBoxBase::YoloBoxBase; //Inheriting Constructors
        };

        void run_detect(std::vector<box_info_internal>& results, cv::Mat& image, std::map<std::string, float>& param_map)
        {
            float conf_threshold= param_map.count("conf_thres") ? param_map["conf_thres"] : 0.4f;
            float nms_threshold = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.5f;

            constexpr int infrW = 640;
            constexpr int infrH = 640;
            constexpr bool ifCvtRGB = true;

            GenPipTools::LetterInfo letter_op;
            auto letter_img = GenPipTools::letter_image(image, infrW, infrH, letter_op, ifCvtRGB);
            auto net_rstmap = ioprocess_pipeline_->forward(letter_img);
            auto tensor_out = net_rstmap.begin()->second;
            const int vf_nums = tensor_out->height(); //vf, visual field
            const int per_vf_len = tensor_out->width();
            std::vector<FlameBox> box_list;
            for (size_t idx = 0; idx < vf_nums; idx++) {
                float* pdata = tensor_out->mutable_cpu_data() + idx * per_vf_len;
                float conf_pos = pdata[4];
                float conf_neg = pdata[5];
                if (conf_pos > conf_threshold && conf_neg < 0.1) {
                    FlameBox obj_box(pdata[0] * infrW, pdata[1] * infrH, pdata[2] * infrW, pdata[3] * infrH, conf_pos, 0);
                    box_list.push_back(obj_box);
                }
            }

            GenPipTools::letter_map_origin_location(box_list, letter_op);
            GenPipTools::nms_cpu(box_list, 0.4);

            for (auto box : box_list) {
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
        std::shared_ptr<PrePostProcessGenPipeline> ioprocess_pipeline_;

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
