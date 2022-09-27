#include <iostream>     // for test
#include <cmath>
#include <numeric>
#include <algorithm>
#include <tuple>
#include <wchar.h>

#include "../hardcode/hardcode.hpp"
#include "ocr_code_internal.hpp"
#include "box_info_impl.hpp"

#include <Excalibur/pipeline.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_resize.hpp>
#include "Excalibur/operation_make_border.hpp"

#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>
#include "Primitives/logger.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include <abi/param_vector.hpp>

// struct for locations
typedef struct Point4f
{
    float x;        // x1
    float y;        // y1
    float ex;       // x2
    float ey;       // y2
} box;

const static int base_seg_index[] = { 0, 60, 130, 200, 265, 314, 375, 439 };

std::vector<std::string> chinese_label_index1 = { "8499", "664b","5180","5b81","7518","8d63","9c81","8c6b","4eac","6caa","6d25", "6e1d", "8fbd","5409","9ed1","82cf","6d59","7696","95fd","9102", "6e58","7ca4","6842", "743c","5ddd","8d35","4e91", "85cf","9655","9752", "65b0" };

const static char char_label_index[] = { '0', '1', '2', '3','4','5','6','7','8','9','A','B','C','D','E','F','G','H',
    'J','K','L','M','N','P','Q','R','S','T','U','V','W','X','Y','Z' };


namespace glasssix::plate
{
    template <typename T>
    std::shared_ptr < glasssix::memory::tensor<T>> operator>(std::shared_ptr<glasssix::memory::tensor<T>>& tensor, float x)
    {
        T* ptr = tensor->mutable_cpu_data();
        for (int i = 0; i < tensor->count(); ++i) {
            if (ptr[i] > x)
                ptr[i] = 1.0;
            else
                ptr[i] = 0.0;
        }
        return tensor;
    }

    class ocr_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
            : model_directory_{ std::string(model_directory) }, device_{ device }
        {

            pnet_instance_ = std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params("plate_det_box", false), std::string(model_directory)                + "/" + "pnet_Weights_sim" + ".racy", device);
            onet_instance_ = std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params("plate_det_orientation", false), std::string(model_directory)        + "/" + "onet_Weights_sim" + ".racy", device);   
            resnet_chinese_instance_ = std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params("plate_det_chinese", false), std::string(model_directory)  + "/" + "res32_chinese_sim" + ".racy", device);
            resnet_char_instance_ = std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params("plate_det_char", false),    std::string(model_directory)     + "/" + "res20_char_sim"    + ".racy", device);
        }

        exposing::param_vector<box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order,
            int x, int y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            // roi params
            std::vector<int> roi{ x, y, roi_height, roi_width };
            init_cache(bitmap, channels, height, width, order, cache0_);

            auto result = exposing::make_param_vector<box_info>();

            auto results = run_detect_classfi(roi, param_map);

            result.push_back(glasssix::exposing::make_as_first<box_info_impl>(results));
            
            return result;
        }

        box_info trace(box_info plate, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            std::vector<int> plate_roi{ (int)plate.x(), (int)plate.y(), (int)plate.width(), (int)plate.height() };
            init_cache(bitmap, channels, height, width, order, cache1_);

            glasssix::excalibur::rectangle<int> rect(plate_roi[0], plate_roi[1], plate_roi[2], plate_roi[3]);
            std::shared_ptr<glasssix::memory::tensor<uint8_t>> input;

            glasssix::excalibur::safty_cut_cpu(cache1_, input, &rect);
            cv::Mat cut_plate_mat = cv::Mat(1, input->height(), input->width(), 3);
            std::memcpy(cut_plate_mat.data, input->cpu_data(), input->count(2, 4));

            box_info next_frame_box_info;
            next_frame_box_info.set_x(plate.x());
            next_frame_box_info.set_y(plate.y());
            next_frame_box_info.set_width(plate.width());
            next_frame_box_info.set_height(plate.height());

            // strinfo
            auto next_frame_str = char_segment_classfi(cut_plate_mat);

            auto next_frame_strinfos = exposing::param_string(next_frame_str);
            next_frame_box_info.set_strinfos(next_frame_strinfos);
            next_frame_box_info.set_aligned_images(plate.aligned_images());

            return next_frame_box_info;
        }

        static std::string version()
        {
            return "1.0.0";
        }

    private:

        void init_cache(exposing::param_span<std::uint8_t>& bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order, std::shared_ptr<memory::tensor<std::uint8_t>>& cache)
        {
            if (cache == nullptr || cache->channels() != channels || cache->height() != height || cache->width() != width || cache->order() != order)
            {
                std::vector<int> shape;
                if (order == memory::NCHW)
                    shape = { static_cast<int>(1), channels, height, width };
                else if (order == memory::NHWC)
                    shape = { static_cast<int>(1), height, width, channels };
                else
                    NOT_IMPLEMENTED;

                cache = std::make_shared<memory::tensor<std::uint8_t>>(shape, -1, (memory::orderType)order /*, &memory::pool_allocator_default<std::uint8_t>::get()*/);
            }

            if (cache->device() > 0)
            {
#ifdef USE_CUDA
                cudaMemcpy(cache->mutable_gpu_data(), bitmap, channels * height * width, cudaMemcpyHostToDevice);
#else
                NO_GPU;
#endif
            }
            else
                std::copy(bitmap.begin(), bitmap.end(), cache->mutable_cpu_data());

        }
        
        void create_scales(std::vector<float>& scales, const int factor_times, const int min_height, const int min_width, std::pair<int, int>& min_lp_size)
        {
            float factor = 0.707;   // sqrt(1.5)
            int factor_count = 0;

            auto height = static_cast<float>(min_height);
            auto width = static_cast<float>(min_width);

            while ((height > min_lp_size.second) && (width > min_lp_size.first) && (factor_count < factor_times))
            {
                scales.push_back(pow(factor, factor_count));
                height *= factor;
                width *= factor;
                factor_count += 1;
            }
        }

        void pnet_select_indices(std::tuple<std::vector<int>, std::vector<int>>& indices, std::vector<float>& probs, int width, int height, const float thresh)
        {
            std::vector<int> indices_w;
            std::vector<int> indices_h;

            for (int i = 0; i < height; i++)
            {
                for (int j = 0; j < width; j++)
                {
                    if (probs[i * width + j] > thresh)
                    {
                        indices_w.push_back(i);
                        indices_h.push_back(j);
                    }
                }
            }
            indices = std::make_tuple(indices_w, indices_h);
        }

        void pnet_select_offsets(std::vector<Point4f>& offsets_inds,
            std::vector<float>& tx1,
            std::vector<float>& tx2,
            std::vector<float>& ty1,
            std::vector<float>& ty2,
            std::tuple<std::vector<int>, std::vector<int>>& indices,
            const int height, const int width)
        {
            int num = std::get<0>(indices).size();

            for (int i = 0; i < num; i++)
            {
                struct Point4f select_offset;

                select_offset.x = tx1[std::get<0>(indices)[i] * width + std::get<1>(indices)[i]];
                select_offset.y = tx2[std::get<0>(indices)[i] * width + std::get<1>(indices)[i]];
                select_offset.ex = ty1[std::get<0>(indices)[i] * width + std::get<1>(indices)[i]];
                select_offset.ey = ty2[std::get<0>(indices)[i] * width + std::get<1>(indices)[i]];

                offsets_inds.push_back(select_offset);
            }
        }

        void pnet_select_scores(std::vector<float>& select_scores,
            std::vector<float>& probs,
            const int probs_width, const int probs_height,
            std::tuple<std::vector<int>, std::vector<int>>& indices)
        {
            int num = std::get<0>(indices).size();

            for (int i = 0; i < num; i++)
            {
                int x = std::get<0>(indices)[i];
                int y = std::get<1>(indices)[i];
                select_scores.push_back(probs[x * probs_width + y]);
            }

        }

        void pnet_select_locations(std::vector<Point4f>& locations,    /*out*/
            const std::tuple<std::vector<int>, std::vector<int>>& indices, /*in*/
            const float scale                    /*in*/)
        {

            auto stride = std::make_pair(2, 5);
            auto cell_size = std::make_pair(12, 44);
            // indices max number;
            int num = std::get<0>(indices).size();

            for (int i = 0; i < num; i++)
            {
                struct Point4f local;

                float stride_1_indices_1 = static_cast<float>(stride.second * std::get<1>(indices)[i]) + 1.0;
                float stride_0_indices_0 = static_cast<float>(stride.first * std::get<0>(indices)[i]) + 1.0;
                local.x = std::round(stride_1_indices_1 / scale);
                local.y = std::round(stride_0_indices_0 / scale);
                local.ex = std::round((stride_1_indices_1 + cell_size.second) / scale);
                local.ey = std::round((stride_0_indices_0 + cell_size.first) / scale);
                locations.push_back(local);
            }
        }

        std::vector<size_t> argssort(const std::vector<float>& scores)
        {
            std::vector<size_t> ids(scores.size());
            std::iota(ids.begin(), ids.end(), 0);
            std::sort(ids.begin(), ids.end(), [&scores](size_t i1, size_t i2) {return scores[i1] < scores[i2]; });
            return ids;
        }

        std::vector<float> maximum(const float src, const std::vector<float>& local_ids)
        {
            std::vector<float> dst;
            for (int i = 0; i < local_ids.size(); i++)
            {
                if (src < local_ids[i])
                {
                    dst.push_back(local_ids[i]);
                }
                else
                {
                    dst.push_back(src);
                }
            }
            return dst;
        }

        std::vector<float> minimum(const float src, const std::vector<float>& local_ids)
        {
            std::vector<float> dst;
            for (int i = 0; i < local_ids.size(); i++)
            {
                if (src > local_ids[i])
                {
                    dst.push_back(local_ids[i]);
                }
                else
                {
                    dst.push_back(src);
                }
            }
            return dst;
        }

        std::vector<size_t> delete_larger(std::vector<size_t>& ids, std::vector<float>& overlap, const float overlap_threshold)
        {
            std::vector<size_t> dst;
            std::vector<size_t> overlap_ids;
            int temp = 0;
            for (auto key : overlap)
            {
                if (key > overlap_threshold)
                    overlap_ids.push_back(temp);
                temp++;
            }

            std::sort(overlap_ids.rbegin(), overlap_ids.rend());    // keep lowwer

            for (auto key = overlap_ids.begin(); key != overlap_ids.end();)
            {
                if (*key < ids.size())
                {
                    ids.erase(ids.begin() + *key);
                }
                key++;
            }

            return ids;
        }

        /**
        * @brief select roi size larger than threshold
        * @param keep   selected satisfy larger than overlap_threshold 0.5
        * @param boxes  bounding_boxes or bounding_boxes vector
        * @param overlap_threshold  float 0.5 or nums_threashold
        * @param mode   "union" or "min".
        */
        std::vector<size_t> nms(std::vector<Point4f>& locations, std::vector<float>& scores, float overlap_threshold = 0.5, std::string mode = "union")
        {
            std::vector<size_t> keep;
            if (scores.size() == 0)
            {
                keep.push_back(0);
                return keep;
            }

            int num = scores.size();

            std::vector<float> x1;
            std::vector<float> y1;
            std::vector<float> x2;
            std::vector<float> y2;

            for (auto& key : locations)
            {
                x1.push_back(key.x);
                y1.push_back(key.y);
                x2.push_back(key.ex);
                y2.push_back(key.ey);
            }

            std::vector<float> area;
            for (int i = 0; i < x1.size(); i++)
            {
                area.push_back((x2[i] - x1[i] + 1.0) * (y2[i] - y1[i] + 1.0));
            }

            std::vector<size_t> ids;
            ids = argssort(scores);

            while (ids.size() > 0)
            {
                int last = ids.size() - 1;
                size_t i = ids[last];
                keep.push_back(i);

                std::vector<float> x1_ids;
                std::vector<float> y1_ids;
                std::vector<float> x2_ids;
                std::vector<float> y2_ids;

                for (int j = 0; j < last; j++)
                {
                    x1_ids.push_back(x1[ids[j]]);
                    y1_ids.push_back(y1[ids[j]]);
                    x2_ids.push_back(x2[ids[j]]);
                    y2_ids.push_back(y2[ids[j]]);
                }

                // left top corner of intersection boxes
                std::vector<float> ix1;
                std::vector<float> iy1;
                ix1 = maximum(x1[i], x1_ids);
                iy1 = maximum(y1[i], y1_ids);

                // right bottom corner of intersection boxes
                std::vector<float> ix2;
                std::vector<float> iy2;
                ix2 = minimum(x2[i], x2_ids);
                iy2 = minimum(y2[i], y2_ids);

                // width and height of intersection boxes
                std::vector<float> w;
                std::vector<float> h;
                std::vector<float> ix2_ix1;
                std::vector<float> iy2_iy1;
                for (int j = 0; j < ix2.size(); j++)
                {
                    ix2_ix1.push_back(ix2[j] - ix1[j] + 1.0);
                    iy2_iy1.push_back(iy2[j] - iy1[j] + 1.0);
                }
                w = maximum(0.0, ix2_ix1);
                h = maximum(0.0, iy2_iy1);

                // intersections' areas
                std::vector<float> overlap;
                std::vector<float> inter;
                std::vector<float> area_min;

                for (int j = 0; j < w.size(); j++)
                {
                    inter.push_back(w[j] * h[j]);
                }

                std::vector<float> area_ids;
                for (int j = 0; j < last; j++)
                {
                    area_ids.push_back(area[ids[j]]);
                }

                if (mode == "min")
                {
                    area_min = minimum(area[i], area_ids);
                    for (int j = 0; j < inter.size(); j++)
                    {
                        overlap.push_back(inter[j] / area_min[j]);
                    }
                }
                else if (mode == "union")
                {
                    for (int j = 0; j < inter.size(); j++)
                    {
                        overlap.push_back(inter[j] / (area[i] + area_ids[j] - inter[j]));
                    }

                }
                // delete last one 
                ids.pop_back();
                // delete which larger than overlap  
                ids = delete_larger(ids, overlap, overlap_threshold);
            }
            return keep;
        }

        void calibrate_box(std::vector<Point4f>& out_locations, std::vector<Point4f>& keep_locations, std::vector<Point4f>& keep_offsets)
        {
            std::vector<float> x1;
            std::vector<float> y1;
            std::vector<float> x2;
            std::vector<float> y2;

            for (auto val : keep_locations)
            {
                x1.push_back(val.x);
                x2.push_back(val.ex);
                y1.push_back(val.y);
                y2.push_back(val.ey);
            }

            std::vector<float> w;
            std::vector<float> h;
            int num = keep_locations.size();
            for (int i = 0; i < num; i++)
            {
                w.push_back(x2[i] - x1[i] + 1.0);
                h.push_back(y2[i] - y1[i] + 1.0);
            }

            // translation
            std::vector<Point4f> translations;
            // hstack
            std::vector<Point4f>  hstack_wh;
            for (int i = 0; i < num; i++)
            {
                Point4f wh_point;
                wh_point.x = w[i];
                wh_point.ex = h[i];
                wh_point.y = w[i];
                wh_point.ey = h[i];
                hstack_wh.push_back(wh_point);
            }

            for (int i = 0; i < num; i++)
            {
                Point4f out_point;
                out_point.x = keep_locations[i].x + hstack_wh[i].x * keep_offsets[i].x;
                out_point.ex = keep_locations[i].ex + hstack_wh[i].ex * keep_offsets[i].ex;
                out_point.y = keep_locations[i].y + hstack_wh[i].y * keep_offsets[i].y;
                out_point.ey = keep_locations[i].ey + hstack_wh[i].ey * keep_offsets[i].ey;
                out_locations.push_back(out_point);
            }
        }

        std::tuple<std::vector<Point4f>, std::vector<Point4f>, std::vector<int>, std::vector<int>> correct_bboxes(
            std::vector<Point4f>& bboxes,
            std::vector<float>& scores,
            int width, int height)
        {
            int num_boxes = bboxes.size();

            std::vector<Point4f>  corrected;
            std::vector<int> w;
            std::vector<int> h;

            for (auto& key : bboxes)
            {
                struct Point4f correct_cut;
                correct_cut.x = key.x;
                correct_cut.y = key.y;

                // np.clip process
                if (key.ex < key.x)
                {
                    correct_cut.ex = key.x;
                }
                else
                {
                    correct_cut.ex = key.ex;
                }

                if (key.ey < key.y)
                {
                    correct_cut.ey = key.y;
                }
                else
                {
                    correct_cut.ey = key.ey;
                }
                corrected.push_back(correct_cut);
                w.push_back(key.ex - key.x + 1.0);
                h.push_back(key.ey - key.y + 1.0);
            }

            std::vector<Point4f>  coordinates;
            for (int i = 0; i < w.size(); i++)
            {
                struct Point4f coordi_cut;
                coordi_cut.x = 0.0f;
                coordi_cut.y = 0.0f;
                coordi_cut.ex = w[i] - 1.0;
                coordi_cut.ey = h[i] - 1.0;
                coordinates.push_back(coordi_cut);
            }

            for (auto key : corrected)
            {
                size_t index = 0;
                if (key.ex > (width - 1.0))           // if box's bottom right corner is too far right
                {
                    coordinates[index].ex = w[index] + width - 2.0 - key.ex;
                    key.ex = width - 1.0;
                }
                else if (key.ey > (height - 1.0))     // if box's bottom right corner is too low
                {
                    coordinates[index].ey = h[index] + height - 2.0 - key.ey;
                    key.ey = height - 1.0;
                }
                else if (key.x < 0.0)                 // if box's top left corner is too far left
                {
                    coordinates[index].x = 0.0 - key.x;
                    key.x = 0.0;
                }
                else if (key.y < 0.0)                 // if box's top left corner is too high
                {
                    coordinates[index].y = 0.0 - key.y;
                    key.y = 0.0;
                }
                index++;
            }

            auto cutouts = (std::make_tuple(corrected, coordinates, w, h));
            return cutouts;
        }

        std::vector<cv::Mat> cut_image_boxes(cv::Mat& image, std::vector<Point4f>& pnet_bboxes, std::vector<float>& pnet_scores)
        {
            std::vector<cv::Mat> img_boxes;
            auto size = std::make_pair(94, 24);
            int num_boxes = pnet_bboxes.size();

            int width = image.cols;
            int height = image.rows;

            auto cutouts = correct_bboxes(pnet_bboxes, pnet_scores, width, height);

            std::vector<Point4f> corrected;
            std::vector<Point4f> coordinates;
            std::vector<int> w;
            std::vector<int> h;
            std::tie(corrected, coordinates, w, h) = cutouts;

            for (int i = 0; i < num_boxes; i++)
            {
                cv::Mat img_box = cv::Mat(h[i], w[i], CV_8UC3);

                int corrected_y = corrected[i].y;
                int corrected_ey = corrected[i].ey + 1;
                int corrected_x = corrected[i].x;
                int corrected_ex = corrected[i].ex + 1;

                cv::Range corrected_ry = cv::Range(corrected_y, corrected_ey);
                cv::Range corrected_rx = cv::Range(corrected_x, corrected_ex);

                img_box = image(corrected_ry, corrected_rx);

                cv::resize(img_box, img_box, cv::Size(94, 24), 0, 0, cv::INTER_LINEAR_EXACT);

                img_boxes.push_back(img_box);
            }
            return img_boxes;
        }

        std::pair<std::vector<Point4f>, std::vector<float>> pnet_detect(cv::Mat& input_image, std::map<std::string, float>& param_map)
        {
            std::map<std::string, float> params = {
                  {"thresh", param_map.count("thresh") ? param_map["thresh"] : 0.5f},
                  {"nums_thresh",  param_map.count("nums_thresh") ? param_map["nums_thresh"] : 0.6f}};

            float thresh = params.at("thresh");
            float nums_thresh = params.at("nums_thresh");
            float overlap_threshold = 0.5;

            // scales
            int factor_times = 4;
            std::pair<int, int> min_lp_size{ 440, 140 };
            std::vector<float> scales;
            int input_width = input_image.cols;
            int input_height = input_image.rows;
            create_scales(scales, factor_times, input_height, input_width, min_lp_size);

            // bounding_boxes
            std::vector<Point4f> bounding_boxes_locations;
            std::vector<float>   bounding_boxes_scores;
            std::vector<Point4f> bounding_boxes_offsets;
            // 
            for (auto& key : scales)
            {
                int sw = std::ceil(input_width * key);
                int sh = std::ceil(input_height * key);

                cv::Mat scale_mat;
                cv::resize(input_image, scale_mat, cv::Size(sw, sh), 0, 0, cv::INTER_LINEAR_EXACT);

                // pnet 
                std::shared_ptr<glasssix::memory::tensor<uint8_t>> pnet_input_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, sh, sw, 3}, -1, glasssix::memory::NHWC));
                // mat convert into tensor
                std::copy(scale_mat.data, scale_mat.data + scale_mat.step[0] * scale_mat.rows, pnet_input_tensor_u8->mutable_cpu_data());

                // NHWC into NCHW tensor
                pnet_input_tensor_u8->convert_order();
                auto pnet_input_tensor = pnet_input_tensor_u8 | glasssix::memory::tensor_convert_to<float>;

                // pnet forward
                auto pnet_infer_output = pnet_instance_->forward(pnet_input_tensor);

                std::shared_ptr<glasssix::memory::tensor<float>> pnet_offset = pnet_infer_output["output"];
                std::shared_ptr<glasssix::memory::tensor<float>> pnet_prob = pnet_infer_output["25"];

                size_t probs_cstep = pnet_prob->count(2, 4);
                int probs_height = pnet_prob->height();
                int probs_width = pnet_prob->width();

                std::vector<float> pnet_probs(pnet_prob->cpu_data() + probs_cstep, pnet_prob->cpu_data() + probs_cstep * 2);

                size_t offsets_cstep = pnet_offset->count(2, 4);
                int offsets_height = pnet_offset->height();
                int offsets_width = pnet_offset->width();

                std::vector<float> pnet_offsets_c0(pnet_offset->cpu_data() + 0 * offsets_cstep, pnet_offset->cpu_data() + 1 * offsets_cstep);
                std::vector<float> pnet_offsets_c1(pnet_offset->cpu_data() + 1 * offsets_cstep, pnet_offset->cpu_data() + 2 * offsets_cstep);
                std::vector<float> pnet_offsets_c2(pnet_offset->cpu_data() + 2 * offsets_cstep, pnet_offset->cpu_data() + 3 * offsets_cstep);
                std::vector<float> pnet_offsets_c3(pnet_offset->cpu_data() + 3 * offsets_cstep, pnet_offset->cpu_data() + 4 * offsets_cstep);

                // select inds indices of boxes where there is probably a lp
                std::tuple<std::vector<int>, std::vector<int>> indices;         // tuple width and height

                pnet_select_indices(indices, pnet_probs, probs_width, probs_height, thresh);

                std::tuple<std::vector<Point4f>, std::vector<float>, std::vector<Point4f>> boxes;

                if (std::get<0>(indices).size() == 0)
                {
                    // all vector make into null;   
                    struct Point4f zero_local = { 0.0f, 0.0f, 0.0f, 0.0f };
                    struct Point4f zero_offset = { 0.0f, 0.0f, 0.0f, 0.0f };
                    std::vector<Point4f> locations = {zero_local};
                    std::vector<float> scores = {0.0f};
                    std::vector<Point4f> offsets = {zero_local};
                }
                else
                {
                    // select offset from offset
                    std::vector<Point4f> offsets_inds;
                    pnet_select_offsets(offsets_inds, pnet_offsets_c0, pnet_offsets_c1, pnet_offsets_c2, pnet_offsets_c3, indices, offsets_height, offsets_width);

                    // scores
                    std::vector<float> scores_inds;
                    pnet_select_scores(scores_inds, pnet_probs, probs_width, probs_height, indices);

                    // rescaled locations
                    std::vector<Point4f> locations_inds;
                    pnet_select_locations(locations_inds, indices, key);

                    // vstack bounding boxes
                    auto bounding_box = std::make_tuple(locations_inds, scores_inds, offsets_inds);

                    // Transpose bounding boxes into boxes
                    auto keep = nms(locations_inds, scores_inds, overlap_threshold);

                    // Transpose bounding boxes into boxes
                    std::vector<Point4f> boxes_locations;
                    std::vector<float>   boxes_scores;
                    std::vector<Point4f> boxes_offsets;
                    for (int i = 0; i < std::get<0>(bounding_box).size(); i++)
                    {
                        for (int j = 0; j < keep.size(); j++)
                        {
                            if (i == keep[j])
                            {
                                boxes_locations.push_back(std::get<0>(bounding_box)[i]);
                                boxes_scores.push_back(std::get<1>(bounding_box)[i]);
                                boxes_offsets.push_back(std::get<2>(bounding_box)[i]);
                            }
                        }
                    }
                    boxes = std::make_tuple(boxes_locations, boxes_scores, boxes_offsets);
                }

                // bounding_boxes only push boxes not None;
                if (std::get<1>(boxes).size() != 1)
                {
                    std::vector<Point4f> locations_temp;
                    std::vector<float> scores_temp;
                    std::vector<Point4f> offsets_temp;

                    std::tie(locations_temp, scores_temp, offsets_temp) = boxes;
                    bounding_boxes_locations.insert(bounding_boxes_locations.end(), locations_temp.begin(), locations_temp.end());
                    bounding_boxes_scores.insert(bounding_boxes_scores.end(), scores_temp.begin(), scores_temp.end());
                    bounding_boxes_offsets.insert(bounding_boxes_offsets.end(), offsets_temp.begin(), offsets_temp.end());
                }
            }

            std::vector<Point4f> keep_locations;
            std::vector<float> keep_scores;
            std::vector<Point4f> keep_offsets;

            if ((bounding_boxes_locations.size() != 0) && (bounding_boxes_scores.size() != 0) && (bounding_boxes_offsets.size() != 0))
            {
                auto bounding_boxes_keep = nms(bounding_boxes_locations, bounding_boxes_scores, nums_thresh);
                for (int j = 0; j < bounding_boxes_keep.size(); j++)
                {
                    for (int i = 0; i < bounding_boxes_locations.size(); i++)
                    {
                        if (bounding_boxes_keep[j] == i)
                        {
                            keep_locations.push_back(bounding_boxes_locations[i]);
                            keep_scores.push_back(bounding_boxes_scores[i]);
                            keep_offsets.push_back(bounding_boxes_offsets[i]);
                        }
                    }
                }

            }
            else
            {
                Point4f zero_point = { 0, 0, 0, 0 };
                keep_locations.push_back(zero_point);
                keep_scores.push_back(0);
                keep_offsets.push_back(zero_point);
            }

            // calibrate_box use offsets predicted by pnet to transform bounding boxes
            std::vector<Point4f> cali_locations;
            calibrate_box(cali_locations, keep_locations, keep_offsets);

            auto result = std::make_pair(cali_locations, keep_scores);
            return result;
        }

        std::pair<std::vector<Point4f>, size_t> onet_detect(cv::Mat& input_image, std::vector<Point4f>& pnet_detect_locations, std::vector<float>& pnet_detect_scores, std::map<std::string, float>& param_map)
        {
            std::map<std::string, float> params = {
                  {"thresh", param_map.count("thresh") ? param_map["thresh"] : 0.5f},
                  {"nums_thresh",  param_map.count("nums_thresh") ? param_map["nums_thresh"] : 0.6f} };

            float thresh = params.at("thresh");
            float nums_thresh = params.at("nums_thresh");
            // onet preprocess:
            int boxes_num = pnet_detect_locations.size();
            auto image_boxes = cut_image_boxes(input_image, pnet_detect_locations, pnet_detect_scores);
            // copy cv::Mat into tenoser

            std::shared_ptr<glasssix::memory::tensor<uint8_t>> image_boxes_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{boxes_num, 24, 94, 3}, -1, glasssix::memory::NHWC));;
            for (int i = 0; i < boxes_num; i++)
            {
                std::copy(image_boxes[i].data, image_boxes[i].data + image_boxes[i].step[0] * image_boxes[i].rows, image_boxes_tensor_u8->mutable_cpu_data() + i * image_boxes[i].step[0] * image_boxes[i].rows);
            }

            image_boxes_tensor_u8->convert_order();

            auto image_boxes_tensor = image_boxes_tensor_u8 | glasssix::memory::tensor_convert_to<float>;

            // onet forward
            std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> onet_infer_output = onet_instance_->forward(image_boxes_tensor);

            std::shared_ptr<glasssix::memory::tensor<float>> offset = onet_infer_output["output"];
            std::shared_ptr<glasssix::memory::tensor<float>> prob = onet_infer_output["47"];

            // probs = [false, true]
            std::vector<float> probs_temp(prob->cpu_data(), prob->cpu_data() + boxes_num * 2);

            std::vector<float> probs;
            for (int i = 0; i < probs_temp.size(); i++)
            {
                if (i % 2 != 0)
                {
                    probs.push_back(probs_temp[i]);
                }
            }

            // offsets 
            std::vector<float> offsets(offset->cpu_data(), offset->cpu_data() + boxes_num * 4);

            std::vector<Point4f> onet_detect_locations;

            // argmax
            auto keep = std::max_element(probs.begin(), probs.end()) - probs.begin();

            std::vector<Point4f> keep_locations;
            std::vector<Point4f> keep_offsets;

            if (probs[keep] > thresh)
            {
                Point4f keep_local;
                keep_local.x = pnet_detect_locations[keep].x;
                keep_local.y = pnet_detect_locations[keep].y;
                keep_local.ex = pnet_detect_locations[keep].ex;
                keep_local.ey = pnet_detect_locations[keep].ey;
                keep_locations.push_back(keep_local);

                Point4f keep_offset;
                keep_offset.x = offsets[keep * 4 + 0];
                keep_offset.y = offsets[keep * 4 + 1];
                keep_offset.ex = offsets[keep * 4 + 2];
                keep_offset.ey = offsets[keep * 4 + 3];
                keep_offsets.push_back(keep_offset);

                calibrate_box(onet_detect_locations, keep_locations, keep_offsets);
            }
            else
            {
                Point4f Zero_bboxes = { 0,0,0,0 };
                onet_detect_locations.push_back(Zero_bboxes);
            }

            return std::make_pair(onet_detect_locations, keep);
        }

        void imgBrightness(cv::Mat& blob, cv::Mat& enhanced, float c = 0.01, const int b = 0)
        {
            int rows = blob.rows;
            int cols = blob.cols;
            int channels = blob.channels();
            cv::Mat blank = cv::Mat::zeros(rows, cols, CV_8UC3);
            cv::addWeighted(blob, c, blank, 1 - c, b, enhanced);
        }

        std::vector<cv::Point> selectCorners(std::vector<std::vector<cv::Point>>& find_contours)
        {
            std::vector<cv::Point> result;

            if (find_contours.size() > 0)
            {
                // y_x and y__x
                std::vector<float> y_x;
                std::vector<float> y__x;

                for (auto key : find_contours[0])
                {
                    y_x.push_back(key.y - key.x);
                    y__x.push_back(key.x + key.y);
                }

                int ind_tr = std::min_element(y_x.begin(), y_x.end()) - y_x.begin();
                int ind_bl = std::max_element(y_x.begin(), y_x.end()) - y_x.begin();

                int ind_tl = std::min_element(y__x.begin(), y__x.end()) - y__x.begin();
                int ind_br = std::max_element(y__x.begin(), y__x.end()) - y__x.begin();

                result.push_back(find_contours[0][ind_tl]);
                result.push_back(find_contours[0][ind_tr]);
                result.push_back(find_contours[0][ind_br]);
                result.push_back(find_contours[0][ind_bl]);
            }
            else
            {
                cv::Point pi = { 0,0 };
                result.push_back(pi);
            }
            return result;
        }

        std::vector<cv::Point> findCorners(cv::Mat& blob)
        {
            cv::Mat gray_image;
            cv::cvtColor(blob, gray_image, cv::COLOR_BGR2GRAY);
            auto lightness = cv::mean(gray_image);
            float lightness_left = std::pow((170.0f / static_cast<int>(lightness[0])), 1.5);
            float c = 3;
            if (lightness_left - c < 0)
            {
                c = lightness_left;
            }
            cv::Mat enhanced;
            imgBrightness(blob, enhanced, c);

            // Use binary gray image and erode
            cv::cvtColor(enhanced, gray_image, cv::COLOR_BGR2GRAY);
            cv::Mat thres;
            cv::threshold(gray_image, thres, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);
            // morphologyEx
            cv::Mat mor_kernel = cv::Mat::ones(1, 5, CV_8UC1);
            cv::morphologyEx(thres, thres, cv::MORPH_CLOSE, mor_kernel, cv::Point(-1, -1), 2);

            // Denoise
            cv::medianBlur(thres, thres, 5);
            cv::blur(thres, thres, cv::Size(5, 5));

            // Find contour points and select four corners
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(thres, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            auto result = selectCorners(contours);
            return result;

        }

        std::vector<cv::Point>  retractROI(std::vector<cv::Point>& corntours, Point4f& blob_bboxes)
        {
            std::vector<cv::Point> retractroi(4);
            retractroi[0].x = blob_bboxes.x + corntours[0].x;
            retractroi[0].y = blob_bboxes.y + corntours[0].y;
            retractroi[1].x = blob_bboxes.x + corntours[1].x;
            retractroi[1].y = blob_bboxes.y + corntours[1].y;
            retractroi[2].x = blob_bboxes.x + corntours[2].x;
            retractroi[2].y = blob_bboxes.y + corntours[2].y;
            retractroi[3].x = blob_bboxes.x + corntours[3].x;
            retractroi[3].y = blob_bboxes.y + corntours[3].y;
            return retractroi;
        }

        void transformImage(cv::Mat& aligned_image, std::vector<cv::Point>& retract_locations, cv::Mat& input_image, std::pair<int, int>& lp_size, std::pair<int, int>& base, std::pair<int, int>& size)
        {
            // pers_tf;
            // cv::Mat pts_ortho = (cv::Mat_<cv::Point2f>(2, 2) << (base.first, base.first), (base.first + lp_size.first, base.first), (base.first + lp_size.second, base.second + lp_size.second), (base.first + base.second + lp_size.second));
            std::vector<cv::Point2f> retract_locations_f;
            for (auto val : retract_locations)
            {
                cv::Point2f pf;
                pf.x = static_cast<float>(val.x);
                pf.y = static_cast<float>(val.y);
                retract_locations_f.push_back(pf);
            }
            std::vector<cv::Point2f> pts_ortho_f = { cv::Point2f(base.first, base.first), cv::Point2f(base.first + lp_size.first, base.first), cv::Point2f(base.first + lp_size.first, base.second + lp_size.second), cv::Point2f(base.first, base.second + lp_size.second) };
            auto transform_matrix = cv::getPerspectiveTransform(retract_locations_f, pts_ortho_f);
            cv::warpPerspective(input_image, aligned_image, transform_matrix, cv::Size(lp_size.first, lp_size.second));
        }

        std::pair<cv::Mat, cv::Rect> find_corners(cv::Mat& input_image, std::vector<Point4f>& onet_detect_locations)
        {
            cv::Mat aligned_image;
            cv::Rect rect_location;
            // range
            int x = static_cast<int>(std::round(onet_detect_locations[0].x));
            int ex = static_cast<int>(std::round(onet_detect_locations[0].ex));
            int y = static_cast<int>(std::round(onet_detect_locations[0].y));
            int ey = static_cast<int>(std::round(onet_detect_locations[0].ey));
            cv::Range x_ex = cv::Range(x, ex);
            cv::Range y_ey = cv::Range(y, ey);
            cv::Mat blob = input_image(y_ey, x_ex);
            //  Use only license plate area to detect corners

            auto result_corners = findCorners(blob);

            if (result_corners.size() != 0)
            {
                // Draw corners on the full - size image
                auto roi_locations = retractROI(result_corners, onet_detect_locations[0]);    // new

                // Align image according to corners
                auto lp_size = std::make_pair(440, 140);
                auto align_base = std::make_pair(0, 0);
                auto align_size = std::make_pair(440, 140);
                transformImage(aligned_image, roi_locations, input_image, lp_size, align_base, align_size);
            }
            else
            {
                std::cout << "Corner detection failed in \n";
            }

            rect_location.x = x;
            rect_location.y = y;
            rect_location.width = ex - x;
            rect_location.height = ey - y;

            return std::make_pair(aligned_image, rect_location);
        }

        std::vector<int> row_sum(cv::Mat& binary_img)
        {
            int width = binary_img.cols;
            int height = binary_img.rows;
            int ch = binary_img.channels();

            std::vector<int> row_result(height);
            for (int i = 0; i < height; i++)
            {
                int sum_temp = 0;
                for (int j = 0; j < width; j++)
                {
                    sum_temp += (int)binary_img.at<uchar>(i, j);
                }
                row_result[i] = sum_temp;
            }
            return row_result;
        }

        cv::Mat remove_border(cv::Mat& aligned_image)
        {
            cv::Mat aligned_images_cut_left;
            cv::Mat plate_gray;
            cv::cvtColor(aligned_image, plate_gray, cv::COLOR_BGR2GRAY);
            cv::Mat plate_binary_img;
            cv::threshold(plate_gray, plate_binary_img, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);

            auto rowsum = row_sum(plate_binary_img);
            std::vector<int> row_histogram_top(70);
            row_histogram_top.assign(rowsum.begin(), rowsum.begin() + 70);
            std::vector<int> row_histogram_down(70);
            row_histogram_down.assign(rowsum.begin() + 70, rowsum.end());
            int top_y = std::min_element(row_histogram_top.begin(), row_histogram_top.end()) - row_histogram_top.begin();
            int down_y = std::min_element(row_histogram_down.begin(), row_histogram_down.end()) - row_histogram_down.begin() + 70;

            if (top_y > 10)
            {
                top_y = 10;
            }

            if (down_y < 120)
            {
                down_y = 139;
            }

            // cut aligned images
            cv::Range cut_range = cv::Range(top_y, down_y);
            cv::Mat aligned_images_cut = aligned_image(cut_range, cv::Range::all());

            cv::Mat gray_plate;
            cv::cvtColor(aligned_images_cut, gray_plate, cv::COLOR_BGR2GRAY);
            cv::Mat binary_plate;
            cv::threshold(gray_plate, binary_plate, 0, 255, cv::THRESH_OTSU);

            std::vector<float> result(60);
            for (int i = 0; i < 60; i++)
            {
                for (int j = 0; j < binary_plate.rows; j++)
                {
                    result[i] += (float)binary_plate.at<uchar>(j, i) / 255;
                }
            }

            size_t n = std::min_element(result.begin(), result.end()) - result.begin();

            if (result[n] < 30)
            {
                aligned_images_cut_left = aligned_images_cut(cv::Range::all(), cv::Range(n, aligned_image.cols));
            }
            else
            {
                aligned_images_cut_left = aligned_images_cut;
            }

            return aligned_images_cut_left;
        }

        // char_segment_classfi
        std::string char_segment_classfi(cv::Mat& aligned_image)
        {
            std::string plate;
            // char segment
            int cut_width = aligned_image.cols;
            float scale = (float)cut_width / 440.0;
            std::vector<int> seg_index;
            for (int i = 0; i < 8; i++)
            {
                float temp = base_seg_index[i] * scale;
                seg_index.push_back(temp);
            }
            cv::Mat chinese_img = cv::Mat(aligned_image, cv::Range::all(), cv::Range(seg_index[0], seg_index[1] - 1));
            cv::Mat rec_chinese_img;
            cv::resize(chinese_img, rec_chinese_img, cv::Size(32, 64));

            // copy cv::mat into tensor;
            std::shared_ptr<glasssix::memory::tensor<uint8_t>> chinese_img_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, 64, 32, 3}, -1, glasssix::memory::NHWC));
            std::copy(rec_chinese_img.data, rec_chinese_img.data + rec_chinese_img.step[0] * rec_chinese_img.rows, chinese_img_tensor_u8->mutable_cpu_data());

            chinese_img_tensor_u8->convert_order();

            auto chinese_img_tensor = chinese_img_tensor_u8 | glasssix::memory::tensor_convert_to<float>;

            auto chinese_classfi_output = resnet_chinese_instance_->forward(chinese_img_tensor);

            std::vector<float> chinese_detections(chinese_classfi_output["output"]->cpu_data(), chinese_classfi_output["output"]->cpu_data() + chinese_classfi_output["output"]->count());

            auto chinese_biggest_index = std::distance(chinese_detections.begin(), std::max_element(chinese_detections.begin(), chinese_detections.end()));

            plate += chinese_label_index1[chinese_biggest_index];
            plate += "_";

            for (int i = 1; i < 7; i++)
            {
                cv::Mat char_img = cv::Mat(aligned_image, cv::Range::all(), cv::Range(seg_index[i], seg_index[i + 1] - 1));
                cv::Mat rec_char_img;
                cv::resize(char_img, rec_char_img, cv::Size(32, 64));
                // copy cv::mat into tensor;
                std::shared_ptr<glasssix::memory::tensor<uint8_t>> char_img_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, 64, 32, 3}, -1, glasssix::memory::NHWC));
                std::copy(rec_char_img.data, rec_char_img.data + rec_char_img.step[0] * rec_char_img.rows, char_img_u8->mutable_cpu_data());

                char_img_u8->convert_order();

                auto char_img_tensor = char_img_u8 | glasssix::memory::tensor_convert_to<float>;

                auto char_classfi_output = resnet_char_instance_->forward(char_img_tensor);

                std::vector<float> char_detections(char_classfi_output["output"]->cpu_data(), char_classfi_output["output"]->cpu_data() + char_classfi_output["output"]->count());

                if (i == 1)
                {
                    auto char_biggest_index = std::distance(char_detections.begin() + 10, std::max_element(char_detections.begin() + 10, char_detections.end()));

                    plate += (char_label_index[char_biggest_index + 10]);
                    plate += "_";
                }
                else
                {
                    auto char_biggest_index = std::distance(char_detections.begin(), std::max_element(char_detections.begin(), char_detections.end()));

                    plate += (char_label_index[char_biggest_index]);
                }
            }

            return plate;
        }

        box_info_internal run_detect_classfi(std::vector<int>& roi, std::map<std::string, float>& param_map)
        {
            // cut roi image
            glasssix::excalibur::rectangle<int> rect((int)roi[0], (int)roi[1], (int)roi[2], (int)roi[3]);
            std::shared_ptr<glasssix::memory::tensor<uint8_t>> input;

            glasssix::excalibur::safty_cut_cpu(cache0_, input, &rect);
            cv::Mat input_mat = cv::Mat(1, input->height(), input->width(), 3);
            std::memcpy(input_mat.data, input->cpu_data(), input->count(2, 4));

            cv::Mat preprocess_input_mat;
            cv::cvtColor(input_mat, preprocess_input_mat, cv::COLOR_BGR2RGB);

            // step 1 detect pnet
            auto pnet_result = pnet_detect(preprocess_input_mat, param_map);

            std::vector<Point4f> pnet_detect_locations;
            std::vector<float> pnet_detect_scores;
            std::tie(pnet_detect_locations, pnet_detect_scores) = pnet_result;

            // onet
            auto onet_result = onet_detect(preprocess_input_mat, pnet_detect_locations, pnet_detect_scores, param_map);

            std::vector<Point4f> onet_detect_locations;
            size_t keep;
            std::tie(onet_detect_locations, keep) = onet_result;

            if ((onet_detect_locations[0].x == 0) && (onet_detect_locations[0].y == 0))
            {
                box_info_internal box;

                box.rect.x = 0;
                box.rect.x = 0;
                box.rect.w = 0;
                box.rect.h = 0;

                box.strinfos = glasssix::exposing::param_string("");

                auto temp_vec = glasssix::exposing::make_param_vector<std::uint8_t>();
                box.aligned_images = temp_vec;

                return box;
            }

            // step 2 find Corners
            auto corners_result = find_corners(input_mat, onet_detect_locations);
            cv::Mat aligned_image;
            cv::Rect corn_locations;
            std::tie(aligned_image, corn_locations) = corners_result;

            // step 3  cut border 
            auto aligned_images_cut_border = remove_border(aligned_image);

            // step 4 char seg and classfi
            auto plate = char_segment_classfi(aligned_image);

            // step 5 make return
            box_info_internal box;

            box.rect.x = corn_locations.x;
            box.rect.x = corn_locations.y;
            box.rect.w = corn_locations.width;
            box.rect.h = corn_locations.height;

            box.score = pnet_detect_scores[keep];

            auto strinfos = glasssix::exposing::param_string(plate);

            box.strinfos = strinfos;

            // save Align image into uint8 vector
            auto temp_vec = glasssix::exposing::make_param_vector<std::uint8_t>();
            int aligned_image_size = aligned_image.channels() * aligned_image.rows * aligned_image.cols;
            temp_vec.resize(static_cast<size_t>(aligned_image_size));
            temp_vec.copy_from({ aligned_image.data , static_cast<size_t>(aligned_image_size) }, 0);

            box.aligned_images = temp_vec;

            return box;
        }

    private:
        std::string model_directory_;
        int device_;
        std::shared_ptr<glasssix::memory::tensor<std::uint8_t>> cache0_;
        std::shared_ptr<glasssix::memory::tensor<std::uint8_t>> cache1_;
        std::unique_ptr<glasssix::excalibur::pipeline<float>> pnet_instance_;
        std::unique_ptr<glasssix::excalibur::pipeline<float>> onet_instance_;
        std::unique_ptr<glasssix::excalibur::pipeline<float>> resnet_chinese_instance_;
        std::unique_ptr<glasssix::excalibur::pipeline<float>> resnet_char_instance_;
    };

    ocr_code_internal::ocr_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    ocr_code_internal::~ocr_code_internal()
    {
    }

    std::string ocr_code_internal::version()
    {
        return impl::version();
    }

    exposing::param_vector<box_info> ocr_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order,
        int x, int y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, order, x, y, roi_width, roi_height, param_map);
    }

    box_info glasssix::plate::ocr_code_internal::trace(box_info plate, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
    {
        return impl_->trace(plate, bitmap, channels, height, width, order);
    }
}