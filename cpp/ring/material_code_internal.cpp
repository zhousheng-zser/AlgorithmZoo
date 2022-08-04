#include "material_code_internal.hpp"
#include "hardcode.hpp"

#include <fstream>
#include <algorithm>

#include "box_info_impl.hpp"
#include "cool_cut_roi.hpp"
#include "char_segment.hpp"
#include "char_classfi.hpp"

#include <Excalibur/pipeline.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_resize.hpp>
#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_rgb2gray.hpp"

#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>
#include "Primitives/logger.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <abi/param_vector.hpp>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

namespace glasssix::ring
{
    template <typename T>
    std::shared_ptr < glasssix::memory::tensor<T>> operator>(std::shared_ptr<glasssix::memory::tensor<T>>& tensor, float x)
    {
        T *ptr = tensor->mutable_cpu_data();
        for (int i = 0; i < tensor->count(); ++i) {
            if (ptr[i] > x)
                ptr[i] = 1.0;
            else
                ptr[i] = 0.0;
        }
        return tensor;
    }

    std::array<std::tuple<int, std::string, std::string, std::string, std::string>, 2> types =
    {
        
        { {8, "bar_det_lite", "bar_segment", "bar_angle", "bar_category"},
          {9, "bar_det_box", "bar_det_orientation", "bar_segment", "bar_category"} }
    };

    class material_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int factory_type, int device)
            : factory_type_(factory_type), device_{ device }, cut_rois_{ 2500 }
        {
            auto factory = std::find_if(types.begin(), types.end(), [factory_type](const std::tuple<int, std::string, std::string, std::string, std::string>& t)
                { return std::get<0>(t) == factory_type; });

            if (factory == types.end())
                throw exposing::abi_invalid_argument("Invalid factory_tpye param!");

            switch (factory_type)
            {
            case 8:
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<1>(*factory)), std::string(model_directory) + "/" + std::get<1>(*factory) + ".racy", device));
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<2>(*factory)), std::string(model_directory) + "/" + std::get<2>(*factory) + ".racy", device));
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<3>(*factory)), std::string(model_directory) + "/" + std::get<3>(*factory) + ".racy", device));
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<4>(*factory)), std::string(model_directory) + "/" + std::get<4>(*factory) + ".racy", device));
                segement_instance_ = std::make_unique<char_segment>(0.6, 0.25, 8, true);
                classfi_instance_ = std::make_unique<char_classfi>(label_type::HEAVY_RAIL);
                break;
            case 9:
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<1>(*factory)), std::string(model_directory) + "/" + std::get<1>(*factory) + ".racy", device));
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<2>(*factory)), std::string(model_directory) + "/" + std::get<2>(*factory) + ".racy", device));
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<3>(*factory)), std::string(model_directory) + "/" + std::get<3>(*factory) + ".racy", device));
                instance_.emplace_back(std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params(std::get<4>(*factory)), std::string(model_directory) + "/" + std::get<4>(*factory) + ".racy", device));
                segement_instance_ = std::make_unique<char_segment>(0.6, 0.25, 8, true);
                classfi_instance_ = std::make_unique<char_classfi>(label_type::HEAVY_RAIL);
                break;
            default:
                break;
            }
        }

        exposing::param_vector<box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int border_orient, int order,
                                                                     int x, int y, int roi_width, int roi_height)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            // roi params
            std::vector<int> roi{ x, y, roi_height, roi_width };
            init_cache(bitmap, channels, height, width, order, roi);

            std::vector<box_info_internal> results;

            auto result = exposing::make_param_vector<box_info>();

            if (factory_type_ == 8)
            {
                run_bar(results, roi);
            }
            else if (factory_type_ == 9)
            {
                run_bar_2(results, roi, border_orient);
            }
            else
                return result;

            for (auto& i : results)
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
        void init_cache(exposing::param_span<std::uint8_t>& bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order, std::vector<int>& roi)
        {
            if (cache_ == nullptr || cache_->channels() != channels || cache_->height() != height || cache_->width() != width || cache_->order() != order)
            {
                std::vector<int> shape;
                if (order == memory::NCHW)
                    shape = { static_cast<int>(1), channels, height, width };
                else if (order == memory::NHWC)
                    shape = { static_cast<int>(1), height, width, channels };
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

        void softmax_along_width(std::shared_ptr<memory::tensor<float>>& input, int dim)
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

        /**
         * @brief resize the shorter edge of img to specified size
         * @param short_size specified size
         * @param img
         * @return cv::Mat
         */
        std::pair<std::shared_ptr<memory::tensor<uint8_t>>, float> resize_fixed_size(float short_size, std::shared_ptr<memory::tensor<uint8_t>>& img, int flag = 0)
        {
            int w = img->width();
            int h = img->height();
            float ratio = 0;
            int resized_w = 0, resized_h = 0, aligned_w = 0, aligned_h = 0;

            std::shared_ptr<memory::tensor<uint8_t>> resized;
            if (flag == 1)
            {
                ratio = w < h ? (short_size / h) : (short_size / w);
                resized_w = static_cast<int>(std::round(w * ratio));
                resized_h = static_cast<int>(std::round(h * ratio));

                excalibur::resize_cpu(img, resized, resized_h, resized_w);

                aligned_w = ((static_cast<int>(short_size) + 31) >> 5) << 5;
                aligned_h = ((static_cast<int>(short_size) + 31) >> 5) << 5;
            }
            else
            {
                if (std::min(w, h) < short_size)
                {
                    ratio = w < h ? (short_size / w) : (short_size / h);
                    resized_w = int(w * ratio);
                    resized_h = int(h * ratio);
                    excalibur::resize_cpu(img, resized, resized_h, resized_w);
                }
                else
                {
                    ratio = 1;
                    resized_w = w;
                    resized_h = h;
                    resized = img;
                }

                aligned_w = ((resized_w + 31) >> 5) << 5;
                aligned_h = ((resized_h + 31) >> 5) << 5;
            }

            std::shared_ptr<memory::tensor<uint8_t>> dst;
            if (flag == 1)
                excalibur::make_border(resized, dst, 0, aligned_h - resized_h, 0, aligned_w - resized_w, excalibur::border_constant, (uint8_t)127);
            else
                excalibur::make_border(resized, dst, 0, aligned_h - resized_h, 0, aligned_w - resized_w);
            return { dst, 1.0f / ratio };
        }

        /**
         * @brief polygon retraction
         * @param pList make sure they're in counterclockwise order
         * @param out output
         * @param SAFELINE negative numbers are contractions, positive numbers are expansions
         */
        void expand_polygon(const std::vector<cv::Point2f>& pList, std::vector<cv::Point2f>& out, float SAFELINE)
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

        bool cmp(const cv::Point2f& a, const cv::Point2f& b)
        {
            return a.x > b.x;
        }

        float get_value(std::vector<cv::Point2f>& box, bool is_min, int index)
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
        void get_mini_boxes(std::vector<T>& contour, std::vector<cv::Point2f>& box, cv::Size& size)
        {
            cv::RotatedRect bounding_box = cv::minAreaRect(contour);
            cv::Point2f points[4];
            bounding_box.points(points);
            
            std::vector<cv::Point2f> points_sort = { std::begin(points), std::end(points) };
            
           
            std::sort(points_sort.begin(), points_sort.end(),
                [](const cv::Point2f& point1, const cv::Point2f& point2) { return point1.x < point2.x; });

            int indexs[4] = {0, 1, 2, 3};
            if (points_sort[1].y > points_sort[0].y)
            {
                indexs[0] = 0;
                indexs[3] = 1;
            }
            else
            {
                indexs[0] = 1;
                indexs[3] = 0;
            }
            
            if (points_sort[3].y > points_sort[2].y)
            {
                indexs[1] = 2;
                indexs[2] = 3;
            }
            else
            {
                indexs[1] = 3;
                indexs[2] = 2;
            }

            for (int i = 0; i < 4; ++i)
            {
                box.push_back(points_sort[ indexs[i] ]);
            }

            size = bounding_box.size;
        }

        float box_score_fast(cv::Mat& bitmap, std::vector<cv::Point2f> box)
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

        float box_score_slow(cv::Mat& bitmap, std::vector<cv::Point>& box)
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
        std::vector<cv::Point2f> unclip(std::vector<cv::Point2f>& box, float unclip_ratio = 0.8)
        {
            float distance = cv::contourArea(box) * unclip_ratio / cv::arcLength(box, true);
            // contour shrinkage
            std::vector<cv::Point2f> out;
            expand_polygon(box, out, distance * (-1));
            return out;
        }

        void boxes_from_bitmap_bar(
            cv::Mat& out, 
            cv::Mat& mask, 
            int src_w, 
            int src_h, 
            size_t max_candidates,
            int min_size,
            float box_thresh,
            float unclip_ratio,
            std::vector<std::vector<cv::Point2f>>& boxes, 
            std::vector<float>& scores, 
            std::vector<cv::Size>& sizes)
        {
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
                cv::Size size;
                get_mini_boxes(contour, points, size);
                float sside = std::min(size.height, size.width);
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
                get_mini_boxes(box, points, size);
                sside = std::min(size.height, size.width);
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
                sizes.push_back(size);
            }
        }

        void det_post_process_bar( std::shared_ptr<memory::tensor<float>>& out_, /*in*/ 
                                   const std::map<std::string, float> &params,   /*in*/ 
                                   std::vector<std::vector<cv::Point2f>>& boxes, /*out*/
                                   std::vector<float>& scores,  /*out*/
                                   std::vector<cv::Size>& sizes /*out*/)
        {
            float thresh = params.at("thresh");
            size_t max_candidates = (size_t)params.at("max_candidates");
            int min_size = (int)params.at("min_size");
            float box_thresh = params.at("box_thresh");
            float unclip_ratio = params.at("unclip_ratio");

            cv::Mat out(out_->height(), out_->width(), CV_32FC1);
            memcpy(out.data, out_->cpu_data(), out_->count(2, 4) * sizeof(float));
            int src_w = out.cols;
            int src_h = out.rows;
            int count = out_->count(2, 4);
            const float* out_data = out_->cpu_data();
            cv::Mat mask(out_->height(), out_->width(), CV_8UC1);
            std::uint8_t* mask_data = mask.data;
            for (int i = 0; i < count; ++i)
            {
                mask_data[i] = (out_data[i] > thresh ? 1 : 0) * 255;//二值化
            }
            boxes_from_bitmap_bar(out, mask, src_w, src_h, max_candidates, min_size, box_thresh, unclip_ratio, boxes, scores, sizes);
        }

        std::pair<std::vector<std::string>, std::vector<std::vector<float>>> decode(std::vector<std::vector<int>>& idxs, std::vector<std::vector<float>>& probs, std::vector<std::string>& character, int border_orient, bool remove_duplicate = true)
        {
            int K = 1;
            if (border_orient)
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

        std::vector<float> angel_postprocess(std::shared_ptr<memory::tensor<float>>& result)
        {
            softmax_along_width(result, 1);
            int idx = 0;
            float prob = 0;
            const float* res_data = result->cpu_data();
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

        std::vector<float> angel_infer(cv::Mat& cut_img, excalibur::pipeline<float>& angle_instance_)
        {
            float ratio = 32.f / cut_img.rows;
            cv::resize(cut_img, cut_img, cv::Size(0, 0), ratio, ratio, cv::INTER_LINEAR);
            // convert from mat to tensor
            std::shared_ptr<memory::tensor<uint8_t>> input(new memory::tensor<uint8_t>(cut_img.channels(), cut_img.rows, cut_img.cols, -1, memory::NHWC, nullptr));
            std::copy(cut_img.data, cut_img.data + cut_img.step[0] * cut_img.rows, input->mutable_cpu_data());
            input->convert_order();
            if (input->width() < 320)
                excalibur::make_border(input, input, 0, 0, 0, 320 - input->width(), excalibur::border_constant);
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

        cv::Mat crop_rect(cv::Mat& img, cv::RotatedRect& rect, float alpha = 0.2)
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

            int diag_edge = std::sqrt(std::pow(img.rows, 2) + std::pow(img.cols, 2));
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

        void run_bar(std::vector<box_info_internal>& results, std::vector<int>& roi) {

            excalibur::rectangle<int> rect((int)roi[0], (int)roi[1], (int)roi[2], (int)roi[3]);
            std::shared_ptr<memory::tensor<uint8_t>> input;

            // image preprocessing
            excalibur::safty_cut_cpu(cache_, input, &rect);
            cv::Mat input_mat(roi[2], roi[3], CV_8UC3);
            std::copy(input->cpu_data(), input->cpu_data() + input->count(1, 4), input_mat.data);

            if (input->order() == memory::NHWC)
                input->convert_order();


            auto [resized_img, ratio] = resize_fixed_size(640, input, 1);//填充式 resize


            auto input_tensor = resized_img | memory::tensor_convert_to<float>;


            // step 1
            // pre process
            // det_preprocess(input_tensor);
            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> out = instance_[0]->forward(input_tensor);

            std::shared_ptr<memory::tensor<float>> output = out["output"];

            std::vector<std::vector<cv::Point2f>> boxes;
            std::vector<float> scores;
            std::vector<cv::Size> sizes;
            std::map<std::string, float> params = { {"thresh", 0.3}, {"box_thresh", 0.8}, {"min_size", 3}, {"max_candidates", 1000}, {"unclip_ratio", 1.5} };
            det_post_process_bar(output, params, boxes, scores, sizes); //
            

            std::pair<std::vector<std::vector<cv::Point2f>>, std::vector<float>> result = std::make_pair(boxes, scores);


            // re calculate
            std::vector<std::vector<cv::Point2f>> box_list = result.first;
            for (size_t i = 0; i < box_list.size(); i++)
            {
                for (size_t j = 0; j < box_list[i].size(); j++)
                {
                    box_list[i][j] *= ratio;
                }
            }


            for (size_t i = 0; i < box_list.size(); ++i)
            {
                bool rotate = false;
                cv::RotatedRect rect = cv::minAreaRect(box_list[i]);
                // crop img
                cv::Mat cut_img = crop_rect(input_mat, rect);
                int newH = cut_img.rows;
                int newW = cut_img.cols;

                // ignore
                if (std::max(newH, newW) / (std::min(newH, newW) * 1.0) <= 1.5)
                    continue;
               
                // rotate
                if (newH > newW)
                {
                    cut_img = rotateAntiClockWise90(cut_img);
                    rotate = true;
                }

                // step 2
                bool inverse = false;
                std::vector<float> res_vec = angel_infer(cut_img, *instance_[2]);

                // 0: The character direction is inverse  1: The character direction is positive
                if (res_vec[0] == 0)
                {
                    cv::flip(cut_img, cut_img, -1);
                    inverse = true;
                }

                std::pair<std::vector<std::string>, std::vector<std::vector<float>>> out;

                //cv::imshow("cut_img", cut_img);
                //cv::waitKey(0);
              

                // step 3 segment 
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


                // step 4 classifi 
                for (size_t j = 0; j < segement_result.size() - 1; j++)
                {
                    cv::Mat small_img = roi_temp(cv::Range::all(), cv::Range((int)segement_result[j], (int)segement_result[j + 1]));
                    //cv::imshow("small_img", small_img);
                    //cv::imwrite("D:/Desktop/small_img" + std::to_string(i) + std::to_string(j) + ".jpg", small_img);
                    //cv::waitKey(0);

                    auto [label, prob] = classfi_instance_->detect(small_img, *instance_[3]);
                    stringinfo.push_back(label);
                    probs.push_back(prob);
                }

                out = std::make_pair<std::vector<std::string>, std::vector<std::vector<float>>>({ stringinfo }, { probs });
                
                // step 5 collect data 
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

                box.cut_roi = exposing::make_param_vector<std::uint8_t>();
                box.cut_roi.resize(cut_img.step[0] * cut_img.rows);
                box.cut_roi.copy_from({ cut_img.data, static_cast<size_t>(cut_img.step[0] * cut_img.rows) }, 0);

                box.cut_roi_width = cut_img.cols;
                box.cut_roi_height = cut_img.rows;

                results.push_back(box);
            }
        }

        inline bool tagDelBox(int det_size, std::vector<cv::Point2f> box, int rect_pad_threshold, float wh_ratio=0.8) {
            int size = box.size();  // 多边形点数

            std::vector<float> dist_s = {};
            for (int i = 0; i < size; ++i)
            {
                auto a = (i + 1) - 1;
                auto b = (a + 1) % 4;
                auto start_point = box[a];
                auto end_point = box[b];
                auto vect = end_point - start_point;

                auto dis = sqrt(vect.x * vect.x + vect.y * vect.y);
                dist_s.push_back(dis);
            }

            std::sort(dist_s.begin(), dist_s.end());
            auto min_l = dist_s[0];
            auto max_l = dist_s[3];
            if (min_l / max_l < wh_ratio)
                return true;

            // 边缘框更严格要求宽高比例
            else if ( ((min_l / max_l) < (1 - (1 - wh_ratio) / 2)) && (box[0].x < rect_pad_threshold) )// 左边缘过滤
                return true;
            else if ( ((min_l / max_l) < (1 - (1 - wh_ratio) / 2)) && (box[0].x > det_size - rect_pad_threshold) )// 右边缘过滤
                return true;
            else      
                return false;
        }

        void custom_sort(std::vector<std::vector<cv::Point2f>>& box_list, std::vector<float>& score_list, int det_size = 320) 
        {
            float pad_ratio = 0.1;
            float  wh_ratio = 0.8;

            int rect_pad_threshold = int(det_size * pad_ratio);

            //auto cmp(a, b);

            // bubble sort with tag del
            // state 1
            int length = box_list.size();
            std::vector<int> tag_index = {};

            if (length == 1)
                if(tagDelBox(det_size, box_list[0], rect_pad_threshold, wh_ratio))
                    tag_index.push_back(0);

            // bubble sort with tag
            for (int i = 0; i < length - 1; ++i)
            {
                bool swapped = false;
                for (int j = 0; j < length -i - 1; ++j)
                {
                    // 最左边的先冒泡
                    if(cmp(box_list[j + 1][0], box_list[j][0]))
                    {
                        //swap
                        auto temp = box_list[j];
                        box_list[j] = box_list[j + 1];
                        box_list[j + 1] = temp;

                        auto temp_score = score_list[j];
                        score_list[j] = score_list[j + 1];
                        score_list[j + 1] = temp_score;

                        swapped = true;
                    }

                }

                // 标记不符合要求的框
                auto tag_idx = length - i - 1;
                if (tagDelBox(det_size, box_list[tag_idx], rect_pad_threshold, wh_ratio))
                    tag_index.push_back(tag_idx); // 记录标记

                if (!swapped)
                    break;
            }

            for(auto idx : tag_index){
                box_list.erase(box_list.begin() + idx);
                score_list.erase(score_list.begin() + idx);
            }

            std::reverse(box_list.begin(), box_list.end());

        }

        auto custom_perspective(cv::Mat& srcImg, std::vector<cv::Point2f>& box, int d_size =320) 
        {
            cv::Mat dstImg;

            cv::Point2f AffinePointsSrc[4] = { box[0], box[1], box[2], box[3] };
            cv::Point2f AffinePointsDst[4] = { cv::Point2f( 0, 0 ), cv::Point2f( d_size - 1, 0 ), 
                                               cv::Point2f( d_size - 1, d_size - 1 ), cv::Point2f( 0, d_size - 1 ) };

            cv::Mat TransMat = cv::getPerspectiveTransform(AffinePointsSrc, AffinePointsDst);
            warpPerspective(srcImg, dstImg, TransMat, cv::Size(d_size, d_size), CV_INTER_CUBIC);

            return dstImg;
        }

        static bool cmp_box_ylt(std::vector<cv::Point2f>& box1, std::vector<cv::Point2f>& box2) 
        {
            float cy_1 = 0, cy_2 = 0;
            int count = 0;
            for (int i = 0; i < box1.size(); ++i)
            {
                cy_1 += box1[i].y;
                cy_2 += box2[i].y;
                count++;
            }
            cy_1 /= count;
            cy_2 /= count;
            
            return cy_1 > cy_2;
        }
        static bool cmp_box_xlt(std::vector<cv::Point2f>& box1, std::vector<cv::Point2f>& box2)
        {
            float cx_1 = 0, cx_2 = 0;
            int count = 0;
            for (int i = 0; i < box1.size(); ++i)
            {
                cx_1 += box1[i].x;
                cx_2 += box2[i].x;
                count++;
            }
            cx_1 /= count;
            cx_2 /= count;

            return cx_1 > cx_2;
        }


        void ott_process(std::shared_ptr<memory::tensor<float>> & out_ott/*in*/, cv::Mat& book/*in*/, std::vector<std::vector<cv::Point2f>> & boxes_text/*in*/, std::vector<cv::Size>& sizes/*in*/,
            std::vector<cv::Mat>& text_rect_s /*out*/)
        {
            // ott process
            auto out_ott_bool = out_ott > 0.5;
            int a = (int)out_ott_bool->cpu_data()[0];
            int b = (int)out_ott_bool->cpu_data()[1];

            if (a == 1 && b == 1)
            {
                std::sort(boxes_text.begin(), boxes_text.end(), cmp_box_ylt);
                std::reverse(boxes_text.begin(), boxes_text.end());
            }
            else if (a == 0 && b == 0)
                std::sort(boxes_text.begin(), boxes_text.end(), cmp_box_ylt);
            else if (a == 0 && b == 1)
                std::sort(boxes_text.begin(), boxes_text.end(), cmp_box_xlt);
            else // a == 1 && b == 0
            {
                std::sort(boxes_text.begin(), boxes_text.end(), cmp_box_xlt);
                std::reverse(boxes_text.begin(), boxes_text.end());
            }

            int fix_height = 32; // 临时：可提出
            
            for (int i = 0; i < boxes_text.size(); ++i)
            {
                auto box = boxes_text[i];
                auto size = sizes[i];

                float h = std::min(size.height, size.width);
                float w = std::max(size.height, size.width);

                float ratio = fix_height / h;
                int new_w = int(ratio * w);
                cv::Size  dst_size(new_w, fix_height);

                std::vector<cv::Point2f> dst_points{ cv::Point2f(0, 0), cv::Point2f(dst_size.width, 0), cv::Point2f(dst_size.width, dst_size.height), cv::Point2f(0, dst_size.height) };
                std::vector<cv::Point2f> src_points{ };

                std::vector<int> indexes_box{};
                if (a == 1 && b == 1)
                    indexes_box = { 0, 1, 2, 3 };
                else if (a == 0 && b == 0)
                    indexes_box = { 2, 3, 0, 1 };

                else if (a == 0 && b == 1)
                    indexes_box = { 1, 2, 3, 0 };
                else // a == 1 && b == 0
                    indexes_box = { 3, 0, 1, 2 };

                for (auto idx : indexes_box)
                    src_points.push_back(box[idx]);

                cv::Mat imgText;
                cv::Mat TransMat = cv::getPerspectiveTransform(src_points, dst_points);
                warpPerspective(book, imgText, TransMat, dst_size, CV_INTER_CUBIC);

                text_rect_s.push_back(imgText);
            }
        }

        
        void run_bar_2(std::vector<box_info_internal>& results, std::vector<int>& roi, int border_orient) {

            // step 1
            // det box infer
            excalibur::rectangle<int> rect((int)roi[0], (int)roi[1], (int)roi[2], (int)roi[3]);

            std::shared_ptr<memory::tensor<uint8_t>> input;

            excalibur::safty_cut_cpu(cache_, input, &rect);

            cv::Mat input_mat(roi[2], roi[3], CV_8UC3);
            std::copy(input->cpu_data(), input->cpu_data() + input->count(1, 4), input_mat.data);

            if (input->order() == memory::NHWC)
                input->convert_order();

            auto [resized_img, ratio] = resize_fixed_size(320, input, 1);//临时: 320需提出
            auto input_tensor = resized_img | memory::tensor_convert_to<float>;


            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> out = instance_[0]->forward(input_tensor);

            std::shared_ptr<memory::tensor<float>> output = out["output"];

            std::vector<std::vector<cv::Point2f>> boxes_rect;
            std::vector<float> scores;
            std::vector<cv::Size> sizes;
            std::map<std::string, float> params = { {"thresh", 0.3}, {"box_thresh", 0.8}, {"min_size", 3}, {"max_candidates", 1000}, {"unclip_ratio", 0.8} };
            det_post_process_bar(output, params, boxes_rect, scores, sizes);


            for (size_t i = 0; i < boxes_rect.size(); i++)
            {
                for (size_t j = 0; j < boxes_rect[i].size(); j++)
                {
                    boxes_rect[i][j] *= ratio;
                }
            }

            custom_sort(boxes_rect, scores);

            if(boxes_rect.size() == 0)
                return;

            std::vector<cv::Point2f> book_location;
            if (border_orient == 0)//letf
            {
            }
            else if (border_orient == 1)//right
            {
                std::reverse(boxes_rect.begin(), boxes_rect.end());
            }
                
            book_location = boxes_rect[0];

            auto book = custom_perspective(input_mat, book_location, 320);

            // step 2 det 
            // orientation infer
            std::shared_ptr<memory::tensor<uint8_t>> input_orient(new memory::tensor<uint8_t>(book.channels(), book.rows, book.cols, -1, memory::NHWC, nullptr));
            std::copy(book.data, book.data + book.step[0] * book.rows, input_orient->mutable_cpu_data());

            input_orient->convert_order();
            auto input_tensor_orient = input_orient | memory::tensor_convert_to<float>;
            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> out_orient = instance_[1]->forward(input_tensor_orient);
            std::shared_ptr<memory::tensor<float>> output_map = out_orient["output"];
            std::shared_ptr<memory::tensor<float>> out_ott = out_orient["orientation"];

            // shrink map process
            std::vector<std::vector<cv::Point2f>> boxes_text;
            std::vector<float> scores_text;
            std::vector<cv::Size> sizes_text;
            std::map<std::string, float> params_orient = { {"thresh", 0.3}, {"box_thresh", 0.8}, {"min_size", 3}, {"max_candidates", 1000}, {"unclip_ratio", 1.35}};
            det_post_process_bar(output_map, params_orient, boxes_text, scores_text, sizes_text);

            // ott process
            std::vector<cv::Mat> text_rect_s;
            ott_process(out_ott, book, boxes_text, sizes_text, text_rect_s);

            std::string full_stringinfo;
            for (int i = 0; i < text_rect_s.size(); ++i)
            {
                auto text_img = text_rect_s[i];
                //cv::imshow("text_img", text_img);
                //cv::waitKey();

                // step 3 segment 
                cv::Mat roi_temp = text_img.clone();
                std::vector<float> segement_result = segement_instance_->detect(roi_temp, true, *instance_[2]);
                std::string stringinfo;
                std::vector<float> probs;

                if (!segement_result.empty())
                {
                    float left = 0.f, right = 0.f;
                    if (segement_result[0] < 0)
                    {
                        left = std::ceil(std::abs(segement_result[0]));
                        segement_result[0] = 0;
                    }
                    if (segement_result[segement_result.size() - 1] > roi_temp.cols)
                        right = std::ceil(segement_result[segement_result.size() - 1] - roi_temp.cols);

                    cv::copyMakeBorder(roi_temp, roi_temp, 0, 0, left, right, cv::BorderTypes::BORDER_CONSTANT, cv::Scalar::all(0));

                    // step 4 classifi
                    for (size_t j = 0; j < segement_result.size() - 1; j++)
                    {
                        cv::Mat small_img = roi_temp(cv::Range::all(), cv::Range((int)segement_result[j], (int)segement_result[j + 1]));

                        auto [label, prob] = classfi_instance_->detect(small_img, *instance_[3]);
                        stringinfo.push_back(label);
                        probs.push_back(prob);
                    }
                }

                full_stringinfo += stringinfo;
            }

            // collect box info
            box_info_internal box;

            box.location = exposing::make_param_vector<float>();

            if (false)
            {
                for (auto point : book_location)
                {
                    box.location.push_back(point.x);
                    box.location.push_back(point.y);
                }
            }
            else
            {
                for (auto box_location : boxes_rect)
                {
                    for (auto point : box_location)
                    {
                        box.location.push_back(point.x);
                        box.location.push_back(point.y);
                    }
                }
            }

            box.cut_roi = exposing::make_param_vector<std::uint8_t>();
            box.cut_roi.resize(book.step[0] * book.rows);
            box.cut_roi.copy_from({ book.data, static_cast<size_t>(book.step[0] * book.rows) }, 0);

            box.cut_roi_height = book.size().height;
            box.cut_roi_width = book.size().width;

            box.strinfos = exposing::make_param_vector<exposing::param_string>();
            box.strinfos.push_back(exposing::param_string(full_stringinfo));//only one

            box.angle = 0;//协议10中angle不用

            results.push_back(box);
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
        : impl_{ std::make_unique<impl>(model_directory, factory_type, device) }
    {
    }

    material_code_internal::~material_code_internal()
    {
    }

    std::string material_code_internal::version()
    {
        return impl::version();
    }


    exposing::param_vector<box_info> material_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int border_orient, int order,
                                                                    int x, int y, int roi_width, int roi_height) const
    {
        return impl_->detect(bitmap, channels, height, width, border_orient, order, x, y, roi_width, roi_height);
    }

}