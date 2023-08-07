#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"

#include "hardcode.hpp"

#include <abi/param_vector.hpp>
#include <utility>

#include <RKNN2Wrapper/rknn2_wrapper.hpp>

#include <opencv2/dnn.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace glasssix::playphone
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                : impl{get_model_params("playphone", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
                :net_instance_(phai,  model_directory + std::string("/playphone_sim.rknn"), device)
        {

        }

        exposing::param_vector<playphone::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
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
                  throw exposing::abi_invalid_argument("incorrect roi in playphone");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width));

            auto result = run_detect(cropped_image, roi_x, roi_y, roi_width, roi_height, param_map);

            auto results = exposing::make_param_vector<playphone::box_info>();

            for(auto& it:result) {
                it.x1+=roi_x;
                it.x2+=roi_x;
                it.y1+=roi_y;
                it.y2+=roi_y;
                results.push_back(glasssix::exposing::make_as_first<box_info_impl>(it));
            }

            return results;
        }

        std::string version()
        {
			const std::string algo_module_version = "1.0.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			//#if 0
			std::string nn_frame_version = net_instance_.version();
#else
			std::string nn_frame_version = net_instance_.version();
#endif
			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);

        }

    private:

        struct box
        {
            int x1;
            int y1;
            int x2;
            int y2;
            int category;
        };

        // sigmoid
        static inline float sigmoid_x(float x) {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

        // letterbox
        static std::pair<cv::Mat, float> letterbox(cv::Mat& img) {
            auto new_shape = cv::Size(640, 640);
            int H = img.rows;
            int W = img.cols;
            float ratio_w = (float)W / (float)new_shape.width;
            float ratio_h = (float)H / (float)new_shape.height;
            float ratio = ratio_w;

            cv::Mat resize_img;
            if (H == new_shape.height && W == new_shape.width) {
                resize_img = img;
            }
            else {
                if (ratio_w == ratio_h) {
                    cv::resize(img, resize_img, cv::Size2i{ new_shape.width, new_shape.height });
                }
                else if (ratio_w > ratio_h) {

                    int new_x = new_shape.width;
                    int new_y = (int)(H / ratio_w);
                    int pad1 = (int)((new_shape.height - new_y) / 2);
                    int pad2 = new_shape.height - new_y - pad1;
                    cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                    cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT,
                        cv::Scalar{ 114, 114, 114 });
                }
                else {
                    ratio = ratio_h;
                    int new_y = new_shape.height;
                    int new_x = (int)(W / ratio_h);
                    int pad1 = (int)((new_shape.width - new_x) / 2);
                    int pad2 = new_shape.width - new_x - pad1;
                    cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
                    cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT,
                        cv::Scalar{ 114, 114, 114 });
                }
            }

            return { resize_img, ratio };
        }

        /**
         * @brief preprocess
         *
         */
        std::pair<cv::Mat, float> preprocess(cv::Mat& image) {
            // letterbox
            cv::Mat crop_image;
            float ratio;
            std::tie(crop_image, ratio) = letterbox(image);

            // cvt BGR2RGB
            cv::Mat rgb_image;
            cv::cvtColor(crop_image, rgb_image, cv::COLOR_BGR2RGB);

            return std::make_pair(rgb_image, ratio);
        }

		/**
		 * @fun concat
		 * @param infer_out, conf_thres
		 * @return source
		 * @details concat 3 into 1
		 */
        static std::vector<std::vector<float>> concat(std::vector<std::shared_ptr<memory::tensor<float>>>& outs, float conf_thres)
        {

            const float anchors[3][6] = { {116,90, 156,198, 373,326},{30,61, 62,45, 59,119},{10,13, 16,30, 33,23}};
            const int stride[3] = {  20,40,80};
            const float strides[3] = { 32.0, 16.0, 8.0};
            std::vector<std::vector<float>> result;
            for(int n = 0; n < 3; n++)
            {
                int num_grid_x = stride[n];
                int num_grid_y = stride[n];

                int ind = 0;
                const float *ptr_out=outs[n]->cpu_data();
                for(int q = 0; q < 3; q++)
                {
                    const float anchor_w = anchors[n][q * 2];
                    const float anchor_h = anchors[n][q * 2 + 1];
                    for(int i = 0; i < num_grid_x; i++)
                    {
                        for(int j = 0; j < num_grid_y; j++)
                        {

                            const float* pdata = ptr_out + ind *  7;

                            float box_score = sigmoid_x(pdata[4]);

                            if(box_score > conf_thres)
                            {
                                float cx = (sigmoid_x(pdata[0]) * 2.f - 0.5f + j) * strides[n];  //cx
                                float cy = (sigmoid_x(pdata[1]) * 2.f - 0.5f + i) * strides[n];  //cy
                                float w = powf(sigmoid_x(pdata[2]) * 2.f, 2.f) * anchor_w;      //w
                                float h = powf(sigmoid_x(pdata[3]) * 2.f, 2.f) * anchor_h;      //h
                                std::vector<float> element = {cx, cy, w, h, box_score, sigmoid_x(pdata[5]), sigmoid_x(pdata[6])};
                                result.push_back(element);
                            }
                            ind++;
                        }
                    }
                }
            }
            return result;
        }

		/**
		 * @fun computeNx6
		 * @param anchor, conf_thres
		 * @return [box,confidence,category]
		 * @details concat xywh into nx6
		 */
         static std::vector<playphone::box_info_internal> computeNx6(std::vector<std::vector<float>>& src, float conf_thres)
         {
             std::vector<playphone::box_info_internal> res;
             for (auto it : src)
             {
                 float top_x = it[0] - it[2] / 2;
                 float top_y = it[1] - it[3] / 2;
                 float bot_x = it[0] + it[2] / 2;
                 float bot_y = it[1] + it[3] / 2;
                 float conf = it[4];
                 int maxPosition = std::max_element(it.begin() + 5, it.end()) - it.begin();
                 if (it[maxPosition] * conf > conf_thres)
                 {
                     playphone::box_info_internal temp{};
                     temp.x1 = top_x;
                     temp.y1 = top_y;
                     temp.x2 = bot_x;
                     temp.y2 = bot_y;
                     temp.score = conf;
                     temp.category = maxPosition - 5;
                     res.push_back(temp);
                 }
             }
             return res;
         }

         /**
          * @fun computNmsInput
          * @param src, max_wh
          * @return std::pair<bboxes, confidence>
          * @details slice dnn_src into bboxes and confidence, which need by dnn::NMS
          */
         static std::tuple<std::vector<cv::Rect2d>, std::vector<float>, std::vector<int>> computeNmsInput(std::vector<playphone::box_info_internal>& src, int max_wh)
         {
             std::vector<cv::Rect2d> boxes;
             std::vector<float> scores;
             std::vector<int> category;
             for (auto const& it : src)
             {
                 int c = max_wh * it.category;
                 cv::Rect2d temp;
                 temp.x = static_cast<double>(it.x1 + c);
                 temp.y = static_cast<double>(it.y1 + c);
                 temp.width = static_cast<double>(it.x2 - it.x1);
                 temp.height = static_cast<double>(it.y2 - it.y1);
                 boxes.push_back(temp);
                 scores.push_back(it.score);
                 category.push_back(it.category);
             }
             return std::make_tuple(boxes, scores, category);
         }

        /**
         * @fun non_max_suppression
         * @param prediction, conf_thres, iou_thres
         * @return std::vector(boxes, classes)
         * @details Non-Maximum Suppression (NMS) on inference results
         */
        static std::vector<playphone::box_info_internal> non_max_suppression(std::vector<std::vector<float>>& prediction, float conf_thres, float iou_thres)
        {
            // Compute conf = obj_conf * cls_conf
            // Box (center x, center y, width, height) to (x1, y1, x2, y2, conf, classes)
            auto compute_box = computeNx6(prediction, conf_thres);

            // Batched NMS
            int max_wh = 4096;
            std::vector<cv::Rect2d> bboxes;
            std::vector<float> scores;
            std::vector<int> classes;

            std::tie(bboxes, scores, classes) = computeNmsInput(compute_box, max_wh);

            // Perform non-maximum suppression to eliminate redundant overlapping boxes with
            // lower confidences
            std::vector<int> indices;
            cv::dnn::NMSBoxes(bboxes, scores, conf_thres, iou_thres, indices);

            std::vector<playphone::box_info_internal> output;

            // x < 0 return 0; x > 0 return x;
            auto f = [](int x) {if (x < 0) return 0; else return x; };

            for (int idx : indices)
            {
                playphone::box_info_internal temp{};
                temp.x1 = f(static_cast<int>(compute_box[idx].x1));
                temp.y1 = f(static_cast<int>(compute_box[idx].y1));
                temp.x2 = f(static_cast<int>(compute_box[idx].x2));
                temp.y2 = f(static_cast<int>(compute_box[idx].y2));
                temp.score = compute_box[idx].score;
                temp.category = compute_box[idx].category;

                output.emplace_back(temp);
            }

            return output;
        }

        /**
         * @fun scale_coords
         * @param coords, old_image, new_image, step
         * @return coords
         */
        std::vector<playphone::box_info_internal> scale_coords(std::vector<playphone::box_info_internal>& coords, cv::Size& input_shape, cv::Size& output_shape)
        {
            std::vector<playphone::box_info_internal> scale_coords_pt;

            auto clamp = [](int x, int min, int max) {if (x < min) return min; else if (x > max) return max; else return x; };

            // gain
            float gain = std::min(input_shape.width / (float)output_shape.width, input_shape.height / (float)output_shape.height);

            // pad
            float pad_w = (input_shape.width - output_shape.width * gain) / 2.0;
            float pad_h = (input_shape.height - output_shape.height * gain) / 2.0;

            // scale coords on point
            for (const auto& it : coords)
            {
                playphone::box_info_internal temp{};

                temp.x1 = (it.x1 - pad_w) / gain;
                temp.y1 = (it.y1 - pad_h) / gain;
                temp.x2 = (it.x2 - pad_w) / gain;
                temp.y2 = (it.y2 - pad_h) / gain;

                clamp(temp.x1, 0, output_shape.width);
                clamp(temp.y1, 0, output_shape.height);
                clamp(temp.x2, 0, output_shape.width);
                clamp(temp.y2, 0, output_shape.height);

                temp.score = it.score;
                temp.category = it.category;
                scale_coords_pt.push_back(temp);
            }

            return scale_coords_pt;
        }

        /**
           * @fun run_detect
           * @param image param_map
           * @return std::vector<playphone::box_info_internal>
           * @details run detect (maybe in multithreading)
        */
        std::vector<playphone::box_info_internal> run_detect(cv::Mat& image, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            std::map<std::string, float> params = {
                    {"conf_thres", param_map.count("conf_thres") ? param_map["conf_thres"] : 0.4f},
                    {"iou_thres",  param_map.count("iou_thres") ? param_map["iou_thres"] : 0.5f}};
			
            cv::Mat blob;
            float ratio;
            std::tie(blob, ratio) = preprocess(image);

            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;

            auto  network_result = net_instance_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);

            std::vector<std::string>  out_names={"534","522","output"};


            for (size_t i=0;i< 3; i++)//对输出数据做处理
            {
                forwards.push_back(network_result[out_names[i]]);
            }

			float conf_threshold = 0.5f;
			float iou_threshold = 0.5f;

			auto result = concat(forwards, conf_threshold);

            auto nms_result = non_max_suppression(result, conf_threshold, iou_threshold);

            auto input_shape  = cv::Size(blob.cols, blob.rows);
            auto output_shape = cv::Size(image.cols, image.rows);

            auto scale_box = scale_coords(nms_result, input_shape, output_shape);

            return scale_box;
        }


    private:
        std::string model_directory_;
        int device_;
        rknnwrapper::rknn_wrapper net_instance_;

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

    exposing::param_vector<playphone::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}
