#include "material_code_internal.hpp"
#include "hardcode.hpp"

#include <fstream>
#include <algorithm>
#include "box_info_impl.hpp"
#include "cool_cut_roi.hpp"
#include "char_segment.hpp"
#include "char_classfi.hpp"
#include <Excalibur/pipeline.hpp>
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include "Primitives/tensor_conversions.hpp"
#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_resize.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/highgui/highgui.hpp>

// #include <opencv2/opencv.hpp>

#include <cfloat>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

namespace glasssix::heimdall
{
    std::array<std::tuple<int, std::string, std::string, std::string>, 7> types = {
        {{0, "hot_rolled_det", "hot_rolled_rec", "hot_material_angle"},
        {1, "cool_rolled_det", "cool_rolled_rec", ""},
        {2, "hot_rolled_det_lite", "hot_rolled_rec_lite", "hot_material_angle"},
        {3, "cool_rolled_det_lite", "cool_rolled_rec_lite", ""},
        {4, "heavy_rail_det_lite", "heavy_rail_rec_lite", ""},
        {5, "heavy_rail_det_lite", "heavy_rail_segment_blank_simp", "heavy_rail_classify_char_simp_2"},
        {6, "cool_rolled_det", "segment_char_simp_3", "singel_char_classfi_simp"}} };

    class material_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int factory_type, int device)
            : factory_type_(factory_type), device_{device}, cut_rois_{ 2500 }
        {
            auto factory = std::find_if(types.begin(), types.end(), [factory_type](const std::tuple<int, std::string, std::string, std::string>& t)
                { return std::get<0>(t) == factory_type; });

            if (factory == types.end())
                throw exposing::abi_invalid_argument("Invalid factory_tpye param!");

            switch (factory_type)
            {
            case 0:
            case 2:
            case 5:
            case 6:
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<1>(*factory)), std::string(model_directory) + "/" + std::get<1>(*factory) + ".racy", device));
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<2>(*factory)), std::string(model_directory) + "/" + std::get<2>(*factory) + ".racy", device));
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<3>(*factory)), std::string(model_directory) + "/" + std::get<3>(*factory) + ".racy", device));
                if (factory_type == 5 || factory_type == 6)
                {
                    segement_instance_ = std::make_unique<char_segment>();
                    if(factory_type == 5)
                        classfi_instance_ = std::make_unique<char_classfi>(label_type::HEAVY_RAIL);
                    else if(factory_type == 6)
                        classfi_instance_ = std::make_unique<char_classfi>(label_type::COOL_ROLL);
                }
                break;
            case 1:
            case 3:
            case 4:
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<1>(*factory)), std::string(model_directory) + "/" + std::get<1>(*factory) + ".racy", device));
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<2>(*factory)), std::string(model_directory) + "/" + std::get<2>(*factory) + ".racy", device));
                break;
            default:
                break;
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
            if (factory_type_ == 0 || factory_type_ == 2)
                run_hot_roll(results, roi, top_five);
            else if (factory_type_ == 1 || factory_type_ == 3 || factory_type_ == 6)
                run_cool_roll(results, roi, top_five);
            else if (factory_type_ == 4 || factory_type_ == 5)
                run_heavy_rail(results, roi, top_five);
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
            return "1.0.0";
        }

    private:
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

            if (order == memory::NHWC)
                cache_->convert_order();
        }

        void softmax_along_width(std::shared_ptr<memory::tensor<float>> &input)
        {
            int height = input->height();
            int width = input->width();
            for (int h = 0; h < height; ++h)
            {
                float sum = 0.0f;
                std::vector<float> y(width);
                float *input_data = input->mutable_cpu_data() + input->offset(0, 0, h);
                for (size_t i = 0; i < width; ++i)
                {
                    sum += y[i] = std::exp(input_data[i]);
                }
                for (size_t i = 0; i < width; ++i)
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

        /**
 * @brief resize the shorter edge of img to specified size
 * @param short_size specified size
 * @param img 
 * @return cv::Mat 
 */
        std::shared_ptr<memory::tensor<uint8_t>> resize_fixed_size(float short_size, std::shared_ptr<memory::tensor<uint8_t>> &img)
        {
            int w = img->width();
            int h = img->height();
            float ratio = 0;
            if (std::min(w, h) < short_size)
            {
                ratio = w < h ? (short_size / w) : (short_size / h);
            }
            else
            {
                ratio = 1;
            }
            int resize_w = int(w * ratio);
            int resize_h = int(h * ratio);
            resize_w = (resize_w >> 5) << 5;
            resize_h = (resize_h >> 5) << 5;
            std::shared_ptr<memory::tensor<uint8_t>> dst;
            excalibur::resize_cpu(img, dst, resize_h, resize_w);
            return dst;
        }

        //void det_preprocess(std::shared_ptr<memory::tensor<float>> &input)
        //{
        //    float mean[] = {0.485, 0.456, 0.406};
        //    float std[] = {0.229, 0.224, 0.225};
        //    int cstep = input->width() * input->height();
        //    float *input_tensor_data = input->mutable_cpu_data();
        //    for (int c = 0; c < input->channels(); ++c)
        //    {
        //        for (int i = 0; i < cstep; ++i)
        //        {
        //            input_tensor_data[c * cstep + i] = (input_tensor_data[c * cstep + i] / 255.f - mean[c]) / std[c];
        //        }
        //    }
        //}

        /**
 * @brief polygon retraction
 * @param pList make sure they're in counterclockwise order
 * @param out output
 * @param SAFELINE negative numbers are contractions, positive numbers are expansions
 */
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
            float distance = cv::contourArea(box) * unclip_ratio / cv::arcLength(box, true);
            // contour shrinkage
            std::vector<cv::Point2f> out;
            expand_polygon(box, out, distance * (-1));
            return out;
        }

        void boxes_from_bitmap(cv::Mat &out, cv::Mat &mask, int src_w, int src_h, std::vector<std::vector<cv::Point2f>> &boxes, std::vector<float> &scores)
        {
            size_t max_candidates = 1000;
            int min_size = 3;
            float box_thresh = 0.5;
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
                std::vector<cv::Point2f> box = unclip(points);
                // sside: minimum between width and height of external retangel
                points.clear();
                get_mini_boxes(box, points, sside);
                if (sside < min_size + 2)
                {
                    continue;
                }
                for (int i = 0; i < points.size(); ++i)
                {
                    points[i].x = std::min(std::max(points[i].x / width * src_w, 0.f), (const float)src_w);
                    points[i].y = std::min(std::max(points[i].y / height * src_h, 0.f), (const float)src_h);
                }
                // boxes.insert(boxes.end(), points.begin(), points.end());
                boxes.push_back(points);
                scores.push_back(score);
            }
        }

        void det_post_process(std::shared_ptr<memory::tensor<float>> &out_, std::vector<std::vector<cv::Point2f>> &boxes, std::vector<float> &scores)
        {
            cv::Mat out(out_->height(), out_->width(), CV_32FC1);
            memcpy(out.data, out_->cpu_data(), out_->count() * sizeof(float));
            int src_w = out.cols;
            int src_h = out.rows;
            float thresh = 0.3;
            int count = out_->count();
            const float *out_data = out_->cpu_data();
            cv::Mat mask(out_->height(), out_->width(), CV_8UC1);
            std::uint8_t *mask_data = mask.data;
            for (int i = 0; i < count; ++i)
            {
                mask_data[i] = (out_data[i] > thresh ? 1 : 0) * 255;
            }
            boxes_from_bitmap(out, mask, src_w, src_h, boxes, scores);
        }

        std::pair<std::vector<std::vector<cv::Point2f>>, std::vector<float>> det_combine_best(std::shared_ptr<memory::tensor<uint8_t>> &input, excalibur::pipeline<float>& det_instance_)
        {
            auto input_tensor = input | memory::tensor_convert_to<float>;
            // pre process
            //det_preprocess(input_tensor);
            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> out = det_instance_.forward(input_tensor);
            std::shared_ptr<memory::tensor<float>> output = out["output"];
            std::vector<std::vector<cv::Point2f>> boxes;
            std::vector<float> scores;
            det_post_process(output, boxes, scores);
            return std::make_pair(boxes, scores);
        }

        std::pair<std::vector<std::string>, std::vector<std::vector<float>>> decode(std::vector<std::vector<int>> &idxs, std::vector<std::vector<float>> &probs, std::vector<std::string> &character, int top_five, bool remove_duplicate = true)
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
            softmax_along_width(result);
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
            static std::vector<std::string> heimdall_rec_character{"blank", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};

            // decode char
            return decode(idxs, probs, heimdall_rec_character, top_five);
        }

        std::pair<std::vector<std::string>, std::vector<std::vector<float>>> rec_combine_best(cv::Mat &cut_img, int top_five, excalibur::pipeline<float>& rec_instance_)
        {
            float ratio = 32.f / cut_img.rows;
            cv::Mat resized_cut_img;
            cv::resize(cut_img, resized_cut_img, cv::Size(0, 0), ratio, ratio, cv::INTER_LINEAR);
            // convert from mat to tensor
            std::shared_ptr<memory::tensor<uint8_t>> input(new memory::tensor<uint8_t>(resized_cut_img.channels(), resized_cut_img.rows, resized_cut_img.cols, -1, memory::NHWC, nullptr));
            std::copy(resized_cut_img.data, resized_cut_img.data + resized_cut_img.step[0] * resized_cut_img.rows, input->mutable_cpu_data());
            input->convert_order();
            auto input_tensor = input | memory::tensor_convert_to<float>;
            //// pre process
            //int count = input_tensor->count();
            //float *input_data = input_tensor->mutable_cpu_data();
            //for (int i = 0; i < count; ++i)
            //{
            //    input_data[i] = (input_data[i] / 255.f - 0.5f) * 2.0f;
            //}
            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> out = rec_instance_.forward(input_tensor);
            std::shared_ptr<memory::tensor<float>> result = out["output"];
            // post process
            return rec_combine_postprocess(result, top_five);
        }

        std::vector<float> angel_postprocess(std::shared_ptr<memory::tensor<float>> &result)
        {
            softmax_along_width(result);
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

        std::vector<float> angel_infer(cv::Mat &cut_img, excalibur::pipeline<float>& angle_instance_)
        {
            float ratio = 32.f / cut_img.rows;
            cv::resize(cut_img, cut_img, cv::Size(0, 0), ratio, ratio, cv::INTER_LINEAR);
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

        void run_hot_roll(std::vector<box_info_internal> &results, std::vector<int> &roi, int top_five)
        {
            excalibur::rectangle<int> rect((int)roi[0], (int)roi[1], (int)roi[2], (int)roi[3]);
            std::shared_ptr<memory::tensor<uint8_t>> input;
            // image preprocessing
            excalibur::safty_cut_cpu(cache_, input, &rect);
            std::shared_ptr<memory::tensor<uint8_t>> resized_img = resize_fixed_size(640, input);
            cv::Mat resized_mat_img(resized_img->height(), resized_img->width(), CV_8UC3);
            // convert NCHW TO NHWC
            const uint8_t *in_data = resized_img->cpu_data();
            int step = resized_img->width() * resized_img->height();
            int channels = resized_img->channels();
            for (int i = 0; i < step; ++i)
            {
                for (int c = 0; c < channels; ++c)
                {
                    *(resized_mat_img.data + channels * i + c) = *(in_data + resized_img->offset(0, c) + i);
                }
            }
            // ocr detect
            std::pair<std::vector<std::vector<cv::Point2f>>, std::vector<float>> result = det_combine_best(resized_img, *instance_[0]);
            std::vector<std::vector<cv::Point2f>> box_list = result.first;

            float ratio = roi[3] * 1.0f / resized_img->width();
            for (int i = 0; i < box_list.size(); ++i)
            {
                bool rotate = false;
                bool inverse = false;
                cv::RotatedRect rect = cv::minAreaRect(box_list[i]);
                // crop img
                cv::Mat cut_img = crop_rect(resized_mat_img, rect);
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

                std::vector<float> res_vec = angel_infer(cut_img, *instance_[2]);
                // 0: The character direction is inverse  1: The character direction is positive
                if (res_vec[0] == 0)
                {
                    cv::flip(cut_img, cut_img, -1);
                    inverse = true;
                }
                
                std::pair<std::vector<std::string>, std::vector<std::vector<float>>> out = rec_combine_best(cut_img, top_five, *instance_[1]);

                box_info_internal box;
                auto location = exposing::make_param_vector<float>();
                for (int j = 0; j < box_list[i].size(); ++j)
                {
                    location.push_back(box_list[i][j].x * ratio);
                    location.push_back(box_list[i][j].y * ratio);
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

                box.cut_roi = exposing::make_param_vector<std::uint8_t>();
                box.cut_roi.resize(cut_img.step[0] * cut_img.rows);
                box.cut_roi.copy_from({ cut_img.data, static_cast<size_t>(cut_img.step[0] * cut_img.rows) }, 0);

                box.cut_roi_width = cut_img.cols;
                box.cut_roi_height = cut_img.rows;

                results.push_back(box);
            }
        }

        void run_heavy_rail(std::vector<box_info_internal>& results, std::vector<int>& roi, int top_five)
        {
            excalibur::rectangle<int> rect((int)roi[0], (int)roi[1], (int)roi[2], (int)roi[3]);
            std::shared_ptr<memory::tensor<uint8_t>> input;
            // image preprocessing
            excalibur::safty_cut_cpu(cache_, input, &rect);
            std::shared_ptr<memory::tensor<uint8_t>> resized_img = resize_fixed_size(640, input);
            cv::Mat resized_mat_img(resized_img->height(), resized_img->width(), CV_8UC3);
            // convert NCHW TO NHWC
            const uint8_t* in_data = resized_img->cpu_data();
            int step = resized_img->width() * resized_img->height();
            int channels = resized_img->channels();
            for (int i = 0; i < step; ++i)
            {
                for (int c = 0; c < channels; ++c)
                {
                    *(resized_mat_img.data + channels * i + c) = *(in_data + resized_img->offset(0, c) + i);
                }
            }
            // ocr detect
            std::pair<std::vector<std::vector<cv::Point2f>>, std::vector<float>> result = det_combine_best(resized_img, *instance_[0]);
            std::vector<std::vector<cv::Point2f>> box_list = result.first;

            float ratio = roi[3] * 1.0f / resized_img->width();
            for (int i = 0; i < box_list.size(); ++i)
            {
                bool rotate = false;
                cv::RotatedRect rect = cv::minAreaRect(box_list[i]);
                // crop img
                cv::Mat cut_img = crop_rect(resized_mat_img, rect);
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

                std::pair<std::vector<std::string>, std::vector<std::vector<float>>> out;
                if (factory_type_ == 5)
                {
                    cv::Mat roi_temp = cut_img.clone();
                    std::vector<float> segement_result = segement_instance_->detect(roi_temp, true, *instance_[1]);
                    std::string stringinfo;
                    std::vector<float> probs;

                    float left = 0.f, right = 0.f;
                    if (segement_result[0] < 0)
                    {
                        left = std::ceil(std::abs(segement_result[0]));
                        segement_result[0] = 0;
                    }

                    if (segement_result[segement_result.size() - 1] > roi_temp.cols)
                        right = std::ceil(segement_result[segement_result.size() - 1] - roi_temp.cols);

                    cv::copyMakeBorder(roi_temp, roi_temp, 0, 0, left, right, cv::BorderTypes::BORDER_CONSTANT, cv::Scalar::all(0));

                    for (size_t j = 0; j < segement_result.size() - 1; j++)
                    {
                        cv::Mat small_img = roi_temp(cv::Range::all(), cv::Range((int)segement_result[j], (int)segement_result[j + 1]));
                        auto [label, prob] = classfi_instance_->detect(small_img, *instance_[2]);
                        stringinfo.push_back(label);
                        probs.push_back(prob);
                    }
                    out = std::make_pair<std::vector<std::string>, std::vector<std::vector<float>>>({ stringinfo }, { probs });
                }
                else
                {
                    // run identify network
                    out = rec_combine_best(cut_img, top_five, *instance_[1]);
                }

                box_info_internal box;
                auto location = exposing::make_param_vector<float>();
                for (int j = 0; j < box_list[i].size(); ++j)
                {
                    location.push_back(box_list[i][j].x * ratio);
                    location.push_back(box_list[i][j].y * ratio);
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
                if (!rotate)
                {
                    angle = std::abs(rect.angle);
                }
                else if (rotate)
                {
                    angle = std::abs(rect.angle) + 270;
                    angle = angle == 360 ? 0 : angle;
                }
                box.angle = angle;

                box.cut_roi = exposing::make_param_vector<std::uint8_t>();
                box.cut_roi.resize(cut_img.step[0] * cut_img.rows);
                box.cut_roi.copy_from({ cut_img.data, static_cast<size_t>(cut_img.step[0] * cut_img.rows) }, 0);

                box.cut_roi_width = cut_img.cols;
                box.cut_roi_height = cut_img.rows;

                results.push_back(box);
            }
        }

        void run_cool_roll(std::vector<box_info_internal>& results, std::vector<int>& roi, int top_five)
        {
            excalibur::rectangle<int> rect(roi[0], roi[1], roi[2], roi[3]);
            std::shared_ptr<memory::tensor<uint8_t>> input;
            // image preprocessing
            excalibur::safty_cut_cpu(cache_, input, &rect);
            std::shared_ptr<memory::tensor<uint8_t>> resized_img = resize_fixed_size(640, input);
            cv::Mat resized_mat_img(resized_img->height(), resized_img->width(), CV_8UC3);
            // Mat convert NCHW TO NHWC
            const uint8_t* in_data = resized_img->cpu_data();
            int step = resized_img->width() * resized_img->height();
            int channels = resized_img->channels();
            for (int i = 0; i < step; ++i)
            {
                for (int c = 0; c < channels; ++c)
                {
                    *(resized_mat_img.data + channels * i + c) = *(in_data + resized_img->offset(0, c) + i);
                }
            }

            // ocr detect
            std::pair<std::vector<std::vector<cv::Point2f>>, std::vector<float>> result = det_combine_best(resized_img, *instance_[0]);
            std::vector<std::vector<cv::Point2f>> box_list = result.first;
            float ratio = roi[3] * 1.0f / resized_img->width();

            if (box_list.size() > 1)
            {
                cordinate_roi result_cut = cut_rois_.cut_roi_gather(box_list, resized_mat_img);

                for (size_t i = 0; i < result_cut.rois.size(); i++)
                {
                    if (result_cut.max_R[i] > 1500)
                        continue;

                    float angle = 0.f;
                    // run identify network
                    std::pair<std::vector<std::string>, std::vector<std::vector<float>>> out;
                    if (factory_type_ == 6)
                    {
                        cv::Mat roi_temp = result_cut.rois[i].clone();
                        std::vector<float> segement_result = segement_instance_->detect(roi_temp, false, *instance_[1]);
                        std::string stringinfo;
                        std::vector<float> probs;

                        float left = 0.f, right = 0.f;
                        if (segement_result[0] < 0)
                        {
                            left = std::ceil(std::abs(segement_result[0]));
                            segement_result[0] = 0;
                        }

                        if (segement_result[segement_result.size() - 1] > roi_temp.cols)
                            right = std::ceil(segement_result[segement_result.size() - 1] - roi_temp.cols);
                        
                        cv::copyMakeBorder(roi_temp, roi_temp, 0, 0, left, right, cv::BorderTypes::BORDER_CONSTANT, cv::Scalar::all(0));

                        for (size_t j = 0; j < segement_result.size () - 1; j++)
                        {
                            cv::Mat small_img = roi_temp(cv::Range::all(), cv::Range((int)segement_result[j], (int)segement_result[j + 1]));
                            auto [label, prob] = classfi_instance_->detect(small_img, *instance_[2]);
                            stringinfo.push_back(label);
                            probs.push_back(prob);
                        }
                        out = std::make_pair<std::vector<std::string>, std::vector<std::vector<float>>>({ stringinfo }, { probs });
                    }
                    else
                        out = rec_combine_best(result_cut.rois[i], top_five, *instance_[1]);

                    box_info_internal box;
                    box.location = exposing::make_param_vector<float>();
                    for (size_t j = 0; j < result_cut.cordinate[i].size(); ++j)
                    {
                        box.location.push_back(result_cut.cordinate[i][j].x * ratio);
                        box.location.push_back(result_cut.cordinate[i][j].y * ratio);
                    }
                    box.strinfos = exposing::make_param_vector<exposing::param_string>();
                    for (size_t j = 0; j < out.first.size(); ++j)
                    {
                        box.strinfos.push_back(exposing::param_string(out.first[j]));
                    }
                    box.angle = angle;

                    box.cut_roi = exposing::make_param_vector<std::uint8_t>();
                    box.cut_roi.resize(result_cut.rois[i].step[0] * result_cut.rois[i].rows);
                    box.cut_roi.copy_from({ result_cut.rois[i].data, static_cast<size_t>(result_cut.rois[i].step[0] * result_cut.rois[i].rows) }, 0);

                    box.cut_roi_width = result_cut.rois[i].cols;
                    box.cut_roi_height = result_cut.rois[i].rows;

                    results.push_back(box);
                }
            }
        }

    private:
        int factory_type_;
        int device_;
        std::vector<std::unique_ptr<excalibur::pipeline<float>>> instance_;
        std::unique_ptr<char_segment> segement_instance_;
        std::unique_ptr<char_classfi> classfi_instance_;

        cut_reg_roi cut_rois_;
        std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
    };

    material_code_internal::material_code_internal(std::string_view model_directory, int factory_type, int device)
        : impl_{ std::make_unique<impl>(model_directory, factory_type, device)}
    {
    }

    material_code_internal::~material_code_internal()
    {
    }

    exposing::param_vector<box_info> material_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int top_five, int order, int x, int y, int roi_width, int roi_height) const
    {
        return impl_->detect(bitmap, channels, height, width, top_five, order, x, y, roi_width, roi_height);
    }

    std::string material_code_internal::version()
    {
        return impl::version();
    }
}