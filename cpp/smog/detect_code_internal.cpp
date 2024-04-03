#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"
#include <abi/param_vector.hpp>
#include <utility>
#include <unordered_map>

#include <opencv2/opencv.hpp>

#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include <GenPipeline/GetPostprocessing.hpp>
#include "../genpipeline/market/yolov8_GEN.hpp"

namespace glasssix::smog
{
    class detect_code_internal::impl
    {
    public:
        impl() {}
        impl(std::string_view model_directory, int device) :impl()
        {
            std::string model_dir = exposing::to_narrow_string(model_directory);
            if (*model_dir.rbegin() != '/') model_dir += '/';
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "smog.rknn", 0);
#elif defined(USE_BMNN)
            ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "smog.bmodel", 0);
#else
            ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "smog.onnx", 0);
#endif
            ioprocess_pipeline_->manual_possible_normalization(0, 1.f / 255);
            ioprocess_pipeline_->set_postprocessing(yolov8_GEN<1, 1>);
        }

        exposing::param_vector<smog::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty()) {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            if (roi_x < 0 || roi_x > width || roi_y > height || roi_y < 0 || roi_height < 0 || (roi_height + roi_y) > height || roi_width < 0 || (roi_width + roi_x) > width)
            {
                throw exposing::abi_invalid_argument("incorrect roi in universal_pedestrian");
            }
            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));

            auto smog_list = run_detect(cropped_image, param_map);

            auto results = exposing::make_param_vector<smog::box_info>();
            for (auto& smog : smog_list)
            {
                box_info_internal smog_internal;
                smog.add(roi_x, roi_y);
                smog_internal.x1 = smog.xmin;
                smog_internal.y1 = smog.ymin;
                smog_internal.x2 = smog.xmax;
                smog_internal.y2 = smog.ymax;
                smog_internal.confidence = smog.score;
                smog_internal.category = 1;
                results.push_back(exposing::make_as_first<box_info_impl>(smog_internal));
            }
            return results;
        }

        struct SmogBox :public GenPipTools::YoloBoxBase {
        public:
            using YoloBoxBase::YoloBoxBase; //Inheriting Constructors
        };

        std::vector<SmogBox> run_detect(cv::Mat& image, std::map<std::string, float>& param_map) {
            float conf_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.65f;
            float nms_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.65f;
            constexpr int infrW = 640;
            constexpr int infrH = 640;
            constexpr bool ifCvtRGB = true;
            GenPipTools::LetterInfo letter_op;
            auto letter_img = GenPipTools::letter_image(image, infrW, infrH, letter_op, ifCvtRGB);

            std::vector<SmogBox> box_list;
            auto net_rstmap = ioprocess_pipeline_->forward(letter_img);
            auto tensor_out = net_rstmap.begin()->second;
            const int vf_nums = tensor_out->height(); //vf, visual field
            const int per_vf_len = tensor_out->width();
            for (size_t idx = 0; idx < vf_nums; idx++) {
                float* pdata = tensor_out->mutable_cpu_data() + idx * per_vf_len;
                float conf_pos = pdata[4];
                if (conf_pos > conf_thres) {
                    SmogBox obj_box(pdata[0] * infrW, pdata[1] * infrH, pdata[2] * infrW, pdata[3] * infrH, conf_pos, 1);
                    box_list.push_back(obj_box);
                }
            }
            GenPipTools::nms_cpu(box_list, nms_thres);
            GenPipTools::letter_map_origin_location(box_list, letter_op);
            return box_list;
        }
       
		std::string version()
		{
			const std::string algo_module_version = "3.0.0";
			std::string nn_frame_version = ioprocess_pipeline_->version();
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
		}

    private:
        std::shared_ptr<PrePostProcessGenPipeline> ioprocess_pipeline_;
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
