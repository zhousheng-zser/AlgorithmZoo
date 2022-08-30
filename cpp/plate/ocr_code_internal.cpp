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
#include <opencv2/core/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <abi/param_vector.hpp>

// struct for locations
typedef struct Point4f
{
    float x;        // x1
    float ex;       // x2
    float y;        // y1
    float ey;       // y2
} box;

// char_seg_classfi
const static int seg_index[] = { 0, 17, 35, 50, 65, 80, 95, 110 };

const static char chinese_label_index[100] = { '蒙', '晋' };
// "冀", "宁", "甘","赣", "鲁", "豫", "京", "沪", "津", "渝","辽", "吉", "黑", "苏", "浙", "皖", "闽", "鄂", "湘", "粤", "桂", "琼", "川", "贵", "云", "藏", "陕", "青", "新"};

const static char char_label_index[] = { '0', '1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G','H','I',
    'J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z' };


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
            chinese_classfi_instance_ = std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params("plate_det_chinese", false), std::string(model_directory) + "/" + "res20_chinese_sim" + ".racy", device);
            char_classfi_instance_    = std::make_unique<excalibur::pipeline<float>>(hardcode::get_model_params("plate_det_char", false),    std::string(model_directory) + "/" + "res20_char_sim"    + ".racy", device);
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
            init_cache(bitmap, channels, height, width, order, roi);

            std::vector<box_info_internal> results;

            auto result = exposing::make_param_vector<box_info>();

            // run pnet
            // run_net(results, roi, param_map);

            for (auto i : results)
            {
                result.push_back(glasssix::exposing::make_as_first<box_info_impl>(i));
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

        }
        

        void create_scales(std::vector<float>& scales, int min_height, int min_width, std::pair<int, int>& min_lp_size)
        {
            float factor = 0.707;   // sqrt(1.5)
            int factor_count = 0;

            auto height = static_cast<float>(min_height);
            auto width = static_cast<float>(min_width);

            while ((min_height > min_lp_size.second) && (min_width > min_lp_size.first))
            {
                scales.push_back(pow(factor, factor_count));
                height *= factor;
                width *= factor;
                factor_count += 1;
            }
        }

        void pnet_select_indices(std::tuple<std::vector<int>, std::vector<int>>& indices, std::shared_ptr<glasssix::memory::tensor<float>>& prob, const float thresh)
        {
            int probs_size = prob->count(2, 4);
            std::vector<float> probs(probs_size);
            memcpy(&probs[0], prob->cpu_data(), probs_size * sizeof(float));

            std::vector<int> indices_w;
            std::vector<int> indices_h;
            int width = static_cast<int>(prob->width());
            int height = static_cast<int>(prob->height());

            for (int i = 0; i < width; i++)
            {
                for (int j = 0; j < height; j++)
                {
                    if (probs[i * height + j] > thresh)
                    {
                        indices_w.push_back(i);
                        indices_h.push_back(j);
                    }
                }
            }
            indices = std::make_tuple(indices_w, indices_h);
        }

        void pnet_select_offsets(std::vector<Point4f>& offsets_inds, std::shared_ptr<glasssix::memory::tensor<float>>& offset, std::tuple<std::vector<int>, std::vector<int>>& indices)
        {
            // �ܳ���
            int num = std::get<0>(indices).size();
            // ��offset��ȡ��һ��ָ��ֵ
            // copy tensor into vector<Point4f>
            std::vector<Point4f> offsets(offset->count(1, 4));
            memcpy(&offsets[0], offset->cpu_data(), offset->count(1, 4) * sizeof(float));
            int width = offset->width();
            int height = offset->height();
            int offset_cstep = offset->count(2, 4);
            for (int i = 0; i < num; i++)
            {
                struct Point4f t_offset;

                t_offset.x = 0 * offset_cstep + std::get<0>(indices)[i] * height + std::get<1>(indices)[i];
                t_offset.ex = 1 * offset_cstep + std::get<0>(indices)[i] * height + std::get<1>(indices)[i];
                t_offset.y = 2 * offset_cstep + std::get<0>(indices)[i] * height + std::get<1>(indices)[i];
                t_offset.ey = 3 * offset_cstep + std::get<0>(indices)[i] * height + std::get<1>(indices)[i];

                offsets_inds.push_back(t_offset);
            }
        }

        void pnet_select_scores(std::vector<float>& select_scores, std::shared_ptr<glasssix::memory::tensor<float>>& prob, std::tuple<std::vector<int>, std::vector<int>>& indices)
        {
            // �ܳ���
            int num = std::get<0>(indices).size();

            int probs_size = prob->count(2, 4);
            std::vector<float> probs(probs_size);
            memcpy(&probs[0], prob->cpu_data(), probs_size * sizeof(float));

            // ��offsets��ȡ��һ��ָ��ֵ
            // step1 copy tensor into Mat
            
            for (int i = 0; i < num; i++)
            {
                int score_cstep = static_cast<float>(prob->height() + std::get<1>(indices)[i]);
                select_scores.push_back(probs[static_cast<int>(std::get<0>(indices)[i] * score_cstep)]);
            }

        }

        void pnet_select_locations(std::vector<Point4f>& locations,    /*out*/
            const std::tuple<std::vector<int>, std::vector<int>>& indices, /*in*/
            const float scale,                    /*in*/
            const std::pair<int, int>& stride,     /*in*/
            const std::pair<int, int>& cell_size   /*in*/)
        {
            // mat merge ��ʽ
            // indices max number;
            int num = std::get<0>(indices).size();

            for (int i = 0; i < num; i++)
            {
                struct Point4f local;

                float stride_1_indices_1 = static_cast<float>(stride.second * std::get<1>(indices)[i]) + 1.0;
                float stride_0_indices_0 = static_cast<float>(stride.first * std::get<0>(indices)[i]) + 1.0;
                local.x  = std::round(stride_1_indices_1 / scale);
                local.ex = std::round(stride_0_indices_0 / scale);
                local.y  = std::round((stride_1_indices_1 + cell_size.second) / scale);
                local.ey = std::round((stride_0_indices_0 + cell_size.first)  / scale);
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

            for (int i = 0; i < ids.size(); i++)
            {
                for (int j = 0; j < overlap_ids.size(); j++)
                {
                    if (i != overlap_ids[j])
                        dst.push_back(ids[i]);
                }
            }
            return dst;
        }

        void det_pnet_infer(std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& out_, /*in*/
            const std::map<std::string, float>& params,   /*in*/
            const float scale,                            /*in*/
            std::tuple<std::vector<Point4f>, std::vector<float>, std::vector<Point4f>>& bboxes/*out*/)
        {
            float thresh = params.at("thresh");
            float nums_thresh = params.at("nums_thresh");

            auto stride = std::make_pair(2, 5);
            auto cell_size = std::make_pair(12, 44);

            std::shared_ptr<glasssix::memory::tensor<float>> pnet_offset = out_["offset"];
            std::shared_ptr<glasssix::memory::tensor<float>> pnet_prob = out_["prob"];

            // select inds indices of boxes where there is probably a lp
            std::tuple<std::vector<int>, std::vector<int>> indices;         // tuple width and height
            pnet_select_indices(indices, pnet_prob, thresh);

            if (std::get<0>(indices).size() == 0)
            {
                // all vector make into null;   
                std::vector<Point4f> locations;
                std::vector<float> scores;
                std::vector<Point4f> offsets;
                struct Point4f zero_local = { 0.0f, 0.0f, 0.0f, 0.0f };
                struct Point4f zero_offset = { 0.0f, 0.0f, 0.0f, 0.0f };

                locations.push_back(zero_local);
                scores.push_back(0.0f);
                offsets.push_back(zero_offset);
                bboxes = std::make_tuple(locations, scores, offsets);
            }
            else
            {
                // select offset from offset
                std::vector<Point4f> offsets_inds;
                pnet_select_offsets(offsets_inds, pnet_offset, indices);

                // scores
                std::vector<float> scores_inds;
                pnet_select_scores(scores_inds, pnet_prob, indices);

                // rescaled locations
                std::vector<Point4f> locations_inds;
                pnet_select_locations(locations_inds, indices, scale, stride, cell_size);

                // merge bounding boxes
                auto bounding_boxes = std::make_tuple(locations_inds, scores_inds, offsets_inds);

                
                // 
                // Transpose bounding boxes into boxes
                std::vector<size_t> keep;
                keep = nms(bounding_boxes);
                std::vector<Point4f> bboxes_locations;
                std::vector<float>   bboxes_scores;
                std::vector<Point4f> bboxes_offsets;
                for (int i = 0; i < std::get<0>(bounding_boxes).size(); i++)
                {
                    for (int j = 0; j < keep.size(); j++)
                    {
                        if (i == keep[j])
                        {
                            bboxes_locations.push_back(std::get<0>(bounding_boxes)[i]);
                            bboxes_scores.push_back(std::get<1>(bounding_boxes)[i]);
                            bboxes_offsets.push_back(std::get<2>(bounding_boxes)[i]);
                        }
                    }
                }
                bboxes = std::make_tuple(bboxes_locations, bboxes_scores, bboxes_offsets);
            }
        }

        void det_onet_infer(std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& out_, /*in*/
            std::vector<Point4f> bboxes,  /*in*/
            const std::map<std::string, float>& params,                   /*in*/
            std::vector<Point4f> locations,  /*out*/
            std::vector<float>   out_scores)  /*out*/
        {
            float thresh = params.at("thresh");
            float nums_thresh = params.at("nums_thresh");

            std::shared_ptr<glasssix::memory::tensor<float>> offset = out_["offset"];   // nx4xhxw
            std::shared_ptr<glasssix::memory::tensor<float>> prob = out_["prob"];     // nx2xhxw

            // ����probs
            int probs_size = prob->count(2, 4);
            std::vector<float> probs(probs_size);
            memcpy(&probs[0], prob->cpu_data(), probs_size * sizeof(float));

            // ����offsets
            int offset_size = offset->count(1, 4);
            std::vector<Point4f> offsets(offset_size);
            memcpy(&offsets[0], offset->cpu_data(), offset_size * sizeof(float));

            std::vector<int> onet_keep;

            for (int i = 0; i < probs.size(); i++)
            {
                if (probs[i] > thresh)
                    onet_keep.push_back(i);
            }

            std::vector<Point4f> bboxes_local = bboxes;
            std::vector<Point4f> onet_bboxes;
            std::vector<Point4f> onet_offsets;

            for (auto key : onet_keep)
            {
                onet_bboxes.push_back(bboxes_local[key]);
                out_scores.push_back(probs[key]);
                onet_offsets.push_back(offsets[key]);
            }

            // 163
            auto onet_output_bboxes = calibrate_box(onet_bboxes, onet_offsets);
            std::string mode = "min";
            auto output_keep = nms(std::make_tuple(onet_output_bboxes, out_scores, onet_offsets), nums_thresh, mode);

            // selected keep;
            std::vector<Point4f> output_bboxes;
            for (auto key : output_keep)
            {
                output_bboxes.push_back(onet_output_bboxes[key]);
            }

            // locations = std::round
            for (auto key : output_bboxes)
            {
                struct Point4f out_local;
                out_local.x  = std::round(key.x);
                out_local.y  = std::round(key.y);
                out_local.ex = std::round(key.ex);
                out_local.ey = std::round(key.ey);
                locations.push_back(out_local);
            }
        }

        void selectCorners(std::vector<cv::Point>& contours, std::vector<std::vector<cv::Point> >& find_contours)
        {
            // ���α�ѡ���ֵ;
            std::vector<cv::Point> cont = find_contours[0];

            // y_x and y__x
            std::vector<float> y_x;
            std::vector<float> y__x;

            for (auto key : cont)
            {
                y_x.push_back(key.y - key.x);
                y__x.push_back(key.x + key.y);
            }

            int ind_tr = std::min_element(y_x.begin(), y_x.end()) - y_x.begin();
            int ind_bl = std::max_element(y_x.begin(), y_x.end()) - y_x.begin();

            int ind_tl = std::min_element(y__x.begin(), y__x.end()) - y__x.begin();
            int ind_br = std::max_element(y__x.begin(), y__x.end()) - y__x.begin();


            contours.push_back(cont[ind_tl]);
            contours.push_back(cont[ind_tr]);
            contours.push_back(cont[ind_br]);
            contours.push_back(cont[ind_bl]);
        }

        std::vector<cv::Point> findCorners(cv::Mat& blob)
        {
            cv::Mat enhance_image;
            cv::normalize(blob, enhance_image, 0, 255, cv::NORM_MINMAX);
            cv::Mat gray_image;
            cv::cvtColor(enhance_image, gray_image, cv::COLOR_BGR2GRAY);
            cv::Mat hsl_image;
            cv::cvtColor(enhance_image, hsl_image, cv::COLOR_BGR2HLS);
            cv::Mat lower_bound2 = (cv::Mat_<int>(1, 3) << 20 * 179 / 239, 0, 0);
            cv::Mat upper_bound2 = (cv::Mat_<int>(1, 3) << 50 * 179 / 239, 255, 255);
            cv::Mat mask2;
            cv::inRange(hsl_image, lower_bound2, upper_bound2, mask2);
            cv::Mat masked;
            cv::bitwise_and(gray_image, gray_image, masked, mask2);

            // Use binary gray image and erode
            cv::Mat thres;
            cv::threshold(masked, thres, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);
            cv::Mat thres_morph;
            cv::Mat kernel = (cv::Mat_<int>(1, 5) << 1, 1, 1, 1, 1);
            int iterations = 2;
            cv::morphologyEx(thres, thres_morph, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), iterations);

            // Denoise
            cv::Mat thres_Gauss;
            cv::GaussianBlur(thres_morph, thres_Gauss, cv::Size(5, 5), 0);
            cv::Mat thres_median;
            cv::medianBlur(thres_Gauss, thres_median, 15);

            // Find contour pointsand select four corners
            std::vector<std::vector<cv::Point> > find_contours;
            std::vector<cv::Vec4i> hierarchy;
            cv::findContours(thres_median, find_contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
            std::vector<cv::Point> contours;
            selectCorners(contours, find_contours);
            return contours;
        }

        std::vector<cv::Point> retractROI(std::vector<cv::Point>& corntours, std::vector<int>& roi, Point4f& blob_bboxes)
        {
            std::vector< cv::Point> retractroi(4);
            retractroi[0].x = blob_bboxes.x + roi[0] + corntours[0].x;
            retractroi[0].y = blob_bboxes.y + roi[1] + corntours[0].y;
            retractroi[1].x = blob_bboxes.x + roi[0] + corntours[1].x;
            retractroi[1].y = blob_bboxes.y + roi[1] + corntours[1].y;
            retractroi[2].x = blob_bboxes.x + roi[0] + corntours[2].x;
            retractroi[2].y = blob_bboxes.y + roi[1] + corntours[2].x;
            retractroi[3].x = blob_bboxes.x + roi[0] + corntours[3].y;
            retractroi[3].y = blob_bboxes.y + roi[1] + corntours[3].y;
            return retractroi;
        }

        void transformImage(cv::Mat& aligned_image, std::vector<cv::Point>& retract_locations, std::pair<int, int>& lp_size, std::pair<int, int>& base, std::pair<int, int>& size, cv::Mat& input_mat)
        {
            // pers_tf;
            cv::Mat pts_ortho = (cv::Mat_<cv::Point2f>(2, 2) << (base.first, base.first), (base.first + lp_size.first, base.first), (base.first + lp_size.second, base.second + lp_size.second), (base.first + base.second + lp_size.second));
            auto transform_matrix = cv::getPerspectiveTransform(retract_locations, pts_ortho);
            cv::warpPerspective(input_mat, aligned_image, transform_matrix, cv::Size(800, 800));
        }


        //  char_segment_classfi
        std::string char_segment_classfi(cv::Mat& aligned_image, std::unique_ptr<glasssix::excalibur::pipeline<float>>& chinese_classfi_instance, std::unique_ptr<glasssix::excalibur::pipeline<float>>& char_classfi_instance)
        {
            // char segment
            // cut image into img;
            std::string s;

            auto pad_size = std::make_pair(64, 48);
            cv::Mat chinese_img = cv::Mat(aligned_image, cv::Range::all(), cv::Range(seg_index[0],seg_index[1]-1));
            cv::Mat chinese_img_border;
            cv::copyMakeBorder(chinese_img, chinese_img_border, pad_size.first - chinese_img.cols, 0, pad_size.second - chinese_img.rows, 0, cv::BORDER_CONSTANT, cv::Scalar{ 0, 0, 0 });
            // copy cv::Mat into tensor;
            std::shared_ptr<memory::tensor<uint8_t>> chinese_img_tensor_u8(new memory::tensor<uint8_t>(std::vector<int>{1, chinese_img_border.cols, chinese_img_border.rows, 3}, -1, memory::NHWC));
            std::copy(chinese_img_border.data, chinese_img_border.data + chinese_img_border.step[0] * chinese_img_border.rows, chinese_img_tensor_u8->mutable_cpu_data());

            chinese_img_tensor_u8->convert_order();

            auto chinese_img_tensor = chinese_img_tensor_u8 | memory::tensor_convert_to<float>;

            auto chinese_classfi_output = chinese_classfi_instance->forward(chinese_img_tensor);

            std::vector<float> chinese_detections(chinese_classfi_output["output"]->cpu_data(), chinese_classfi_output["output"]->cpu_data() + chinese_classfi_output["output"]->count());

            auto chinese_biggest_index = std::distance(chinese_detections.begin(), std::max_element(chinese_detections.begin(), chinese_detections.end()));

            s += chinese_label_index[chinese_biggest_index];

            for (int i = 1; i < 7; i++)
            {
                cv::Mat char_img = cv::Mat(aligned_image, cv::Range::all(), cv::Range(seg_index[i], seg_index[i+1] - 1));
                cv::Mat char_img_border;
                cv::copyMakeBorder(char_img, char_img_border, pad_size.first - char_img.cols, 0, pad_size.second - char_img.rows, 0, cv::BORDER_CONSTANT, cv::Scalar{ 0, 0, 0 });
                // copy cv::Mat into tensor;
                std::shared_ptr<memory::tensor<uint8_t>> char_img_u8(new memory::tensor<uint8_t>(std::vector<int>{1, char_img_border.cols, char_img_border.rows, 3}, -1, memory::NHWC));
                std::copy(char_img_border.data, char_img_border.data + char_img_border.step[0] * char_img_border.rows, char_img_u8->mutable_cpu_data());

                char_img_u8->convert_order();

                auto char_img_tensor = char_img_u8 | memory::tensor_convert_to<float>;

                auto char_classfi_output = chinese_classfi_instance->forward(char_img_tensor);

                std::vector<float> char_detections(char_classfi_output["output"]->cpu_data(), char_classfi_output["output"]->cpu_data() + char_classfi_output["output"]->count());

                auto char_biggest_index = std::distance(char_detections.begin(), std::max_element(char_detections.begin(), char_detections.end()));

                s += char_label_index[char_biggest_index];

            }

            return s;
        }
        
        /**
        * @brief select roi size larger than threshold
        * @param keep   selected satisfy larger than overlap_threshold 0.5
        * @param boxes  bounding_boxes or bounding_boxes vector
        * @param overlap_threshold  float 0.5 or nums_threashold
        * @param mode   "union" or "min".
        */
        std::vector<size_t> nms(std::tuple<std::vector<Point4f>, std::vector<float>, std::vector<Point4f>>& boxes, float overlap_threshold = 0.5, std::string mode = "union")
        {
            std::vector<size_t> keep;
            if (std::get<1>(boxes).size() == 0)
            {
                keep.push_back(0);
                return keep;
            }

            int num = std::get<0>(boxes).size();

            std::vector<float> x1;
            std::vector<float> y1;
            std::vector<float> x2;
            std::vector<float> y2;

            std::vector<Point4f> locations;
            std::vector<float> scores;
            std::tie(locations, scores, std::ignore) = boxes;

            for (auto& key : locations)
            {
                x1.push_back(key.x);
                x2.push_back(key.ex);
                y1.push_back(key.y);
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
                    y1_ids.push_back(x2[ids[j]]);
                    x2_ids.push_back(y1[ids[j]]);
                    y2_ids.push_back(y1[ids[j]]);
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
                ids.erase(ids.end());
                // delete which larger than overlap  
                ids = delete_larger(ids, overlap, overlap_threshold);
            }
        }

        std::vector<Point4f> calibrate_box(std::vector<Point4f> bboxes, std::vector<Point4f>offsets)
        {
            std::vector<float> w;
            std::vector<float> h;
            for (auto box : bboxes)
            {
                w.push_back(box.ex - box.x);
                h.push_back(box.ey - box.y);
            }

            std::vector<Point4f> translation;
            for (int i = 0; i < offsets.size(); i++)
            {
                struct Point4f trans;
                trans.x = w[i] * offsets[i].x;
                trans.ex = h[i] * offsets[i].ex;
                trans.y = w[i] * offsets[i].y;
                trans.ey = h[i] * offsets[i].ey;
                translation.push_back(trans);
            }
            std::vector<Point4f> locations;
            for (int i = 0; i < bboxes.size(); i++)
            {
                struct Point4f location;
                location.x = bboxes[i].x + translation[i].x;
                location.ex = bboxes[i].ex + translation[i].ex;
                location.y = bboxes[i].y + translation[i].y;
                location.ey = bboxes[i].ey + translation[i].ey;
                locations.push_back(location);
            }
            return locations;
        }

        std::tuple<std::vector<Point4f>, std::vector<Point4f>, std::vector<int>, std::vector<int>> correct_bboxes(
            std::vector<Point4f>& bboxes,
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
                    correct_cut.ex = key.x;
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

        void cut_image_boxes(std::vector<cv::Mat>& img_boxes, cv::Mat& image, std::vector<Point4f>& bboxes)
        {
            auto size = std::make_pair(94, 24);
            int num_boxes = bboxes.size();

            int width = image.cols;
            int height = image.rows;

            std::tuple<std::vector<Point4f>, std::vector<Point4f>, std::vector<int>, std::vector<int>> cutouts;
            cutouts = correct_bboxes(bboxes, width, height);
            std::vector<Point4f> corrected;
            std::vector<Point4f> coordinates;
            std::vector<int> w;
            std::vector<int> h;
            std::tie(corrected, coordinates, w, h) = cutouts;

            for (int i = 0; i < num_boxes; i++)
            {
                cv::Mat img_box = cv::Mat::zeros(h[i], w[i], CV_32FC3);

                int coordinates_y = coordinates[i].y;
                int coordinates_ey = coordinates[i].ey + 1;
                int coordinates_x = coordinates[i].x;
                int coordinates_ex = coordinates[i].ex + 1;

                int corrected_y = corrected[i].y;
                int corrected_ey = corrected[i].ey + 1;
                int corrected_x = corrected[i].x;
                int corrected_ex = corrected[i].ex + 1;

                cv::Range coordinates_ry = cv::Range(coordinates_y, coordinates_ey);
                cv::Range coordinates_rx = cv::Range(coordinates_x, coordinates_ex);
                cv::Range corrected_ry = cv::Range(corrected_y, corrected_ey);
                cv::Range corrected_rx = cv::Range(corrected_x, corrected_ex);

                img_box(coordinates_ry, coordinates_rx) = image(corrected_ry, corrected_rx);

                cv::resize(img_box, img_box, cv::Size(size.first, size.second));
                // preprocess
                cv::cvtColor(img_box, img_box, cv::COLOR_BGR2RGB);
                // hwc => chw input_mat Ĭ�Ͼ���nchw    // TODO

                // ��һ��
                img_box.convertTo(img_box, CV_32FC3, 0, 255);
                img_boxes.push_back(img_box);
            }
        }

        void run_net(std::vector<box_info_internal>& results, std::vector<int>& roi, std::map<std::string, float>& param_map)
        {
            excalibur::rectangle<int> rect((int)roi[0], (int)roi[1], (int)roi[2], (int)roi[3]);
            std::shared_ptr<glasssix::memory::tensor<uint8_t>> input;

            // step 1 image preprocessing
            excalibur::safty_cut_cpu(cache_, input, &rect);
            cv::Mat input_mat = cv::Mat(1, input->height(), input->width(), 3);
            std::memcpy(input_mat.data, input->cpu_data(), input->count(2, 4));
            // cut image into roi image

            if (input->order() == glasssix::memory::NHWC)
                input->convert_order();

            // step 2 use pnet detect bboxes;
            int height = int(roi[2]);
            int width = int(roi[3]);
            std::pair<int, int> min_lp_size{ 50, 15 };

            // scale by scales
            std::vector<float> scales;
            create_scales(scales, height, width, min_lp_size);

            std::vector<std::tuple<std::vector<Point4f>, std::vector<float>, std::vector<Point4f>>> bounding_boxes;

            for (auto& key : scales)
            {
                int sw = std::ceil(width * key);
                int sh = std::ceil(height * key);

                cv::Mat scale_mat = cv::Mat();
                cv::resize(input_mat, scale_mat, cv::Size(sw, sh), 0, 0, cv::INTER_LINEAR_EXACT);
                // pnet test examples
                // cv::Mat img = cv::imread("E:pnet_test.jpg");
                // convert pnet_test.jpg nhwc into nchw
                std::shared_ptr<memory::tensor<uint8_t>> pnet_input_tensor_u8(new memory::tensor<uint8_t>(std::vector<int>{1, sh, sw, 3}, -1, memory::NHWC));
                // mat convert into tensor
                std::copy(scale_mat.data, scale_mat.data + scale_mat.step[0] * scale_mat.rows, pnet_input_tensor_u8->mutable_cpu_data());
                pnet_input_tensor_u8->convert_order();
                auto pnet_input_tensor = pnet_input_tensor_u8 | glasssix::memory::tensor_convert_to<float>;
                // pnet forward
                std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>, std::hash<std::string>> pnet_infer_output = pnet_instance_->forward(pnet_input_tensor);

                std::vector<Point4f> locations;
                std::vector<float> scores;
                std::vector<Point4f> offsets;
                std::map<std::string, float> pnet_params = {
                    {"thresh", param_map.count("thresh") ? param_map["thresh"] : 0.6},
                    {"nums_thresh", param_map.count("nums_thresh") ? param_map["nums_thresh"] : 0.5}
                };

                std::tuple<std::vector<Point4f>, std::vector<float>, std::vector<Point4f>> bboxes;

                det_pnet_infer(pnet_infer_output, pnet_params, key, bboxes);

                bounding_boxes.push_back(bboxes);
               
            }
            
            // collect boxes (and offsets, and scores) from different scales
            for(auto val = bounding_boxes.begin(); val != bounding_boxes.end();)
            {
                if (std::get<1>(*val).size() == 1)
                {
                    val = bounding_boxes.erase(val);
                }  
                else
                {
                    val++;
                }
            }

            float nums_thresh = 0.6;

            std::vector<std::tuple<std::vector<Point4f>, std::vector<float>, std::vector<Point4f>>> bounding_boxes_keeped;
            std::vector<size_t>  bounding_boxes_keep;
            std::vector<Point4f> keep_local;
            std::vector<float>   keep_scores;
            std::vector<Point4f> keep_offset;

            if(bounding_boxes.size() != 0)
            {
                for(auto key: bounding_boxes)
                {
                    auto bounding_boxes_keep = nms(key, nums_thresh);
                }

                if(bounding_boxes_keep.size() > 0)
                {
                    for(int i = 0; i < bounding_boxes.size(); i++)
                    {
                        for(int j = 0; j < bounding_boxes_keep.size(); j++)
                        {
                            if (i != bounding_boxes_keep[j])
                            {
                                bounding_boxes_keeped.push_back(bounding_boxes[i]);
                            }
                        }
                    }
                }
            }
            else
            {
                struct Point4f zero_bboxes = {0,0,0,0};

                std::vector<Point4f> zero_local = {zero_bboxes};
                std::vector<float> zero_scores  = {0};
                std::vector<Point4f> zero_offset= {zero_bboxes};

                bounding_boxes_keeped.push_back(std::make_tuple(zero_local, zero_scores, zero_offset));
            }
            
            // vstack ֻȡ location4 ���� ��scores ��Ӧ�����Ŷ�
            std::vector<Point4f> out_locations;
            std::vector<float> out_scores;
            std::vector<Point4f> out_offsets;
            for (auto key : bounding_boxes_keeped)
            {
                int num = std::get<0>(key).size();
                for (int i = 0; i < num; i++)
                {
                    out_locations.push_back(std::get<0>(key)[i]);
                    out_scores.push_back(std::get<1>(key)[i]);
                    out_offsets.push_back(std::get<2>(key)[i]);
                }
            }

            auto out_bboxes = calibrate_box(out_locations, out_offsets);
            // step 2 end; pent detect return bboxes

            // step 3 use onet include boxes
            // cut image into image_boxes;
            // onet test
            // cv::Mat img = cv::imread("E:\onet_test.jpg")
            std::vector<cv::Mat> image_boxes;
            cut_image_boxes(image_boxes, input_mat, out_bboxes);
            // copy cv::Mat into tenosr
            int boxes_num = image_boxes.size();
            // 
            std::shared_ptr<memory::tensor<uint8_t>> image_boxes_tensor_u8(new memory::tensor<uint8_t>(std::vector<int>{boxes_num, 24, 94, 3}, -1, memory::NHWC));;
            for (int i = 0; i < boxes_num; i++)
            {
                std::copy(image_boxes[i].data, image_boxes[i].data + image_boxes[i].step[0] * image_boxes[i].rows, image_boxes_tensor_u8->mutable_cpu_data());
            }

            image_boxes_tensor_u8->convert_order();

            auto image_boxes_tensor = image_boxes_tensor_u8 |  memory::tensor_convert_to<float>;

            // onet forward
            std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> onet_infer_output = onet_instance_->forward(image_boxes_tensor);

            
            std::vector<Point4f> onet_locations;
            std::vector<float>   onet_scores;

            std::map<std::string, float> onet_params = {
                    {"thresh", param_map.count("thresh") ? param_map["thresh"] : 0.6},
                    {"nums_thresh", param_map.count("nums_thresh") ? param_map["nums_thresh"] : 0.7}
            };

            det_onet_infer(onet_infer_output, out_bboxes, onet_params, onet_locations, onet_scores);
            // onet end

            // step 4 find Corners
            // sort ���ֵ
            std::vector<size_t> max_index(onet_scores.size());
            std::iota(max_index.begin(), max_index.end(), 0);
            std::sort(max_index.begin(), max_index.end(), [&onet_scores](size_t t1, size_t t2) {return onet_scores[t1] < onet_scores[t2]; });

            auto blob_bboxes = onet_locations[max_index[0]];
            cv::Mat blob = input_mat(cv::Range(blob_bboxes.x,blob_bboxes.ex),cv::Range(blob_bboxes.y,blob_bboxes.ey));
            //  Use only license plate area to detect corners
            std::vector<cv::Point> corners;        // x y width height
            corners = findCorners(blob);

            // Draw corners on the full - size image
            // locations = retractROI(corners, args.roi_x1y1, args.roi_wh, new_wh, bbox)
            std::vector<cv::Point> retract_locations;
            retract_locations = retractROI(corners, roi, blob_bboxes);


            auto lp_size = std::make_pair(44,14);
            auto align_base = std::make_pair(0, 0);
            auto align_size = std::make_pair(44, 14);

            
            // Align image
            // according to corners
            cv::Mat aligned_image;
            transformImage(aligned_image, retract_locations, lp_size, align_base, align_size, input_mat);

            // step 5 char seg and classfi
            // char segment and classfi
            // test example
            // cv::Mat aligned_image = cv::imread("E:\char_seg_classfi.jpg");
            // std::string plate = "��A 123456";

            auto plate = char_segment_classfi(aligned_image, chinese_classfi_instance_, char_classfi_instance_);

            // std::cout<< plate;

            // step 6 collect C++ result into json label

            box_info_internal box;
            auto location = exposing::make_param_vector<float>();
            for (auto key : retract_locations)
            {
                location.push_back(key.x);
                location.push_back(key.y);
            }
            box.location = location;

            auto strinfos = exposing::param_string(plate);

            box.strinfos = strinfos;
            
            // save Align image into uint8 vector
            auto temp_vec = exposing::make_param_vector<std::uint8_t>();
            int aligned_image_size = aligned_image.channels()* aligned_image.rows * aligned_image.cols;
            temp_vec.resize(static_cast<size_t>(aligned_image_size));
            temp_vec.copy_from({ aligned_image.data , static_cast<size_t>(aligned_image_size) }, 0);

            box.aligned_images = temp_vec;

            results.push_back(box);
            
        }


    private:
        std::string model_directory_;
        int device_;
        std::shared_ptr<glasssix::memory::tensor<std::uint8_t>> cache_;
        std::unique_ptr<glasssix::excalibur::pipeline<float>> pnet_instance_;
        std::unique_ptr<glasssix::excalibur::pipeline<float>> onet_instance_;
        std::unique_ptr<glasssix::excalibur::pipeline<float>> chinese_classfi_instance_;
        std::unique_ptr<glasssix::excalibur::pipeline<float>> char_classfi_instance_;
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
}