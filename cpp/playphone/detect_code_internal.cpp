#include <iostream>
#include <cmath>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "../posture/box_info.hpp"

#include <GenPipeline/GenPipeTools.hpp>
#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include "../genpipeline/market/yolov8_GEN.hpp"

namespace glasssix::playphone
{
    class detect_code_internal::impl {
    public:
        impl() {}

        impl(std::string_view model_directory, int device) :impl()
        {
            std::string model_dir = exposing::to_narrow_string(model_directory);
            if (*model_dir.rbegin() != '/') model_dir += '/';
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            iopipe_phone_det_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "playphone_4b.rknn", 0);
#elif defined(USE_BMNN)
            iopipe_phone_det_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "playphone_4b.bmodel", 0);
#else
            iopipe_phone_det_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "playphone_4b.onnx", 0);
#endif
            iopipe_phone_det_->manual_possible_normalization(0, 1.f / 255);
            iopipe_phone_det_->set_postprocessing(yolov8_GEN<1, 1>);
        }

        exposing::param_vector<playphone::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list_raw, std::map<std::string, float>& param_map)
        {
            auto result = exposing::make_param_vector<playphone::box_info>();
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            if (roi_x < 0 || roi_x > width || roi_y > height || roi_y < 0 || roi_height < 0 || (roi_height + roi_y) > height || roi_width < 0 || (roi_width + roi_x) > width)
            {
                throw exposing::abi_invalid_argument("incorrect roi");
            }

            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));

            float phone_conf_thres = param_map.count("phone_conf_thres") ? param_map["phone_conf_thres"] : 0.7f;
            float phone_nms_thres = param_map.count("phone_nms_thres") ? param_map["phone_nms_thres"] : 0.5f;

            for (auto pinfo : posture_info_list_raw)
            {
                PostureInfo postureInfo{ pinfo };
				postureInfo.set_origin_image_border(0, 0, width, height);

                box_info_internal pphone_box_info;
                pphone_box_info.set_man(postureInfo);

                if (postureInfo.invaild_hand_kpnum() < 2 && postureInfo.invaild_face_kpnum() < 2)
                {
                    //detect phones
                    const auto playphone_det_region_rect = postureInfo.get_playphone_det_region(); // upperbody_img
                    const int max_upperbody_img_side = std::max(playphone_det_region_rect.width, playphone_det_region_rect.height);
                    // Ear center to phone center threshold: 0.16 ¡Á longest side of playphone_det_region_rect.
                    const float ear_tresh = max_upperbody_img_side * 0.16f;
                    // Ear-to-nose distance too close threshold: 0.12 ¡Á longest side of playphone_det_region_rect.
					const float hand_nose_thresh= max_upperbody_img_side * 0.12f;
                    const bool hand_close_nose = postureInfo.if_hand_close_nose(hand_nose_thresh);

                    auto playphone_det_region = GenPipTools::safty_cut(image, playphone_det_region_rect);
                    std::vector<PhoneBox> phone_list = phone_detect(playphone_det_region, phone_conf_thres, phone_nms_thres);

                    for (auto& phoneObj : phone_list)
                    {
                        phoneObj.add(playphone_det_region_rect.tl()); // mapping location
                        auto phoneRect = phoneObj.get_rect();

                        if (hand_close_nose)
                            phoneObj.score *= 0.71;

                        // phone too close ears
						cv::Point phoneRectCenter(phoneRect.x + phoneRect.width / 2, phoneRect.y + phoneRect.height / 2);
                        auto earD1 = cv::norm(phoneRectCenter - postureInfo.Kpoints[3]);
                        auto earD2 = cv::norm(phoneRectCenter - postureInfo.Kpoints[4]);
                        if (earD1 < ear_tresh || earD2 < ear_tresh) {
                            continue;
                        }

                        auto hands_region = postureInfo.get_playphone_hands_region();
                        // phone traversing match hands
                        for (auto& hand_region : hands_region)
                        {
							auto is_overlap = overlap(phoneRect, hand_region, 0.1);

                            if (is_overlap)
                            {
                                if (phoneRect.area() >= hand_region.area() * 5) {
                                    phoneObj.score *= 0.75;
                                }

                                pphone_box_info.set_phone(phoneObj);
                                break;
                            }
                        }
                    }
                }
                else 
                {
                    //body error
                    pphone_box_info.set_body_error(postureInfo);
                }

                result.push_back(exposing::make_as_first<box_info_impl>(pphone_box_info));
            }

            return result;
        }

        std::vector<PhoneBox> phone_detect(cv::Mat& image, float conf_thres, float iou_thres) {
            const int letter_h = 384;
            const int letter_w = 384;
            std::vector<PhoneBox> box_list;

            GenPipTools::LetterInfo letter_op;
            auto letter_img = GenPipTools::letter_image(image, letter_w, letter_h, letter_op, true);
            auto tensor_out = iopipe_phone_det_->forward(letter_img).begin()->second;
            const int vf_nums = tensor_out->height(); //vf, visual field
            const int per_vf_len = tensor_out->width();
            for (size_t idx = 0; idx < vf_nums; idx++) {
                float* pdata = tensor_out->mutable_cpu_data() + idx * per_vf_len;
                float phone_conf = pdata[4];
                if (phone_conf > conf_thres) {
                    PhoneBox obj_box(pdata[0] * letter_w, pdata[1] * letter_h, pdata[2] * letter_w, pdata[3] * letter_h, phone_conf, 1);
                    box_list.push_back(obj_box);
                }
            }
            GenPipTools::nms_cpu(box_list, iou_thres);
            GenPipTools::letter_map_origin_location(box_list, letter_op);
            return box_list;
        }

        // if rectA intersect rectB
        bool overlap(cv::Rect phoneRect, cv::Rect handRect, float iou_thres)
        {
            cv::Rect inteRect = phoneRect & handRect;
            float iou_phone = inteRect.area() * 1.f / phoneRect.area();
            return iou_phone > 0.1;
        }

        std::string version()
        {
			const std::string algo_module_version = "3.0.0";
			std::string nn_frame_version = iopipe_phone_det_->version();
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

        static inline cv::Mat playphone_HSVCover_preprocess(cv::Mat img) {

            cv::Mat md_img;
            cv::Mat hsv_image;
            cv::Mat black_mask;
            cv::cvtColor(img, hsv_image, cv::COLOR_BGR2HSV);
            const cv::Scalar lower_black_ = cv::Scalar{ 0, 0, 0 };
            const cv::Scalar upper_black_ = cv::Scalar{ 180, 255, 60 };
            cv::inRange(hsv_image, lower_black_, upper_black_, black_mask);

            for (int row = 0; row < black_mask.rows; ++row)
            {
                for (int col = 0; col < black_mask.cols; ++col)
                {
                    auto& hsv = hsv_image.at<cv::Vec3b>(row, col);
                    if (black_mask.at<uchar>(row, col) > 0 || black_mask.at<uchar>(row, col) > 0)
                    {
                        hsv = { 0, 0, 65 };
                    }
                }
            }
            cv::cvtColor(hsv_image, md_img, cv::COLOR_HSV2RGB);// is RGB!
            return md_img;
        }

    private:
        std::shared_ptr<PrePostProcessGenPipeline> iopipe_phone_det_;
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

    exposing::param_vector<playphone::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, posture_info_list, param_map);
    }
}
