#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include <GenPipeline/GetPostprocessing.hpp>
#include "../genpipeline/market/yolov8_GEN.hpp"

#include <opencv2/opencv.hpp>
#include <abi/param_vector.hpp>
#include <utility>
//#include "dbg.h"

namespace glasssix::tumble
{
    class detect_code_internal::impl
    {
    public:
        impl() {}

        impl(const exposing::param_string model_directory, int device = -1) :impl()
        {
            std::string model_dir = exposing::to_narrow_string(model_directory);
            if (*model_dir.rbegin() != '/') model_dir += '/';
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            iopipeline_det_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "tumble_sim.rknn", 0);
            iopipeline_cls_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "tumble_cls.rknn", 0);
#elif defined(USE_BMNN)
            iopipeline_det_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "tumble_sim.bmodel", 0);
            iopipeline_cls_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "tumble_cls.bmodel", 0);
#else
            iopipeline_det_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "tumble_sim.onnx", 0);
            iopipeline_cls_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "tumble_cls.onnx", 0);
#endif
            iopipeline_cls_->manual_possible_normalization(0, 1.f / 255);

            iopipeline_det_->manual_possible_normalization(0, 1.f / 255);
            iopipeline_det_->set_postprocessing(yolov8_GEN<2, 1>);
        }

        exposing::param_vector<tumble::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
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

            auto results_box_info = exposing::make_param_vector<tumble::box_info>();

            std::vector<TumbleBBox> tumble_list = run_detect(cropped_image, param_map);

            for (auto& tman : tumble_list)
            {
                cv::Rect cls_region = tman.get_rect();
                if (cls_region.area() < 4 || cls_region.width < 1 || cls_region.height < 1) {
                    continue;
                }
                auto cls_region_img = GenPipTools::safty_cut(cropped_image, cls_region);
                auto cls_letter_img = GenPipTools::letter_image(cls_region_img, 256, 256, true);

                auto tensor_out = iopipeline_cls_->forward(cls_letter_img).begin()->second;
                auto tensor_out_data = tensor_out->mutable_cpu_data();

                struct FallCls {
					enum class Tag { Fall, HardFall };
                    float score;
                    Tag tag;
                    FallCls(float* cls_data) :score(std::max(cls_data[0], cls_data[1])) {
                        tag = cls_data[0] > cls_data[1]? Tag::Fall: Tag::HardFall;
                    }
                };
                FallCls fall_cls(tensor_out_data);
                //printf("cls %f,  %f,  %f\n", tensor_out_data[0], tensor_out_data[1], tensor_out_data[2]);

                //dbg(tensor_out->data_shape());
                //dbg(tensor_out->mutable_cpu_data()[0]);
                //dbg(tensor_out->mutable_cpu_data()[1]);
                //dbg(tensor_out->mutable_cpu_data()[2]);

                if (fall_cls.score > 0.8) {
					box_info_internal box_info;
					tman.add(roi_x, roi_y);
					if (!GenPipTools::constraintRectBoundary(tman, height, width)) {
						continue;
					}
					box_info.x1 = tman.xmin;
					box_info.x2 = tman.xmax;
					box_info.y1 = tman.ymin;
					box_info.y2 = tman.ymax;
					box_info.score = fall_cls.score;
					box_info.category = tman.cid;
					results_box_info.push_back(exposing::make_as_first<box_info_impl>(box_info));
                }
                else {
                    box_info_internal box_info;
                    tman.add(roi_x, roi_y);
                    if (!GenPipTools::constraintRectBoundary(tman, height, width)) {
                        continue;
                    }
                    box_info.x1 = tman.xmin;
                    box_info.x2 = tman.xmax;
                    box_info.y1 = tman.ymin;
                    box_info.y2 = tman.ymax;
                    box_info.score = fall_cls.score;
                    box_info.category = 0;
                    results_box_info.push_back(exposing::make_as_first<box_info_impl>(box_info));
                }
            }    

            return results_box_info;
        }

        struct TumbleBBox :public GenPipTools::YoloBoxBase {
        public:
            using YoloBoxBase::YoloBoxBase; //Inheriting Constructors
        };

        std::vector<TumbleBBox> run_detect(cv::Mat& image, std::map<std::string, float>& param_map) {
            float conf_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.6f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.65f;
            const int letter_h = 1280;
            const int letter_w = 1280;
            constexpr int NO_TUMBLE = 0;
            constexpr int IS_TUMBLE = 1;

            std::vector<TumbleBBox> box_list;

            GenPipTools::LetterInfo letter_op;
            auto letter_img = GenPipTools::letter_image(image, letter_w, letter_h, letter_op, true);
            auto tensor_out = iopipeline_det_->forward(letter_img).begin()->second;
            const int vf_nums = tensor_out->height(); //vf, visual field
            const int per_vf_len = tensor_out->width();
            for (size_t idx = 0; idx < vf_nums; idx++) {
                float* pdata = tensor_out->mutable_cpu_data() + idx * per_vf_len;
                float no_tumble_conf = pdata[4];
                float is_tumble_conf = pdata[5];
                if (is_tumble_conf > conf_thres) {
                    TumbleBBox obj_box(pdata[0] * letter_w, pdata[1] * letter_h, pdata[2] * letter_w, pdata[3] * letter_h, is_tumble_conf, IS_TUMBLE);
                    box_list.push_back(obj_box);
                }
            }
            GenPipTools::nms_cpu(box_list, iou_thres);
            GenPipTools::letter_map_origin_location(box_list, letter_op);

            return box_list;        
        }     

        std::string version()
        {
			const std::string algo_module_version = "3.2.0";
			std::string nn_frame_version = iopipeline_det_->version();
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }


    private:
        std::shared_ptr<PrePostProcessGenPipeline> iopipeline_det_;
        std::shared_ptr<PrePostProcessGenPipeline> iopipeline_cls_;
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

    exposing::param_vector<tumble::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}
