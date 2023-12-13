#include <iostream>
#include <cmath>
#include "hardcode.hpp"

#include "classify_code_internal.hpp"
#include "box_info_impl.hpp"
#include <Excalibur/pipeline.hpp>
#include <Primitives/tensor_conversions.hpp>
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


namespace glasssix::pedestrian
{
    class classify_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
            : impl{ get_model_params("pedestrian", false), exposing::to_narrow_string(model_directory), device }
        {
        }

        impl(const std::vector<std::string>& phai, std::string model_directory, int device)
            : net_detect_(phai, model_directory + std::string("/pedestrian.rknn"), device)
        {
        }
                exposing::param_vector<pedestrian::box_info> detect(const exposing::param_span<std::uint8_t> &bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float> &param_map)
        {
            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.5f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;
            bool is_wander_call = param_map.count("wander") ? param_map["wander"] : 0.f;

            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * channels * height * width);

            if (roi_x < 0 || roi_x > width || roi_y > height || roi_y < 0 || roi_height < 0 || (roi_height + roi_y) > height || roi_width < 0 || (roi_width + roi_x) > width)
            {
                throw exposing::abi_invalid_argument("incorrect roi in universal_pedestrian");
            }

            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));

            std::vector<pedestrian::box_info_internal> result = universal_pedestrian_detect(cropped_image, con_thres, iou_thres, is_wander_call);

            auto results = exposing::make_param_vector<pedestrian::box_info>();

            for (auto &it : result)
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
            const std::string algo_module_version = "3.1.0";

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            // #if 0
            std::string nn_frame_version = net_detect_.version();
#else
            std::string nn_frame_version = net_detect_.version();
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

        void Softmax(float *data, int num)
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

        inline float sigmoid_x(float x)
        {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

        void tranpose(std::shared_ptr<memory::tensor<float>> &data, std::shared_ptr<memory::tensor<float>> &dest)
        {
            const float *sour_ptr = data->cpu_data();

            float *dest_ptr = dest->mutable_cpu_data();

            int dim_2 = dest->count() / 33600;

            for (int i = 0; i < dim_2; i++)
            {
                for (int j = 0; j < 33600; j++)
                {
                    dest_ptr[j * dim_2 + i] = sour_ptr[i * 33600 + j];
                }
            }
        }
        std::shared_ptr<glasssix::memory::tensor<float>> Concat(std::vector<std::shared_ptr<memory::tensor<float>>>& outs, float conf_thres)
        {
            // 40 80 160
            std::vector<float> cat(65 * 33600); // 1*65*33600 = 64*33600 + 1*33600
            const float* data80 = outs[2]->cpu_data();
            const float* data40 = outs[1]->cpu_data();
            const float* data20 = outs[0]->cpu_data();
            // int i=0;
            int Candidate = 33600;
            for (int i = 0; i < 65; i++)
            {
                int j = 0;
                for (; j < 25600; j++)
                {
                    cat[i * Candidate + j] = data80[i * 25600 + j];
                }
                for (; j < 32000; j++)
                {
                    cat[i * Candidate + j] = data40[i * 6400 + j - 25600];
                }

                for (; j < 33600; j++)
                {
                    cat[i * Candidate + j] = data20[i * 1600 + j - 32000];
                }
            }

            // boxes cat[0:64*33600]

            std::vector<float> reshape_box(33600 * 64);
            // tranpose and softmax
            for (int i = 0; i < 64; i++)
            {
                for (int j = 0; j < 33600; j++)
                {
                    reshape_box[j * 64 + i] = cat[i * 33600 + j];
                }
            }

            int index = 0;
            for (int i = 0; i < 33600; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    Softmax(reshape_box.data() + 16 * index, 16);
                    index++;
                }
            }

            // reshape and tranpose  64*33600 ->33600*64
            std::vector<float> reshape_box2(16 * 4 * 33600);

            std::array<float, 64> temp;

            for (int i = 0; i < 33600; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    for (int k = 0; k < 16; k++)
                    {
                        reshape_box2[k * 4 * 33600 + j * 33600 + i] = reshape_box[i * 16 * 4 + j * 16 + k];
                    }
                }
            }

            std::vector<float> conv(4 * 33600);

            for (int i = 0; i < 4 * 33600; i++)
            {
                conv[i] = 0.f;
            }

            // 16个通道 1*1卷积
            for (int i = 0; i < 16; i++)
            {
                for (int j = 0; j < 4 * 33600; j++)
                {
                    int location = 4 * 33600;
                    reshape_box2[i * location + j] = reshape_box2[i * location + j] * i;
                    conv[j] = conv[j] + reshape_box2[i * location + j];
                }
            }

            // slice and function operator

            std::vector<float> sub_add(33600 * 2);

            for (int i = 0; i < 25600; i++)
            {
                sub_add[i] = i % 160 - 0.5f + 1.f;
            }
            for (int i = 0; i < 6400; i++)
            {
                sub_add[25600 + i] = i % 80 - 0.5f + 1.f;
            }
            for (int i = 0; i < 1600; i++)
            {
                sub_add[32000 + i] = i % 40 - 0.5f + 1.f;
            }

            for (int i = 0; i < 25600; i++)
            {
                sub_add[33600 + i] = i / 160 - 0.5f + 1.f;
            }
            for (int i = 0; i < 6400; i++)
            {
                sub_add[33600 + 25600 + i] = i / 80 - 0.5f + 1.f;
            }
            for (int i = 0; i < 1600; i++)
            {
                sub_add[33600 + 32000 + i] = i / 40 - 0.5f + 1.f;
            }

            // 2次sub and add   此处应该是xyxy2xywh
            std::vector<float> sub_data(33600 * 2);
            std::vector<float> add_data(33600 * 2);
            for (int i = 0; i < 33600 * 2; i++)
            {
                sub_data[i] = sub_add[i] - conv[i];
                add_data[i] = conv[i + 33600 * 2] + sub_add[i];
            }

            std::vector<float> add2_data(33600 * 2);
            std::vector<float> sub2_data(33600 * 2);

            for (int i = 0; i < 33600 * 2; i++)
            {
                add2_data[i] = sub_data[i] + add_data[i];
                sub2_data[i] = add_data[i] - sub_data[i];
            }

            // div concat
            std::vector<float> concat(33600 * 24);
            for (int i = 0; i < 33600 * 2; i++)
            {
                concat[i] = add2_data[i] / 2.f;
                concat[i + 33600 * 2] = sub2_data[i];
            }

            std::vector<float> MUL(33600);

            for (int i = 0; i < 25600; i++)
            {
                MUL[i] = 8;
                if (i < 6400)
                {
                    MUL[i + 25600] = 16;
                }
                if (i < 1600)
                {
                    MUL[i + 32000] = 32;
                }
            }

            std::shared_ptr<glasssix::memory::tensor<float>> output0(new memory::tensor<float>(std::vector<int>{1, 5, 33600}, -1, memory::NCHW));
            // std::vector<float> output(5*33600);
            float* output = output0->mutable_cpu_data();
            for (int i = 0; i < 33600; i++)
            {
                concat[33600 * 0 + i] = concat[33600 * 0 + i] * MUL[i];
                concat[33600 * 1 + i] = concat[33600 * 1 + i] * MUL[i];
                concat[33600 * 2 + i] = concat[33600 * 2 + i] * MUL[i];
                concat[33600 * 3 + i] = concat[33600 * 3 + i] * MUL[i];

                output[33600 * 0 + i] = concat[33600 * 0 + i];
                output[33600 * 1 + i] = concat[33600 * 1 + i];
                output[33600 * 2 + i] = concat[33600 * 2 + i];
                output[33600 * 3 + i] = concat[33600 * 3 + i];
				output[33600 * 4 + i] = sigmoid_x(cat[33600 * 64 + i]);
				//if (i % 100 == 0)
				//	printf("%f,  %f,  %f,  %f | %f\n",
				//		output[33600 * 0 + i], output[33600 * 1 + i], output[33600 * 2 + i], output[33600 * 3 + i], output[33600 * 4 + i]);
            }

            return output0;
        }

        std::vector<std::array<float, 5>> non_max_suppression(std::vector<std::array<float, 5>> pred, float conf_thres, float iou_thres)
        {
            // generate NMS data
            std::vector<cv::Rect2d> boxes;
            std::vector<float> scores;
            for (auto &it : pred)
            {
                cv::Rect2d rect;
                rect.x = it[0] - it[2] / 2;
                rect.y = it[1] - it[3] / 2;
                rect.width = it[2];
                rect.height = it[3];
                boxes.push_back(rect);
                scores.push_back(it[4]);
            }

            // opencv dnn nms
            std::vector<int> indices;
            cv::dnn::NMSBoxes(boxes, scores, conf_thres, iou_thres, indices, 1.f, 1);

            // select indices
            std::vector<std::array<float, 5>> bboxes;

            for (int i = 0; i < indices.size(); i++)
            {
                std::array<float, 5> box;

                box[0] = pred[indices[i]][0] - pred[indices[i]][2] / 2;
                box[1] = pred[indices[i]][1] - pred[indices[i]][3] / 2;

                box[2] = pred[indices[i]][0] + pred[indices[i]][2] / 2;
                box[3] = pred[indices[i]][1] + pred[indices[i]][3] / 2;

                box[4] = pred[indices[i]][4];

                bboxes.push_back(box);
            }

            return bboxes;
        }

        std::vector<std::vector<float>> post_process(std::shared_ptr<memory::tensor<float>> &net_result, cv::Mat &blob,
                                                     int pad_h, int pad_w, float scale, float threshold = 0.5, float iou_thres = 0.6)
        {
            std::vector<std::vector<float>> output;

            int dim_2 = net_result->count() / 33600; //5
            std::shared_ptr<glasssix::memory::tensor<float>> dest(new glasssix::memory::tensor<float>(33600, dim_2, -1, glasssix::memory::NCHW, nullptr));

            tranpose(net_result, dest);

            const float *dest_ptr = dest->cpu_data();

            std::vector<cv::Rect2d> xywh_boxes;

            std::vector<std::vector<float>> key_points;

            std::vector<float> scores;
            std::vector<int> indices_body; // 候选框顺序

            int count = 0;
            for (int i = 0; i < 33600; i++)
            {
                if (dest_ptr[dim_2 * i + 4] > 0.450)
                {
                    count++;
                    indices_body.push_back(i);

                    cv::Rect2d boxwh;
                    boxwh.x = static_cast<double>(dest_ptr[dim_2 * i] - dest_ptr[dim_2 * i + 2] / 2);
                    boxwh.y = static_cast<double>(dest_ptr[dim_2 * i + 1] - dest_ptr[dim_2 * i + 3] / 2);
                    boxwh.width = static_cast<double>(dest_ptr[dim_2 * i + 2]);
                    boxwh.height = static_cast<double>(dest_ptr[dim_2 * i + 3]);

                    {
                        xywh_boxes.push_back(boxwh);
                        scores.push_back(dest_ptr[dim_2 * i + 4]);
                        indices_body.push_back(i);
                    }
                }
            }


            std::vector<int> indices_body_copy(indices_body.size());

            for (int i = 0; i < indices_body_copy.size(); i++)
            {
                indices_body_copy[i] = i;
            }

            cv::dnn::NMSBoxes(xywh_boxes, scores, threshold, iou_thres, indices_body_copy, 1.f, 0);

            // cv::imwrite("../preocess.jpg",blob);

            for (int i = 0; i < indices_body_copy.size(); i++)
            {
                int index = indices_body_copy[i];
                std::vector<float> temp_output(5);

                temp_output[0] = (xywh_boxes[index].x - pad_w) * scale;
                temp_output[1] = (xywh_boxes[index].y - pad_h) * scale;
                temp_output[2] = (xywh_boxes[index].width + xywh_boxes[index].x - pad_w) * scale;
                temp_output[3] = (xywh_boxes[index].height + xywh_boxes[index].y - pad_h) * scale;
                temp_output[4] = scores[index];

                output.emplace_back(temp_output);
            }

            // cv::imwrite("../preocesdss.jpg",blob);

            int k = 0;

            return output;
        }

        std::tuple<cv::Mat, float> preprocess_detection(cv::Mat src, int &pad_h, int &pad_w, cv::Size input_shape = cv::Size(640, 640))
        {
            float scale = std::min((float)input_shape.width / (float)src.cols, (float)input_shape.height / (float)src.rows);
            cv::Mat cut_image;
            cv::Mat mask_image(input_shape, CV_8UC3, cv::Scalar(114, 114, 114));
            if (src.rows != input_shape.height || src.cols != input_shape.width)
            {
                cv::resize(src, cut_image, cv::Size((int)(src.cols * scale), (int)(src.rows * scale)), cv::INTER_LINEAR);

                pad_h = int((input_shape.height - cut_image.rows) / 2);
                pad_w = int((input_shape.width - cut_image.cols) / 2);
                cv::copyMakeBorder(cut_image, mask_image, pad_h, input_shape.height - cut_image.rows - pad_h, pad_w, input_shape.width - cut_image.cols - pad_w, cv::BORDER_CONSTANT, cv::Scalar{114, 114, 114});
            }
            else
            {
                src.copyTo(mask_image);
            }
            cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);
            return {mask_image, scale};
        }

        std::vector<pedestrian::box_info_internal> universal_pedestrian_detect(cv::Mat &image, float con_thres, float iou_thres, bool is_wander_call)
        {
            std::vector<box_info_internal> output;
            float conf_threshold = con_thres;
            float iou_threshold = iou_thres;

            if (is_wander_call)
            {
                cv::Mat hsv_image;
                cv::Mat red_mask;
                cv::Mat blue_mask;

                cv::cvtColor(image, hsv_image, cv::COLOR_BGR2HSV);

                cv::Scalar lower_blue = cv::Scalar{ 110, 50, 50 };
                cv::Scalar upper_blue = cv::Scalar{ 115, 255, 255 };

                cv::Scalar lower_red = cv::Scalar{ 0, 50, 50 };
                cv::Scalar upper_red = cv::Scalar{ 5, 200, 200 };
                cv::inRange(hsv_image, lower_red, upper_red, red_mask);
                cv::inRange(hsv_image, lower_blue, upper_blue, blue_mask);

                for (int row = 0; row < red_mask.rows; ++row)
                {
                    for (int col = 0; col < red_mask.cols; ++col)
                    {
                        auto& hsv = hsv_image.at<cv::Vec3b>(row, col);
                        if (red_mask.at<uchar>(row, col) > 0 || blue_mask.at<uchar>(row, col) > 0)
                        {
                            hsv = { 120, 255, 255 };
                        }
                    }
                }

                cv::cvtColor(hsv_image, image, cv::COLOR_HSV2BGR);
            }

            auto new_shape = cv::Size(1280, 1280);
            cv::Mat blob;
            float ratio = 0;
            int pad_h = 0;
            int pad_w = 0;
            std::tie(blob, ratio) = preprocess_detection(image, pad_h, pad_w, new_shape);
            //std::map<std::string, std::shared_ptr<memory::tensor<float>>> forwards;

            unsigned char *blobdata = blob.ptr<uchar>();

            // auto network_result = net_detect_.forward(blob.data, {1, blob.rows, blob.cols, blob.channels()}, RKNN_TENSOR_NHWC);

            auto network_results = net_detect_.forward(blob.data, {1, blob.rows, blob.cols, blob.channels()}, RKNN_TENSOR_NHWC);

            std::vector<std::string> out_names = {"355", "340", "output0"};

            std::vector<std::shared_ptr<memory::tensor<float>>> forwards;

            for (size_t i = 0; i < out_names.size(); i++) // 对输出数据做处理
            {
                forwards.push_back(network_results[out_names[i]]);
            }

            // std::shared_ptr<memory::tensor<float>> real_output = yolov8s_complement(network_results);

            auto real_output = Concat(forwards, conf_threshold); // 5*longline

            //auto nms_result = post_process(real_output, blob, pad_h, pad_w, 1.f / ratio);

            auto nms_result = post_process(real_output, blob, pad_h, pad_w, 1.f / ratio, conf_threshold, iou_threshold);

            for (auto &head : nms_result)
            {
                int x1 = std::round(head[0]) > 0 ? std::round(head[0]) : 0;
                int y1 = std::round(head[1]) > 0 ? std::round(head[1]) : 0;
                int x2 = std::round(head[2]) < image.cols ? std::round(head[2]) : image.cols;
                int y2 = std::round(head[3]) < image.rows ? std::round(head[3]) : image.rows;

                //std::cout << x1 << " " << x2 << " " << y1 << " " << y2 << " " << head[4] << std::endl;
                cv::rectangle(image, cv::Point(x1, y1), cv::Point(x2, y2),
                              cv::Scalar(0, 255, 0), 2);

                if ((y2 - y1) < 0 || (x2 - x1) < 0)
                {
                    continue;
                }
                cv::Mat crop = image(cv::Range(y1, y2), cv::Range(x1, x2));
                cv::Mat headimg;
                cv::resize(crop, headimg, cv::Size((int)(96), (int)(96)), cv::INTER_LINEAR);

                box_info_internal headp;
                headp.x1 = x1;
                headp.x2 = x2;
                headp.y1 = y1;
                headp.y2 = y2;
                headp.score=  head[4]; 

                output.push_back(headp);
            }
            // std::cout<<output.size()<<std::endl;
            return output;
            // std::cout<<output[0].x1<<std::endl;
            // std::cout<<output[0].y1<<std::endl;
        }

    private:
        std::string model_directory_;
        int device_;
        glasssix::rknnwrapper::rknn_wrapper net_detect_;
        // glasssix::rknnwrapper::rknn_wrapper net_class_;
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

    exposing::param_vector<pedestrian::box_info> classify_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }
}