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
#include <opencv2/imgproc/imgproc.hpp>

#if defined(USE_BMNN)
#include <sophonyolov8/SophonYolov8Wrapper.hpp>
#endif

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include <GenPipeline/GetPostprocessing.hpp>
#include "../genpipeline/market/yolov8_GEN.hpp"

#include "weld_detect.hpp"

#ifdef BUILD_DEBUG_INFO
#define GetShowRatio(visual_img) std::min(float(1920.f / visual_img.cols), float(1080.f / visual_img.rows)) * 0.75
#define ShowResize(visual_img, showRatio) cv::resize(visual_img, visual_img, cv::Size(), showRatio, showRatio)
#define ImgShow(visual_img) cv::imshow("visual_img", visual_img);cv::waitKey(0)
#define AdpShow(img) {auto visual_img=img.clone();ShowResize(visual_img,GetShowRatio(visual_img));ImgShow(visual_img);}
#endif // BUILD_DEBUG_INFO

namespace glasssix::pump_weld
{
    class detect_code_internal::impl
    {
    public:
        impl(int device) noexcept : device_{ device } {}
        impl(std::string_view model_directory, int device): impl(device)
        {
            std::string model_dir = exposing::to_narrow_string(model_directory) + "/";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "pump_weld.rknn", 0);
            ioprocess_pipeline_->manual_possible_normalization(0, 0.003921568);
            ioprocess_pipeline_->set_postprocessing(yolov8_GEN<3, 0>);
#elif defined(USE_BMNN)
            ioprocess_pipeline_ = std::make_shared<SophonYolov8Wrapper>(std::string(model_directory) + "/pump_weld.bmodel");
            ioprocess_pipeline_->init();
#endif
        }

        exposing::param_vector<pump_weld::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int batch, int height, int width, std::map<std::string,float>& param_map_std)
        {
			if (bitmap.empty() || bitmap.size() != batch * height * width * 3)
            {
                throw exposing::abi_invalid_argument("invalid image frame");
            }
            auto result = exposing::make_param_vector<pump_weld::box_info>();
            // split bitmap to img vector
            std::vector<cv::Mat> BatchImgs;
            for (int i = 0; i < batch; i++) {
				cv::Mat InteImage(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data() + height * width * 3 * i));
                BatchImgs.push_back(InteImage.clone());
            }

            float wmachine_conf_thres = param_map_std.count("wmachine_conf_thres") ? param_map_std["wmachine_conf_thres"] : 0.3f;
            float wlight_conf_thres = param_map_std.count("wlight_conf_thres") ? param_map_std["wlight_conf_thres"] : 0.6f;
            float weld_machine_nms_thres = param_map_std.count("nms_thres") ? param_map_std["nms_thres"] : 0.4f;
            int candidate_box_width= param_map_std.count("candidate_box_width") ? param_map_std["candidate_box_width"] : 500.f;
			int candidate_box_height = param_map_std.count("candidate_box_height") ? param_map_std["candidate_box_height"] : 500.f;

            //auto [weld_box_list, candidate_box_list] = weld_detect(BatchImgs, height, width, candidate_box_width, candidate_box_height);

			auto [wlight_list_batch, machine_list_batch] = weld_yolo_seqdet(BatchImgs, wmachine_conf_thres, weld_machine_nms_thres);

            //std::vector<std::vector<cv::Rect>> time_light_box_list;
            //for (auto& wlight_list : wlight_list_batch) {
            //    std::vector<cv::Rect> wlight_list_rec;
            //    for (auto& wlight : wlight_list) {
            //        wlight_list_rec.push_back(wlight.get_rect());
            //    }
            //    time_light_box_list.emplace_back(wlight_list_rec);
            //}

			auto weld_box_list = get_weld_box(wlight_list_batch, wlight_conf_thres);
            auto candidate_box_list = weld_box_list;
            get_candidate_box(candidate_box_list, width, height, candidate_box_width, candidate_box_height);


#ifdef BUILD_DEBUG_INFO
            //auto vs = BatchImgs[0];
            //for (auto& wlight_list : wlight_list_batch) {
            //    for (auto& wlight : wlight_list) {
            //        cv::rectangle(vs, wlight.get_rect(), { 255, 255, 255 }, 3);
            //    }
            //}
            //for (auto candidate : candidate_box_list) {
            //    cv::rectangle(vs, candidate, { 255, 0, 255 }, 3);
            //}
            //for (auto machine : machine_list_batch) {
            //    cv::rectangle(vs, machine.get_rect(), { 0, 0, 255 }, 3);
            //}
            //AdpShow(vs);
#endif // BUILD_DEBUG_INFO

            constexpr bool STANDARD_WELD = false;
            constexpr bool UNSTANDARD_WELD = true;

            struct CandidateBoxFlaglist
            {
                std::vector<cv::Rect> weld_boxes;
                cv::Rect candidate_box;
                bool category; //0=standard, 1=unstandard
                CandidateBoxFlaglist(cv::Rect& candidate_box_, std::vector<cv::Rect>& weld_boxes_, bool category_) :
                    candidate_box(candidate_box_), weld_boxes(weld_boxes_), category(category_) {}
            };

            std::vector<CandidateBoxFlaglist> candidate_box_flag_list;
            for (int i = 0; i < candidate_box_list.size(); i++) {
                auto& candidate_box = candidate_box_list[i];
                candidate_box_flag_list.emplace_back(CandidateBoxFlaglist{ candidate_box, weld_box_list, UNSTANDARD_WELD });
            }

			for (auto machine_bbox : machine_list_batch) {
				for (int i = 0; i < candidate_box_flag_list.size(); i++) {
					auto& candidate_box = candidate_box_flag_list[i];
					constexpr float MIN_IOU_BETWEEN_TUBE_AND_WELD_BOX = 0.001;

					auto iou = weld_count_iou(machine_bbox.get_rect(), candidate_box.candidate_box);

//#ifdef BUILD_DEBUG_INFO
//					cv::rectangle(BatchImgs[0], tube_bbox.get_rect(), { 0, 255, 0 }, 3);
//					cv::rectangle(BatchImgs[0], candidate_box, { 0, 0, 255 }, 3);
//					AdpShow(BatchImgs[0]);
//#endif // BUILD_DEBUG_INFO

					if (iou >= MIN_IOU_BETWEEN_TUBE_AND_WELD_BOX) {
						candidate_box.category = STANDARD_WELD;
					}
				}
			}

			for (auto& candidate_box_flag : candidate_box_flag_list) {
				box_info_internal candidate_box_internal;
				for (auto weld_box : candidate_box_flag.weld_boxes) {
					candidate_box_internal.weldlocal_list.push_back(weld_box.x);
					candidate_box_internal.weldlocal_list.push_back(weld_box.y);
					candidate_box_internal.weldlocal_list.push_back(weld_box.x + weld_box.width);
					candidate_box_internal.weldlocal_list.push_back(weld_box.y + weld_box.height);
				}

				candidate_box_internal.can_x1 = candidate_box_flag.candidate_box.x;
				candidate_box_internal.can_y1 = candidate_box_flag.candidate_box.y;
				candidate_box_internal.can_x2 = candidate_box_flag.candidate_box.x + candidate_box_flag.candidate_box.width;
				candidate_box_internal.can_y2 = candidate_box_flag.candidate_box.y + candidate_box_flag.candidate_box.height;

				candidate_box_internal.category = candidate_box_flag.category;
				result.push_back(glasssix::exposing::make_as_first<box_info_impl>(candidate_box_internal));
			}

            return result;
        }

        struct MachineBox :public GenPipTools::YoloBoxBase {
        public:
            using YoloBoxBase::YoloBoxBase; //Inheriting Constructors
        };

		std::pair<std::vector<std::vector<WlightBox>>, std::vector<MachineBox>> weld_yolo_seqdet(std::vector<cv::Mat>& BatchImgs, float wmachine_conf_thres, float weld_machine_nms_thres) {
            constexpr int infrW = 1280;
            constexpr int infrH = 736;
            constexpr bool ifCvtRGB = true;

            std::vector<std::vector<WlightBox>> wlight_list_batch;
            std::vector<MachineBox> machine_list_batch;

            for (auto image : BatchImgs) {
                std::vector<MachineBox> chassis_list;
                std::vector<MachineBox> tube_list;
                std::vector<WlightBox> wlight_list;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
                GenPipTools::LetterInfo letter_op;
                auto letter_img = GenPipTools::letter_image(image, infrW, infrH, letter_op, ifCvtRGB);
                auto net_rstmap = ioprocess_pipeline_->forward(letter_img);
                auto tensor_out = net_rstmap.begin()->second;
                const int vf_nums = tensor_out->height(); //vf, visual field
                const int per_vf_len = tensor_out->width();
                for (size_t idx = 0; idx < vf_nums; idx++) {
                    float* pdata = tensor_out->mutable_cpu_data() + idx * per_vf_len;
                    float chassis_conf = pdata[4];
                    float tube_conf = pdata[5];
                    float wlight_conf = pdata[6];

                    if (chassis_conf > wmachine_conf_thres) {
                        MachineBox chassisBox(pdata[0] * infrW, pdata[1] * infrH, pdata[2] * infrW, pdata[3] * infrH, chassis_conf, 0);
                        chassis_list.push_back(chassisBox);
                    }
                    if (tube_conf > wmachine_conf_thres) {
                        MachineBox tubeBox(pdata[0] * infrW, pdata[1] * infrH, pdata[2] * infrW, pdata[3] * infrH, tube_conf, 1);
                        tube_list.push_back(tubeBox);
                    }
                    if (wlight_conf > wmachine_conf_thres) {
                        WlightBox wlightBox(pdata[0] * infrW, pdata[1] * infrH, pdata[2] * infrW, pdata[3] * infrH, wlight_conf, 3);
                        wlight_list.push_back(wlightBox);
                    }
                }
                GenPipTools::nms_cpu(chassis_list, weld_machine_nms_thres);
                GenPipTools::nms_cpu(tube_list, weld_machine_nms_thres);
                GenPipTools::nms_cpu(wlight_list, weld_machine_nms_thres);
                GenPipTools::letter_map_origin_location(chassis_list, letter_op);
                GenPipTools::letter_map_origin_location(tube_list, letter_op);
                GenPipTools::letter_map_origin_location(wlight_list, letter_op);
#elif defined(USE_BMNN)
                auto cropped_result = ioprocess_pipeline_->get_objects(image, wmachine_conf_thres, weld_machine_nms_thres);// ∑¿ª§√Ê’÷ºÏ≤‚
                for (auto& object : cropped_result)
                {
                    if (object.category == 0)
                    {
                        MachineBox chassisBox(object.x1, object.y1, object.x2 - object.x1, object.y2 - object.y1, object.score, 0);
                        chassis_list.push_back(chassisBox);
                    }
                    else if (object.category == 1)
                    {
                        MachineBox tubeBox(object.x1, object.y1, object.x2 - object.x1, object.y2 - object.y1, object.score, 1);
                        tube_list.push_back(tubeBox);
                    }
                    else if (object.category == 2)
                    {
                        WlightBox wlightBox(object.x1, object.y1, object.x2- object.x1, object.y2 - object.y1, object.score, 3);
                        wlight_list.push_back(wlightBox);
                    }
                }
#endif  
                machine_list_batch.insert(machine_list_batch.begin(), chassis_list.begin(), chassis_list.end());
                machine_list_batch.insert(machine_list_batch.begin(), tube_list.begin(), tube_list.end());
                wlight_list_batch.push_back(wlight_list);
            }

            return { wlight_list_batch, machine_list_batch };
        }

        std::string version()
        {
            const std::string algo_module_version = "2.0.3";

            std::string nn_frame_version = "2.0.3";

            return fmt::format(R"({ {"nn_frame_version":"{}", "algo_module_version" : "{}"} })", nn_frame_version, algo_module_version);
        }

    private:
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::shared_ptr<PrePostProcessGenPipeline> ioprocess_pipeline_;
#elif defined(USE_BMNN)
        std::shared_ptr<SophonYolov8Wrapper> ioprocess_pipeline_;
#endif
        int device_;
    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal()
    {
    }

    exposing::param_vector<pump_weld::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int batch, int height, int width, std::map<std::string,float>& param_map_std)
    {
        return impl_->detect(bitmap, batch, height, width, param_map_std);
    }

    std::string detect_code_internal::version()
    {
        return impl_->version();
    }
}
