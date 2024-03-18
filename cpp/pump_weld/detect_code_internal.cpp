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

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

#include "yolov8_weld_postproc.hpp"
#include "GenPipline.hpp"
#include "weld_detect.hpp"
#include "obj_box_info.hpp"


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
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::string model = std::string(model_directory) + "/" + "pump_weld.rknn";
#else
            std::string model = std::string(model_directory) + "/" + "pump_weld.onnx";
            //std::string model_inte = std::string(model_directory) + "/" + "pump_weld_intege.onnx";
#endif
			weld_machine_instance_.set_pipline(std::make_shared<GenPipline>(model, device));
#ifdef BUILD_DEBUG_INFO
            weld_machine_instance_.handset_possible_normalization({ 0,0,0 }, { 0.003921568,0.003921568,0.003921568 });//div 255, if backend can hand set normalization, it will, else do nothing
#endif // BUILD_DEBUG_INFO

            // set YOLO8 postprocessing for lixinyao(score-location order in raw cut-onnx output, other is location-score order)
            PostprocessingFunction weld_machine_postproc = weld_concat;
            weld_machine_instance_.set_postprocessing(weld_machine_postproc);
        }

        exposing::param_vector<pump_weld::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int batch, int height, int width, std::map<std::string,float>& param_map_std)
        {
            auto result = exposing::make_param_vector<pump_weld::box_info>();

            size_t bitmap_size = bitmap.size();
            CHECK_EQ(bitmap.size(), batch * height * width * 3);
            // split bitmap to img vector
            std::vector<cv::Mat> BatchImgs;
            for (int i = 0; i < batch; i++) {
                cv::Mat InteImage(cv::Size(width, height), CV_8UC3);
                std::memcpy(InteImage.data, bitmap.data() + height * width * 3 * i, sizeof(uint8_t) * height * width * 3);
                BatchImgs.push_back(InteImage);
            }

            float weld_machine_conf_thres = param_map_std.count("conf_thres") ? param_map_std["conf_thres"] : 0.3f;
            float weld_machine_nms_thres = param_map_std.count("nms_thres") ? param_map_std["nms_thres"] : 0.4f;
            int candidate_box_width= param_map_std.count("candidate_box_width") ? param_map_std["candidate_box_width"] : 500.f;
			int candidate_box_height = param_map_std.count("candidate_box_height") ? param_map_std["candidate_box_height"] : 500.f;

            auto [weld_box_list, candidate_box_list] = weld_detect(BatchImgs, height, width, candidate_box_width, candidate_box_height);

            std::vector<std::vector<ObjBox>> weld_machine_list = weld_machine_seqdet(BatchImgs, weld_machine_conf_thres, weld_machine_nms_thres);

            std::vector<ObjBox> machine_box_ist;
            for (auto& weld_machine_list_frame : weld_machine_list) {
                for (auto& weld_machine : weld_machine_list_frame) {
                    machine_box_ist.push_back(weld_machine);
                }
            }
			auto tube_bbox_list = combine_related_box(machine_box_ist);

#ifdef BUILD_DEBUG_INFO
            //for (auto img : BatchImgs) {
            //    for(auto weld_box : weld_box_list)
            //        cv::rectangle(img, weld_box, { 0, 0, 255 }, 3);
            //    for (auto candidate_box : candidate_box_list)
            //        cv::rectangle(img, candidate_box, { 255, 0, 255 }, 3);
            //    for (auto tube : tube_bbox_list)
            //        cv::rectangle(img, tube.get_rect(), { 0, 255, 0 }, 3);
            //    AdpShow(img);
            //}
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

            for (auto tube_bbox : tube_bbox_list) {
                for (int i = 0; i < candidate_box_flag_list.size(); i++) {
                    auto& candidate_box = candidate_box_flag_list[i];
                    constexpr float MIN_IOU_BETWEEN_TUBE_AND_WELD_BOX = 0.001;

                    auto iou = count_iou(tube_bbox.get_rect(), candidate_box.candidate_box);

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

        ObjBox get_bounding_rect_Obj(ObjBox& a, ObjBox& b) {
            auto bounding_rect = get_outer_box(a.get_rect(), b.get_rect());
			return ObjBox(bounding_rect, std::min(a.score, b.score), -1);
        }

		std::vector<ObjBox> combine_related_box(std::vector<ObjBox> box_list, float iou_threshold = 0.8) {
			std::sort(box_list.begin(), box_list.end(), [](ObjBox& a, ObjBox& b) {
				return a.get_area() > b.get_area();});

            bool flag = true;
            while (flag)
            {
                flag = false;
                auto it = box_list.begin();
                while (it != box_list.end()) {
                    auto curr = it++;
                    if (it == box_list.end()) break;

                    auto to_erase = box_list.end();
                    for (auto other = it; other != box_list.end(); ++other) {
                        if (count_iou(curr->get_rect(), other->get_rect()) >= iou_threshold) {
                            *curr = get_bounding_rect_Obj(*curr, *other);

                            to_erase = other; // 标记要删除的迭代器  
                            flag = true; // 表示发生了合并操作  
                            break; // 跳出循环，因为我们已经更新了curr并且可能要删除一个元素  
                        }
                    }

                    if (to_erase != box_list.end()) {
                        it = box_list.erase(to_erase);
                    }
                }
            }

            return box_list;
        }

        std::vector<std::vector<ObjBox>> weld_machine_seqdet(std::vector<cv::Mat>& BatchImgs, float conf_thres, float nms_thres) {
            std::array<int, 4> det_seq{ 0,2,4,6 };
            std::vector<std::vector<ObjBox>> frames_weld_machine_list;

            for (int seq_idx : det_seq) {
                constexpr int reShapeSide_W = 1152;
                constexpr int reShapeSide_H = 640;

                auto image = BatchImgs[seq_idx];
                //auto image = cv::imread("D:/cyj4.jpeg");

				bool is_horizon_pad = false; //is_horizon_pad is meaningful When pad_val > 0
				int pad_val = 0;
				float resize_scale = 0;
                auto letter_img = GenPiplineTools::letter_image(image, reShapeSide_W, reShapeSide_H, is_horizon_pad, pad_val, resize_scale, true);

                auto rst_map = weld_machine_instance_.forward(letter_img);
                auto tensor_out = rst_map.begin()->second;

                std::vector<ObjBox> weld_machine_list;
                int targetnum = tensor_out->height();
                int infonum = tensor_out->width();

#ifdef BUILD_DEBUG_INFO
                //YHC
                cv::Mat letter_img_cp = letter_img.clone();
#endif // BUILD_DEBUG_INFO

                for (size_t idx = 0; idx < targetnum; idx++) {
                    float* pdata = tensor_out->mutable_cpu_data() + idx * infonum;
                    float chassis_conf = pdata[4];
                    float tube_conf = pdata[5];

                    //if (tube_conf > conf_thres|| chassis_conf > conf_thres)
                    if (tube_conf > conf_thres)
                    {
                        //constexpr float CY_SHIF = 1;
                        constexpr float CY_SHIF = 0.55;
						ObjBox welmbox(pdata[0] * reShapeSide_W, pdata[1] * reShapeSide_H * CY_SHIF, pdata[2] * reShapeSide_W, pdata[3] * reShapeSide_H, tube_conf, 0);
                        weld_machine_list.push_back(welmbox);
//                        
//#ifdef BUILD_DEBUG_INFO
//						cv::rectangle(letter_img_cp, welmbox.get_rect(), { 0,0,255 }, 3);    
//#endif // BUILD_DEBUG_INFO
                    }
                }

				//ImgShow(letter_img_cp);

                for (auto& bbox : weld_machine_list) {
                    bbox.mul_ratio(resize_scale);
                    if (!is_horizon_pad) {
                        bbox.xmin -= pad_val;
                        bbox.xmax -= pad_val;
                    }
                    else {
                        bbox.ymin -= pad_val;
                        bbox.ymax -= pad_val;
                    }
                }

                obj_box_nms_cpu(weld_machine_list, nms_thres);
//#ifdef BUILD_DEBUG_INFO
//                for (auto obj : weld_machine_list) {
//                    cv::rectangle(image, obj.get_rect(), { 0,255,0 }, 3);
//                }
//                AdpShow(image);
//#endif // BUILD_DEBUG_INFO

                frames_weld_machine_list.push_back(weld_machine_list);
            }

            return frames_weld_machine_list;
        }

        std::string version()
        {
            const std::string algo_module_version = "1.5.0";

            std::string nn_frame_version = weld_machine_instance_.version();

            return fmt::format(R"({ {"nn_frame_version":"{}", "algo_module_version" : "{}"} })", nn_frame_version, algo_module_version);
        }

    private:
        PrePostProcessGenPipline weld_machine_instance_;
        PrePostProcessGenPipline weld_machine_instance_inte_;
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
