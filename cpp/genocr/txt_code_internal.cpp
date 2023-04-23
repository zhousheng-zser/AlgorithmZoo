#include "txt_code_internal.hpp"
#include "hardcode.hpp"

#include <fstream>
#include <algorithm>
#include "box_info_impl.hpp"
#include <Excalibur/pipeline.hpp>
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include "Primitives/tensor_conversions.hpp"
#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_resize.hpp"
#include "Excalibur/operation_rgb2gray.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/opencv.hpp>

#include <cfloat>
#include <numeric>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

namespace glasssix::genocr
{
    std::array<std::tuple<int, std::string, std::string, std::string, std::string>, 2> types =
    {
        {
            {0, "english_ocr_det", "english_ocr_angle", "english_ocr_rec", ""},
            {1, "chinese_ocr_det", "chinese_ocr_cls", "chinese_ocr_rec", ""}
        }
    };

    class txt_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, std::string_view chardic_directory, int factory_type, int device, std::map<std::string, float>& param_map)
            : factory_type_(factory_type), device_{device}, param_map_{ param_map }
        {
            auto factory = std::find_if(types.begin(), types.end(), [factory_type](const std::tuple<int, std::string, std::string, std::string, std::string>& t)
                { return std::get<0>(t) == factory_type; });

            if (factory == types.end())
                throw exposing::abi_invalid_argument("Invalid factory_tpye param!");

            std::string charDicPath;
            if (factory_type == 0)
            {
                charDicPath = std::string(chardic_directory) + "/" +"english_dic.txt";
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<1>(*factory)), std::string(model_directory) + "/" + std::get<1>(*factory) + ".racy", device));
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<2>(*factory)), std::string(model_directory) + "/" + std::get<2>(*factory) + ".racy", device));
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<3>(*factory)), std::string(model_directory) + "/" + std::get<3>(*factory) + ".racy", device));
            }
            else if(factory_type == 1)
            {
                charDicPath = std::string(chardic_directory) + "/" +"chinese_dic.txt";
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<1>(*factory)), std::string(model_directory) + "/" + std::get<1>(*factory) + ".racy", device));
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<2>(*factory)), std::string(model_directory) + "/" + std::get<2>(*factory) + ".racy", device));
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<3>(*factory)), std::string(model_directory) + "/" + std::get<3>(*factory) + ".racy", device));
            }

            char_dictionary_.push_back("blank");
            std::ifstream in(charDicPath.c_str());
            std::string line;
            if (in) {
                while (getline(in, line)) {/*line中不包括每行的换行符*/
                    char_dictionary_.push_back(line);
                }
            }
            else {
                if (factory_type == 0)
                    throw exposing::abi_invalid_operation("file 'english_dic.txt' was not found!");
                else if (factory_type == 1)
                    throw exposing::abi_invalid_operation("file 'chinese_dic.txt' was not found!");
            }
        }

        exposing::param_vector<box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int top_five, int order, int x, int y, int roi_width, int roi_height)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            // roi params
            std::vector<int> roi{x, y, roi_height, roi_width};
            init_cache(bitmap, channels, height, width, order, roi);

            std::vector<box_info_internal> results;
            auto result = exposing::make_param_vector<box_info>();
            if (factory_type_ == 0)
                run_gen_english_ocr(results, roi, top_five);
            else if (factory_type_ == 1)
                run_gen_chinese_ocr(results, roi, top_five);
            else
                return result;

            for (auto &i : results)
            {
                result.push_back(exposing::make_as_first<box_info_impl>(i));
            }
            return result;
        }

        static std::string version()
        {
            return "1.0.0_2023.03.17";
        }

    private:
        void run_gen_english_ocr(std::vector<box_info_internal>& results, std::vector<int>& roi, int top_five)
        {
            excalibur::rectangle<int> rect((int)roi[0], (int)roi[1], (int)roi[2], (int)roi[3]);
            std::shared_ptr<memory::tensor<uint8_t>> input;
            // image preprocessing
            excalibur::safty_cut_cpu(cache_, input, &rect);
            cv::Mat input_mat(roi[2], roi[3], CV_8UC3);
            std::copy(input->cpu_data(), input->cpu_data() + input->count(1, 4), input_mat.data);
            if (input->order() == memory::NHWC)
                input->convert_order();
            auto [resized_img, ratio] = en_resize_fixed_size(640, input);

            // ocr detect
            std::pair<std::vector<std::vector<cv::Point2f>>, std::vector<float>> result = det_combine_best(resized_img, *instance_[0], 1.6, 0.4);
            std::vector<std::vector<cv::Point2f>> box_list = result.first;
            for (size_t i = 0; i < box_list.size(); i++)
            {
                for (size_t j = 0; j < box_list[i].size(); j++)
                {
                    box_list[i][j] *= ratio;
                }
            }
            #ifdef BUILD_DEBUG_INFO
            //auto visual_mat = input_mat.clone();
            //for (auto& box : box_list)
            //    for (auto& point : box)
            //        cv::circle(visual_mat, cv::Point(point.x, point.y), 3, cv::Scalar(0, 0, 255), 3);
            //cv::imshow("visual_mat", visual_mat); cv::waitKey(0);
            #endif

            for (size_t i = 0; i < box_list.size(); ++i) {
                bool rotate = false;
                cv::RotatedRect rect = cv::minAreaRect(box_list[i]);
                // crop img
                cv::Mat cut_img = crop_rect(input_mat, rect);
                int newH = cut_img.rows;
                int newW = cut_img.cols;
                // ignore
                if (std::max(newH, newW) / (std::min(newH, newW) * 1.0) <= 1.5)
                {
                    continue;
                }
                if (newH > newW)
                {
                    cut_img = rotateAntiClockWise90(cut_img);
                    rotate = true;
                }
                bool inverse = false;
                std::vector<float> res_vec = angel_infer(32, 320, cut_img, *instance_[1]);
                if (res_vec[0] == 0)
                {
                    cv::flip(cut_img, cut_img, -1);
                    inverse = true;
                }

                auto out = rec_combine_best(cut_img, top_five, *instance_[2]);

                box_info_internal box;
                auto location = exposing::make_param_vector<float>();
                for (int j = 0; j < box_list[i].size(); ++j)
                {
                    location.push_back(box_list[i][j].x);
                    location.push_back(box_list[i][j].y);
                }
                box.location = location;
                auto strinfos = exposing::make_param_vector<exposing::param_string>();
                for (int j = 0; j < out.first.size(); ++j)
                {
                    strinfos.push_back(exposing::param_string(out.first[j]));
                }
                box.strinfos = strinfos;
                // process angle -> [0, 360)
                float angle;
                if (!rotate && !inverse)
                {
                    angle = std::abs(rect.angle);
                }
                else if (rotate && inverse)
                {
                    angle = std::abs(rect.angle) + 90;
                }
                else if (!rotate && inverse)
                {
                    angle = std::abs(rect.angle) + 180;
                }
                else if (rotate && !inverse)
                {
                    angle = std::abs(rect.angle) + 270;
                    angle = angle == 360 ? 0 : angle;
                }
                box.angle = angle;
                results.push_back(box);
            }
        }

        void run_gen_chinese_ocr(std::vector<box_info_internal>& results, std::vector<int>& roi, int top_five)
        {
            excalibur::rectangle<int> rect((int)roi[0], (int)roi[1], (int)roi[2], (int)roi[3]);
            std::shared_ptr<memory::tensor<uint8_t>> input;
            // image preprocessing
            excalibur::safty_cut_cpu(cache_, input, &rect);
            cv::Mat input_mat(roi[2], roi[3], CV_8UC3);
            std::copy(input->cpu_data(), input->cpu_data() + input->count(1, 4), input_mat.data);
            if (input->order() == memory::NHWC)
                input->convert_order();
            auto [resized_img, ratio_pair] = cn_resize_fixed_size(640, input);
            auto [ratio_w, ratio_h] = ratio_pair;

            // ocr detect
            std::pair<std::vector<std::vector<cv::Point2f>>, std::vector<float>> result = det_combine_best(resized_img, *instance_[0], 1.5, 0.6);
            std::vector<std::vector<cv::Point2f>> box_list = result.first;
            for (size_t i = 0; i < box_list.size(); i++)
            {
                for (size_t j = 0; j < box_list[i].size(); j++)
                {
                    box_list[i][j].x *= ratio_w;
                    box_list[i][j].y *= ratio_h;
                }
            }

            #ifdef BUILD_DEBUG_INFO
            //auto visual_mat = input_mat.clone();
            //for (auto& box : box_list)
            //    for (auto& point : box)
            //        cv::circle(visual_mat, cv::Point(point.x, point.y), 3, cv::Scalar(0, 0, 255), 3);
            //cv::imshow("visual_mat", visual_mat); cv::waitKey(0);
            #endif

            for (size_t i = 0; i < box_list.size(); ++i) {
                bool rotate = false;
                cv::RotatedRect rect = cv::minAreaRect(box_list[i]);
                // crop img
                cv::Mat cut_img = crop_rect(input_mat, rect);
                int newH = cut_img.rows;
                int newW = cut_img.cols;
                // ignore
                if (std::max(newH, newW) / (std::min(newH, newW) * 1.0) <= 1.5)
                {
                    continue;
                }
                if (newH > newW)
                {
                    cut_img = rotateAntiClockWise90(cut_img);
                    rotate = true;
                }
                bool inverse = false;
                std::vector<float> res_vec = angel_infer(48, 192, cut_img, *instance_[1]);
                if (res_vec[0] == 0)
                {
                    cv::flip(cut_img, cut_img, -1);
                    inverse = true;
                }

                auto out = rec_combine_best(cut_img, top_five, *instance_[2]);

                box_info_internal box;
                auto location = exposing::make_param_vector<float>();
                for (int j = 0; j < box_list[i].size(); ++j)
                {
                    location.push_back(box_list[i][j].x);
                    location.push_back(box_list[i][j].y);
                }
                box.location = location;
                auto strinfos = exposing::make_param_vector<exposing::param_string>();
                for (int j = 0; j < out.first.size(); ++j)
                {
                    strinfos.push_back(exposing::param_string(out.first[j]));
                }
                box.strinfos = strinfos;
                // process angle -> [0, 360)
                float angle;
                if (!rotate && !inverse)
                {
                    angle = std::abs(rect.angle);
                }
                else if (rotate && inverse)
                {
                    angle = std::abs(rect.angle) + 90;
                }
                else if (!rotate && inverse)
                {
                    angle = std::abs(rect.angle) + 180;
                }
                else if (rotate && !inverse)
                {
                    angle = std::abs(rect.angle) + 270;
                    angle = angle == 360 ? 0 : angle;
                }
                box.angle = angle;
                results.push_back(box);
            }
        }

        void init_cache(exposing::param_span<std::uint8_t> &bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order, std::vector<int> &roi)
        {
            if (cache_ == nullptr || cache_->channels() != channels || cache_->height() != height || cache_->width() != width || cache_->order() != order)
            {
                std::vector<int> shape;
                if (order == memory::NCHW)
                    shape = {static_cast<int>(1), channels, height, width};
                else if (order == memory::NHWC)
                    shape = {static_cast<int>(1), height, width, channels};
                else
                    NOT_IMPLEMENTED;

                cache_ = std::make_shared<memory::tensor<std::uint8_t>>(shape, -1, (memory::orderType)order /*, &memory::pool_allocator_default<std::uint8_t>::get()*/);
            }

            if (cache_->device() > 0)
            {
#ifdef USE_CUDA
                cudaMemcpy(cache_->mutable_gpu_data(), bitmap, channels * height * width, cudaMemcpyHostToDevice);
#else
                NO_GPU;
#endif
            }
            else
                std::copy(bitmap.begin(), bitmap.end(), cache_->mutable_cpu_data());

            //if (order == memory::NHWC)
            //    cache_->convert_order();
        }

        void softmax_along_width(std::shared_ptr<memory::tensor<float>> &input, int dim)
        {
            if (dim == 2)
            {
                int height = input->height();
                int width = input->width();
                for (int h = 0; h < height; ++h)
                {
                    float sum = 0.0f;
                    std::vector<float> y(width);
                    float* input_data = input->mutable_cpu_data() + input->offset(0, 0, h);
                    for (int i = 0; i < width; ++i)
                    {
                        sum += y[i] = std::exp(input_data[i]);
                    }
                    for (int i = 0; i < width; ++i)
                    {
                        input_data[i] = y[i] / sum;
                    }
                }
            }
            else
            {
                int count = input->count();
                float sum = 0.f;
                std::vector<float> y(count);
                float* input_data = input->mutable_cpu_data();
                for (int i = 0; i < count; i++)
                {
                    sum += y[i] = std::exp(input_data[i]);
                }
                for (int i = 0; i < count; ++i)
                {
                    input_data[i] = y[i] / sum;
                }
            }
        }

        // AntiClockWise 90
        cv::Mat rotateAntiClockWise90(cv::Mat src)
        {
            cv::transpose(src, src);
            cv::flip(src, src, 0);
            return src;
        }

        std::pair<std::shared_ptr<memory::tensor<uint8_t>>, float> en_resize_fixed_size(float dst_size, std::shared_ptr<memory::tensor<uint8_t>> &img)
        {
            std::shared_ptr<memory::tensor<uint8_t>> dst;
            float ratio_y = dst_size / (float)img->height();
            float ratio_x = dst_size / (float)img->width();
            std::pair<float, bool> ratio;
            if (ratio_y < ratio_x) {
                ratio.first = ratio_y;
                ratio.second = false;
            }
            else {
                ratio.first = ratio_x;
                ratio.second = true;
            }
            int aligned_w = ratio.first * img->width();
            int aligned_h = ratio.first * img->height();
            excalibur::resize_cpu(img, dst, aligned_h, aligned_w);
            if (ratio.second) { //padding bottom
                if (img->height() < dst_size)
                    excalibur::make_border(dst, dst, 0, dst_size - dst->height(), 0, 0);
            }
            else { //padding right
                if (img->width() < dst_size)
                    excalibur::make_border(dst, dst, 0, 0, 0, dst_size - dst->width());
            }

            return { dst, 1.0f / ratio.first };
        }

        std::pair<std::shared_ptr<memory::tensor<uint8_t>>, std::pair<float, float>> cn_resize_fixed_size(float short_size, std::shared_ptr<memory::tensor<uint8_t>>& img)
        {
            int w = img->width();
            int h = img->height();
            float ratio = 1;
            int resized_w = w, resized_h = h;
            int aligned_w = 0, aligned_h = 0;
            std::shared_ptr<memory::tensor<uint8_t>> dst;

            if (std::min(w, h) < short_size)
            {
                ratio = w < h ? (short_size / w) : (short_size / h);
                resized_w = int(w * ratio);
                resized_h = int(h * ratio);
            }
            aligned_w = std::max((int)std::round((float)resized_w / 32) * 32, 32);
            aligned_h = std::max((int)std::round((float)resized_h / 32) * 32, 32);
            excalibur::resize_cpu(img, dst, aligned_h, aligned_w);

            float ratio_w = (float)w / (float)aligned_w;
            float ratio_h = (float)h / (float)aligned_h;

            return { dst,{ratio_w, ratio_h} };
        }

        void expand_polygon(const std::vector<cv::Point2f> &pList, std::vector<cv::Point2f> &out, float SAFELINE)
        {
            // edge set and normalize it
            std::vector<cv::Point2f> dpList, ndpList;
            int count = pList.size();
            for (int i = 0; i < count; i++)
            {
                int next = (i == (count - 1) ? 0 : (i + 1));
                dpList.push_back(pList.at(next) - pList.at(i));
                float unitLen = 1.0f / sqrt(dpList.at(i).dot(dpList.at(i)));
                ndpList.push_back(dpList.at(i) * unitLen);
            }

            // compute Line
            for (int i = 0; i < count; i++)
            {
                int startIndex = (i == 0 ? (count - 1) : (i - 1));
                int endIndex = i;
                float sinTheta = ndpList.at(startIndex).cross(ndpList.at(endIndex));
                cv::Point2f orientVector = ndpList.at(endIndex) - ndpList.at(startIndex); //i.e. PV2-V1P=PV2+PV1
                cv::Point2f temp_out;
                temp_out.x = pList.at(i).x + SAFELINE / sinTheta * orientVector.x;
                temp_out.y = pList.at(i).y + SAFELINE / sinTheta * orientVector.y;
                out.push_back(temp_out);
            }
        }

        bool cmp(const cv::Point2f &a, const cv::Point2f &b)
        {
            return a.x > b.x;
        }

        float get_value(std::vector<cv::Point2f> &box, bool is_min, int index)
        {
            int size = box.size();
            float min_value;
            float max_value;
            if (index == 0)
            {
                min_value = box[0].x;
                max_value = box[0].x;
                for (int i = 1; i < size; ++i)
                {
                    if (min_value > box[i].x)
                    {
                        min_value = box[i].x;
                    }
                    if (max_value < box[i].x)
                    {
                        max_value = box[i].x;
                    }
                }
            }
            else if (index == 1)
            {
                min_value = box[0].y;
                max_value = box[0].y;
                for (int i = 1; i < size; ++i)
                {
                    if (min_value > box[i].y)
                    {
                        min_value = box[i].y;
                    }
                    if (max_value < box[i].y)
                    {
                        max_value = box[i].y;
                    }
                }
            }
            return (is_min ? min_value : max_value);
        }

        template <typename T>
        void get_mini_boxes(std::vector<T> &contour, std::vector<cv::Point2f> &box, float &sside)
        {
            cv::RotatedRect bounding_box = cv::minAreaRect(contour);
            cv::Point2f points[4];
            bounding_box.points(points);

            for (int i = 0; i < 4; ++i)
            {
                box.push_back(points[i]);
            }
            sside = std::min(bounding_box.size.height, bounding_box.size.width);
        }

        float box_score_fast(cv::Mat &bitmap, std::vector<cv::Point2f> box)
        {
            int h = bitmap.rows;
            int w = bitmap.cols;
            int xmin = std::min(std::max((int)std::floor(get_value(box, true, 0)), 0), w - 1);
            int xmax = std::min(std::max((int)std::ceil(get_value(box, false, 0)), 0), w - 1);

            int ymin = std::min(std::max((int)std::floor(get_value(box, true, 1)), 0), h - 1);
            int ymax = std::min(std::max((int)std::ceil(get_value(box, false, 1)), 0), h - 1);

            cv::Mat mask(ymax - ymin + 1, xmax - xmin + 1, CV_8UC1);
            memset(mask.data, 0, mask.total());

            // fill any polygon
            for (int i = 0; i < box.size(); ++i)
            {
                box[i].x = box[i].x - xmin;
                box[i].y = box[i].y - ymin;
            }
            std::vector<std::vector<cv::Point>> ppt;
            std::vector<cv::Point> box_tmp;
            for (int i = 0; i < box.size(); ++i)
            {
                box_tmp.emplace_back(static_cast<int>(box[i].x), static_cast<int>(box[i].y));
            }
            ppt.push_back(box_tmp);
            cv::fillPoly(mask, ppt, cv::Scalar(1));
            float result = cv::mean(bitmap(cv::Rect(xmin, ymin, xmax + 1 - xmin, ymax + 1 - ymin)), mask)[0];
            return result;
        }

        float box_score_slow(cv::Mat &bitmap, std::vector<cv::Point> &box)
        {
            // int h = bitmap.rows;
            // int w = bitmap.cols;
            // int xmin = std::min(std::max((int)get_value(box, true, 0), 0), w - 1);
            // int xmax = std::min(std::max((int)get_value(box, false, 0), 0), w - 1);

            // int ymin = std::min(std::max((int)get_value(box, true, 1), 0), h - 1);
            // int ymax = std::min(std::max((int)get_value(box, false, 1), 0), h - 1);

            // cv::Mat mask(xmax - xmin + 1, xmax - xmin + 1, CV_8UC1);
            // memset(mask.data, 0, (xmax - xmin + 1) * (xmax - xmin + 1));
            // for (int i = 0; i < box.size(); ++i)
            // {
            //     box[i].x = box[i].x - xmin;
            //     box[i].y = box[i].y - ymin;
            // }
            // // fill any polygon
            // cv::fillPoly(mask, box, 1);
            // cv::Mat bitmap_(bitmap, cv::Rect(xmin, ymin, xmax + 1 - xmin, ymax + 1 - ymin));
            // float result = cv::mean(bitmap_, mask)[0];
            // return result;
            return 0.f;
        }

        // Outline of the contract
        std::vector<cv::Point2f> unclip(std::vector<cv::Point2f> &box, float unclip_ratio = 1.5)
        {
            float distance = cv::contourArea(box) * (param_map_.count("unclip_ratio") ? param_map_["unclip_ratio"] : unclip_ratio) / cv::arcLength(box, true);
            // contour shrinkage
            std::vector<cv::Point2f> out;
            expand_polygon(box, out, distance * (-1));
            return out;
        }

        void boxes_from_bitmap(cv::Mat &out, cv::Mat &mask, int src_w, int src_h, std::vector<std::vector<cv::Point2f>> &boxes, std::vector<float> &scores, float unclip_ratio = 1.5, float box_thresh_ = 0.5)
        {
            size_t max_candidates = 1000;
            int min_size = 3;
            float box_thresh = param_map_.count("box_thresh") ? param_map_["box_thresh"] : box_thresh_;
            message_box_thresh_ = box_thresh;
            std::string score_mode = "fast";
            int width = mask.cols;
            int height = mask.rows;
            // detect contour
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(mask, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
            size_t num_contours = std::min(contours.size(), max_candidates);
            for (size_t i = 0; i < num_contours; ++i)
            {
                std::vector<cv::Point> contour = contours[i];
                // get min rect of per contour
                std::vector<cv::Point2f> points;
                float sside;
                get_mini_boxes(contour, points, sside);
                if (sside < min_size)
                {
                    continue;
                }
                float score;
                if (score_mode == "fast")
                {
                    score = box_score_fast(out, points);
                }
                else
                {
                    score = box_score_slow(out, contour);
                }
                if (score < box_thresh)
                {
                    continue;
                }
                std::vector<cv::Point2f> box = unclip(points, unclip_ratio);
                // sside: minimum between width and height of external retangel
                points.clear();
                get_mini_boxes(box, points, sside);
                if (sside < min_size + 2)
                {
                    continue;
                }
                for (int i = 0; i < points.size(); ++i)
                {
                    points[i].x = std::round(std::min(std::max(points[i].x / width * src_w, 0.f), (const float)src_w));
                    points[i].y = std::round(std::min(std::max(points[i].y / height * src_h, 0.f), (const float)src_h));
                }
                // boxes.insert(boxes.end(), points.begin(), points.end());
                boxes.push_back(points);
                scores.push_back(score);
            }
        }

        void det_post_process(std::shared_ptr<memory::tensor<float>> &out_, std::vector<std::vector<cv::Point2f>> &boxes, std::vector<float> &scores, float unclip_ratio = 1.5, float box_thresh = 0.5)
        {
            cv::Mat out(out_->height(), out_->width(), CV_32FC1);
            memcpy(out.data, out_->cpu_data(), out_->count(2, 4) * sizeof(float));
            int src_w = out.cols;
            int src_h = out.rows;
            float thresh = 0.3;
            message_det_thresh_ = thresh;
            int count = out_->count(2, 4);
            const float *out_data = out_->cpu_data();
            cv::Mat mask(out_->height(), out_->width(), CV_8UC1);
            std::uint8_t *mask_data = mask.data;
            for (int i = 0; i < count; ++i)
            {
                mask_data[i] = (out_data[i] > thresh ? 1 : 0) * 255;
            }
            boxes_from_bitmap(out, mask, src_w, src_h, boxes, scores, unclip_ratio, box_thresh);
        }

        std::pair<std::vector<std::vector<cv::Point2f>>, std::vector<float>> det_combine_best(std::shared_ptr<memory::tensor<uint8_t>> &input, excalibur::pipeline<float>& det_instance_, float unclip_ratio = 1.5, float box_thresh = 0.5)
        {
            auto input_tensor = input | memory::tensor_convert_to<float>;
            // pre process
            //det_preprocess(input_tensor);

            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> out = det_instance_.forward(input_tensor);

            std::shared_ptr<memory::tensor<float>> output = out["output"];


            std::vector<std::vector<cv::Point2f>> boxes;
            std::vector<float> scores;

            det_post_process(output, boxes, scores, unclip_ratio, box_thresh);
            
            return std::make_pair(boxes, scores);
        }

        std::pair<std::vector<std::string>, std::vector<std::vector<float>>> decode(std::vector<std::vector<int>> &idxs, std::vector<std::vector<float>> &probs, const std::vector<std::string> &character, int top_five, bool remove_duplicate = true)
        {
            int K = 1;
            if (top_five)
            {
                K = 5;
            }
            std::vector<std::string> strinfos(K);
            std::vector<std::vector<float>> probs_list(K);
            for (int i = 0; i < K; ++i)
            {
                for (int j = 0; j < idxs.size(); ++j)
                {
                    if (idxs[j][i] == 0)
                    {
                        continue;
                    }
                    // remove duplicate characters from the string
                    if (remove_duplicate && j > 0 && idxs[j - 1][i] == idxs[j][i])
                    {
                        continue;
                    }
                    strinfos[i].append(character[idxs[j][i]]);
                    probs_list[i].push_back(probs[j][i]);
                }
            }
            return std::make_pair(strinfos, probs_list);
        }

        std::pair<std::vector<std::string>, std::vector<std::vector<float>>> rec_combine_postprocess(std::shared_ptr<memory::tensor<float>> &result, int top_five)
        {
            int K = 1;
            if (top_five)
            {
                K = 5;
            }
            softmax_along_width(result, 2);
            int height = result->height();
            int width = result->width();
            float *out_data = result->mutable_cpu_data();
            std::vector<std::vector<int>> idxs(height);
            std::vector<std::vector<float>> probs(height);
            int index = 0;
            float max_val = 0;
            for (int i = 0; i < height; ++i)
            {
                for (int n = 0; n < K; ++n)
                {
                    for (int j = 0; j < width; ++j)
                    {
                        float value = out_data[i * width + j];
                        if (max_val < value)
                        {
                            max_val = value;
                            index = j;
                        }
                    }
                    idxs[i].push_back(index);
                    probs[i].push_back(max_val);
                    out_data[i * width + index] = 0;
                    index = 0;
                    max_val = 0;
                }
            }

            // decode char
            return decode(idxs, probs, char_dictionary_, top_five);
        }

        std::pair<std::vector<std::string>, std::vector<std::vector<float>>> rec_combine_best(cv::Mat &cut_img, int top_five, excalibur::pipeline<float>& rec_instance_)
        {
            int resize_H = 32;
            int resize_W = 320;
            float ratio_y = float(resize_H) / cut_img.rows;
            float ratio_x = float(resize_W) / cut_img.cols;
            std::pair<float, bool> ratio;
            if (ratio_y < ratio_x) {
                ratio.first = ratio_y;
                ratio.second = false;
            }
            else {
                ratio.first = ratio_x;
                ratio.second = true;
            }
            cv::resize(cut_img, cut_img, cv::Size(0, 0), ratio.first, ratio.first, cv::INTER_LINEAR);

            if (ratio.second) { //padding bottom
                if (cut_img.rows < resize_H)
                    cv::copyMakeBorder(cut_img, cut_img, 0, resize_H - cut_img.rows, 0, 0, cv::BORDER_CONSTANT);
            }
            else { //padding right
                if (cut_img.cols < resize_W)
                    cv::copyMakeBorder(cut_img, cut_img, 0, 0, 0, resize_W - cut_img.cols, cv::BORDER_CONSTANT);
            }
            //cv::imshow("input_tensor", cut_img); cv::waitKey(0);

            std::shared_ptr<memory::tensor<uint8_t>> input(new memory::tensor<uint8_t>(cut_img.channels(), cut_img.rows, cut_img.cols, -1, memory::NHWC, nullptr));
            std::copy(cut_img.data, cut_img.data + cut_img.step[0] * cut_img.rows, input->mutable_cpu_data());
            input->convert_order();
            auto input_tensor = input | memory::tensor_convert_to<float>;
            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> out = rec_instance_.forward(input_tensor);
            std::shared_ptr<memory::tensor<float>> result = out["output"];
            // post process
            return rec_combine_postprocess(result, top_five);
        }

        std::vector<float> angel_postprocess(std::shared_ptr<memory::tensor<float>> &result)
        {
            softmax_along_width(result, 1);
            int idx = 0;
            float prob = 0;
            const float *res_data = result->cpu_data();
            for (int i = 0; i < result->count(); ++i)
            {
                if (res_data[i] > prob)
                {
                    idx = i;
                    prob = res_data[i];
                }
            }
            return std::vector<float>{(float)idx, prob};
        }

        std::vector<float> angel_infer(int resize_H, int resize_W, const cv::Mat &img, excalibur::pipeline<float>& angle_instance_)
        {
            cv::Mat cut_img = img.clone();
            float ratio_y = float(resize_H) / cut_img.rows;
            float ratio_x = float(resize_W) / cut_img.cols;
            std::pair<float,bool> ratio;
            if (ratio_y < ratio_x) {
                ratio.first = ratio_y;
                ratio.second = false;
            }
            else {
                ratio.first = ratio_x;
                ratio.second = true;
            }
            cv::resize(cut_img, cut_img, cv::Size(0, 0), ratio.first, ratio.first, cv::INTER_LINEAR);

            //cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 127,127,127 });
            if (ratio.second) { //padding bottom
                if (cut_img.rows < resize_H)
					cv::copyMakeBorder(cut_img, cut_img, 0, resize_H - cut_img.rows, 0, 0, cv::BORDER_CONSTANT);
            }
            else { //padding right
                if (cut_img.cols < resize_W)
                    cv::copyMakeBorder(cut_img, cut_img, 0, 0, 0, resize_W - cut_img.cols, cv::BORDER_CONSTANT);
            }
            //cv::imshow("angel_infer", cut_img); cv::waitKey(0);

            // convert from mat to tensor
            std::shared_ptr<memory::tensor<uint8_t>> input(new memory::tensor<uint8_t>(cut_img.channels(), cut_img.rows, cut_img.cols, -1, memory::NHWC, nullptr));
            std::copy(cut_img.data, cut_img.data + cut_img.step[0] * cut_img.rows, input->mutable_cpu_data());
            input->convert_order();

            auto input_tensor = input | memory::tensor_convert_to<float>;
            //// pre process
            //int count = input_tensor->count();
            //float *input_data = input_tensor->mutable_cpu_data();
            //for (int i = 0; i < count; ++i)
            //{
            //    input_data[i] = (input_data[i] / 255.f - 0.5) / 0.5;
            //}
            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> out = angle_instance_.forward(input_tensor);
            std::shared_ptr<memory::tensor<float>> result = out["output"];
            return angel_postprocess(result);
        }

        cv::Mat crop_rect(cv::Mat &img, cv::RotatedRect &rect, float alpha = 0.2)
        {
            float degree = rect.angle;
            cv::Size size = rect.size;
            cv::Point2f center = rect.center;

            float width = size.width;
            float height = size.height;
            if (width > height)
            {
                width = width + height * alpha;
            }
            else
            {
                height = height + width * alpha;
            }

            int diag_edge = std::sqrt(std::pow(img.rows, 2)+ std::pow(img.cols, 2));
            int rows_extend = (diag_edge - img.rows) >> 1;
            int cols_extend = (diag_edge - img.cols) >> 1;
            cv::Mat extend_img;
            cv::copyMakeBorder(img, extend_img, rows_extend, rows_extend, cols_extend, cols_extend, cv::BORDER_CONSTANT, cv::Scalar(0));
            center.x += cols_extend;
            center.y += rows_extend;
            cv::Mat extend_rotated;
            cv::Mat cropped;
            cv::Mat M = cv::getRotationMatrix2D(center, degree, 1);
            cv::warpAffine(extend_img, extend_rotated, M, cv::Size(extend_img.cols, extend_img.rows));
            cv::getRectSubPix(extend_rotated, cv::Size(width, height), center, cropped);
            return cropped;
        }

    private:
        std::vector<std::string> char_dictionary_;
        int factory_type_;
        int device_;
        std::map<std::string, float> param_map_;
        float message_det_thresh_ = -1.f;
        float message_box_thresh_ = -1.f;
        std::vector<std::unique_ptr<excalibur::pipeline<float>>> instance_;
        std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
    };


    txt_code_internal::txt_code_internal(std::string_view model_directory, std::string_view chardic_directory, int factory_type, int device, std::map<std::string, float>& param_map)
        : impl_{ std::make_unique<impl>(model_directory, chardic_directory, factory_type, device, param_map)}
    {
    }

    txt_code_internal::~txt_code_internal()
    {
    }

    exposing::param_vector<box_info> txt_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int top_five, int order, int x, int y, int roi_width, int roi_height) const
    {
        return impl_->detect(bitmap, channels, height, width, top_five, order, x, y, roi_width, roi_height);
    }

    std::string txt_code_internal::version()
    {
        return impl::version();
    }
}