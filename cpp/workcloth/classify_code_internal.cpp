#include <iostream>
#include <cmath>
#include "hardcode.hpp"

#include "classify_code_internal.hpp"
#include "box_info_impl.hpp"
#include <Excalibur/pipeline.hpp>
#include <Primitives/tensor_conversions.hpp>
//#include <Primitives/pool_allocator.hpp>
//#include <Excalibur/operation_safty_cut.hpp>
//#include "Excalibur/operation_make_border.hpp"
//#include "Excalibur/operation_resize.hpp"
//#include "Excalibur/operation_rgb2gray.hpp"
#include "logger.hpp"

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#ifdef BUILD_DEBUG_INFO
#include <opencv2/highgui/highgui.hpp>
#endif // BUILD_DEBUG_INFO


#include <abi/param_vector.hpp>
#include <Primitives/fmt/format.h>
#include <utility>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif


#ifdef USE_RKNNAPI
//#if 0
#include "../../common/include/RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#endif


namespace glasssix::workcloth
{
    class classify_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
            : model_directory_{ std::string(model_directory) }, device_{ device }
        {
            //Excalibur needs to distinguish between float and int8 models, rknn and rknn2 does not
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            pipline_instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(hardcode::get_model_params("workcloth"), std::string(model_directory) + "/" + "workcloth" + ".rknn", device);
#else
            pipline_instance_ = std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params("workcloth"), std::string(model_directory) + "/" + "workcloth" + ".racy", device);
#endif
        }

        exposing::param_vector<workcloth::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);
            if (roi_x<0 || roi_x>width || roi_y > height || roi_y < 0 || roi_height<0 || (roi_height + roi_y) >height || roi_width<0 || (roi_width + roi_x) > width)
            {
                throw exposing::abi_invalid_argument("incorrect roi in phone");
            }
            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));


            std::vector<box_info_internal> results;
            auto result = exposing::make_param_vector<box_info>();

            run_workcloth(results, cropped_image, param_map);

            for (auto& i : results)
            {
                result.push_back(exposing::make_as_first<box_info_impl>(i));
            }
            return result;
        }

        std::string version()
        {
            const std::string algo_module_version = "1.0.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            //#if 0
            //std::string nn_frame_version = rknnwrapper::rknn_wrapper::version();
            std::string nn_frame_version = pipline_instance_->version();
#else
            std::string nn_frame_version = excalibur::pipeline<float>::version();
#endif
            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

    private:
        void run_workcloth(std::vector<box_info_internal>& results, cv::Mat& image, std::map<std::string, float>& param_map)
        {
            float W = image.cols;
            float H = image.rows;
            float conf_threshold = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.5f;
            float nms_threshold = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.5f;

            auto det_mat = workcloth_imgprocess(image, 640, 640);
            // cvt BGR2RGB
            cv::cvtColor(det_mat, det_mat, cv::COLOR_BGR2RGB);

            float map_ratio = float(std::max(image.cols, image.rows)) / 640;
#ifdef BUILD_DEBUG_INFO
            //cv::imshow("det_mat", det_mat); cv::waitKey(0);
#endif // BUILD_DEBUG_INFO

            std::vector<Bbox> sub_bboxes = detect_boxes(det_mat, conf_threshold, map_ratio);

            for (auto& bbox : sub_bboxes) {
                float padh = float(image.cols - image.rows) / 2;
                bbox.ymin -= padh;
                bbox.ymax -= padh;
                // boundary check
                bbox.ymin = std::max(bbox.ymin, 0.f);
                bbox.ymax = std::max(bbox.ymax, 0.f);
                bbox.ymin = std::min(bbox.ymin, H - 1);
                bbox.ymax = std::min(bbox.ymax, H - 1);

                bbox.xmin = std::max(bbox.xmin, 0.f);
                bbox.xmax = std::max(bbox.xmax, 0.f);
                bbox.xmin = std::min(bbox.xmin, W - 1);
                bbox.xmax = std::min(bbox.xmax, W - 1);
            }

            nms_cpu(sub_bboxes, nms_threshold);

#ifdef BUILD_DEBUG_INFO
            {
                auto visul_mat = image.clone();
                for (auto& bbox : sub_bboxes)
                    cv::rectangle(visul_mat, bbox.get_rect(), bbox.cid ? cv::Scalar{ 0,0,255 } : cv::Scalar{ 0, 255, 0 }, 5);
                cv::resize(visul_mat, visul_mat, cv::Size{}, 0.3, 0.3);
                //cv::imshow("run_boxes", visul_mat); cv::waitKey(0);
            }
#endif // BUILD_DEBUG_INFO

            for (auto box : sub_bboxes) {
                cv::Mat sub_person = image(box.get_rect());
                int sub_w = sub_person.cols;
                int sub_h = sub_person.rows;
                int body_h = sub_h * 0.35;
                int body_start = sub_h * 0.15;
                int leg_h = sub_h * 0.5;

				cv::Mat sub_person_body = sub_person(cv::Rect(0, body_start, sub_w, body_h));
				cv::Mat sub_person_leg = sub_person(cv::Rect(0, leg_h, sub_w, leg_h));
				auto up_bgr = cv::sum(sub_person_body) / (sub_w * body_h);
                auto lw_bgr = cv::sum(sub_person_leg) / (sub_w * leg_h);

#ifdef BUILD_DEBUG_INFO
                std::cout << "up_bgr: " << up_bgr << "\t" << (int)up_bgr[0] << " " << (int)up_bgr[1] << " " << (int)up_bgr[2] << std::endl;
				std::cout << "lw_bgr: " << lw_bgr << "\t" << (int)lw_bgr[0] << " " << (int)lw_bgr[1] << " " << (int)lw_bgr[2] << std::endl;
				std::cout << std::endl;
                //cv::imshow("sub_person_body", sub_person_body); cv::waitKey(0);
                //cv::imshow("sub_person_leg", sub_person_leg); cv::waitKey(0);
                cv::imshow("sub_person", sub_person); cv::waitKey(0);
#endif // BUILD_DEBUG_INFO

                box_info_internal in_box_info;
                in_box_info.score = box.score;
                in_box_info.up_rgb = exposing::make_param_vector<int>();
                in_box_info.up_rgb.push_back((int)up_bgr[2]);//R
                in_box_info.up_rgb.push_back((int)up_bgr[1]);//G
                in_box_info.up_rgb.push_back((int)up_bgr[0]);//B
                in_box_info.lw_rgb = exposing::make_param_vector<int>();
                in_box_info.lw_rgb.push_back((int)lw_bgr[2]);//R
                in_box_info.lw_rgb.push_back((int)lw_bgr[1]);//G
                in_box_info.lw_rgb.push_back((int)lw_bgr[0]);//B

                in_box_info.category = box.cid;
                in_box_info.x1 = box.xmin;
                in_box_info.y1 = box.ymin;
                in_box_info.x2 = box.xmax;
                in_box_info.y2 = box.ymax;
                results.push_back(in_box_info);
            }
        }


        std::vector<Bbox> detect_boxes(cv::Mat img, float conf_threshold, float mapping_ratio) {
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            auto result = pipline_instance_->forward(img.data, { 1, img.rows, img.cols,img.channels() }, RKNN_TENSOR_NHWC);
#else
            auto input_tsr = std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{1, img.rows, img.cols, img.channels()}, -1, memory::NHWC);
            std::copy(img.data, img.data + img.step[0] * img.rows, input_tsr->mutable_cpu_data());
            input_tsr->convert_order();
            auto input_tensor = input_tsr | memory::tensor_convert_to<float>;
            //for (auto i = 0; i < input_tensor->count(); i++) {
            //    input_tensor->mutable_cpu_data()[i] /= 255;
            //}
            auto result = pipline_instance_->forward(input_tensor);
#endif


            std::vector<std::shared_ptr<memory::tensor<float>>> outRst;
            for (auto& out : result) {
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
                CHECK_EQ(out.second->data_shape().size(), 5);
                std::vector<int> shape = out.second->data_shape();
                out.second->reshape(std::vector<int>{shape[0], shape[1], shape[2]* shape[3], shape[4]});
#endif
                outRst.push_back(out.second);
            }
            std::sort(outRst.begin(), outRst.end(), [](const std::shared_ptr<memory::tensor<float>>& A, const std::shared_ptr<memory::tensor<float>>& B) {
                auto countA = A->count();
                auto countB = B->count();
                return countA > countB;
                });

            std::vector<Bbox> preds = concat_yolo(outRst, conf_threshold);

//#ifdef BUILD_DEBUG_INFO
//            {
//                auto visul_mat = img.clone();
//                for (auto& bbox : preds)
//                    cv::rectangle(visul_mat, bbox.get_rect(), bbox.cid ? cv::Scalar{ 0,0,255 } : cv::Scalar{ 0, 255, 0 }, 5);
//                cv::imshow("detect_boxes", visul_mat); cv::waitKey(0);
//            }
//#endif // BUILD_DEBUG_INFO

            for (auto& bbox : preds) {
                bbox.mul_ratio(mapping_ratio);
            }
            return preds;

        }

        std::vector<Bbox> concat_yolo(std::vector<std::shared_ptr<glasssix::memory::tensor<float>>>& forwards, float conf_threshold)
        {
            const float stride[3] = { 8, 16, 32 };
            const float anchors[3][6] = {
            {12,16, 19,36, 40,28},
            {36,75, 76,55, 72,146},
            {142,110, 192,243, 459,401}
            };
            auto max_conf = [](float x, float y) {if (x > y) return x; else return y; };
            auto class_pred = [](float x, float y) {if (x > y) return 1; else return 0; };
            auto class_pred_list = [](std::vector<float>& cls_list) {
                int maxPosdition = std::max_element(cls_list.begin(), cls_list.end()) - cls_list.begin();
                return maxPosdition;
            };

            std::vector<Bbox> boxes_pred;

            for (int n = 0; n < 3; n++)
            {
                auto& block = forwards[n];
                int order = block->order();
                int num = block->num();
                int C = block->channels(); // 3
                int H = block->height(); // [rows * cols]
                int W = block->width();
                int HW_step = H * W;

                int num_grid_x = (int)(640 / stride[n]);
                int num_grid_y = (int)(640 / stride[n]);
                for (int q = 0; q < 3; q++)
                {
                    const float anchor_w = anchors[n][q * 2];
                    const float anchor_h = anchors[n][q * 2 + 1];
                    for (int i = 0; i < num_grid_y; i++)
                    {
                        for (int j = 0; j < num_grid_x; j++)
                        {
                            int cur = q * HW_step + (i * num_grid_x + j) * W;
                            float* pdata = block->mutable_cpu_data() + cur;

                            float x = (sigmoid_x(pdata[0]) * 2.f - 0.5f + j) * stride[n];  //cx
                            float y = (sigmoid_x(pdata[1]) * 2.f - 0.5f + i) * stride[n];  //cy
                            float w = powf(sigmoid_x(pdata[2]) * 2.f, 2.f) * anchor_w;     //w
                            float h = powf(sigmoid_x(pdata[3]) * 2.f, 2.f) * anchor_h;     //h
                            float obj_conf = sigmoid_x(pdata[4]);
                            float person_conf = sigmoid_x(pdata[5]);

                            if (obj_conf > conf_threshold && person_conf > conf_threshold) {
                                Bbox bbox;
                                bbox.xmin = x - w / 2;
                                bbox.xmax = x + w / 2;
                                bbox.ymin = y - h / 2;
                                bbox.ymax = y + h / 2;
                                bbox.score = obj_conf * person_conf;
                                bbox.cid = 0;

                                boxes_pred.push_back(bbox);
                            }
                        }
                    }
                }
            }
            return boxes_pred;
        }

        void nms_cpu(std::vector<Bbox>& bboxes, float iou_thres) {
            if (bboxes.empty()) return;
            std::sort(bboxes.begin(), bboxes.end(), [&](Bbox b1, Bbox b2) {return b1.score > b2.score; });
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
            std::sort(bboxes.begin(), bboxes.end(), [&](Bbox b1, Bbox b2) {return b1.score > b2.score; });
        }

        cv::Mat workcloth_imgprocess(cv::Mat& img, int hope_w = 384, int hope_h = 640) {
            int H = img.rows;
            int W = img.cols;
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
                cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 127,127,127 });
            }
            return resize_img;
        }

        static inline float sigmoid_x(float x)
        {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

    private:
        std::string model_directory_;
        int device_;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        //#if 0
        std::unique_ptr<rknnwrapper::rknn_wrapper> pipline_instance_;
#else
        std::unique_ptr<excalibur::pipeline<float>> pipline_instance_;
#endif
    };

    classify_code_internal::classify_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    classify_code_internal::~classify_code_internal() = default;

    std::string classify_code_internal::version()
    {
        return impl_->version();
    }

    exposing::param_vector<workcloth::box_info> classify_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}