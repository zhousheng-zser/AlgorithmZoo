#include "ocr_net_internal.hpp"
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
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <cfloat>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

namespace glasssix::mjollner
{
    class ocr_net_internal::impl
    {
    public:
        impl(std::string_view det_racy_path, std::string_view rec_racy_path, std::string_view alphabet_path, int device) : impl{hardcode::get_model_params("det_ocr"), det_racy_path, hardcode::get_model_params("rec_ocr"), rec_racy_path, alphabet_path, device}
        {
        }

        impl(const std::vector<std::string> &det_phai, std::string_view det_racy_path, const std::vector<std::string> &rec_phai, std::string_view rec_racy_path, std::string_view alphabet_path, int device) : device_{device}, det_instance_{det_phai, std::string{det_racy_path}, device}, rec_instance_{rec_phai, std::string{rec_racy_path}, device}, alphabet_path_{std::string{alphabet_path}}
        {
        }

        exposing::param_vector<box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order, int x, int y, int roi_width, int roi_height)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            if (std::min(width, height) < 640)
            {
                throw exposing::abi_invalid_argument("min(width, height) must be greater than 640!");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            // roi params
            std::vector<int> roi{x, y, roi_width, roi_height};
            init_cache(bitmap, channels, height, width, order, roi);
            std::vector<box_info_internal> results;
            run_pipeline(results, roi);
            auto result = exposing::make_param_vector<box_info>();
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

        void expand_polygon(const std::vector<cv::Point2f> &pList, std::vector<cv::Point2f> &out, float SAFELINE)
        { // already ordered by anticlockwise
            // 1. vertex set -> pList
            // 2. edge set and normalize it
            std::vector<cv::Point2f> dpList, ndpList;
            int count = pList.size();
            for (int i = 0; i < count; i++)
            {
                int next = (i == (count - 1) ? 0 : (i + 1));
                dpList.push_back(pList.at(next) - pList.at(i));
                float unitLen = 1.0f / sqrt(dpList.at(i).dot(dpList.at(i)));
                ndpList.push_back(dpList.at(i) * unitLen);
            }

            // 3. compute Line
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

            // 填充任意多边形
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

        std::vector<cv::Point2f> unclip(std::vector<cv::Point2f> &box, float unclip_ratio = 1.5)
        {
            // Outline of the contract
            float distance = cv::contourArea(box) * unclip_ratio / cv::arcLength(box, true);
            std::vector<cv::Point2f> out;
            expand_polygon(box, out, distance * (-1));
            return out;
        }

        void boxes_from_bitmap(cv::Mat &out, cv::Mat &mask, int src_w, int src_h, std::vector<std::vector<cv::Point2f>> &boxes, std::vector<float> &scores)
        {
            size_t max_candidates = 1000;
            int min_size = 3;
            float box_thresh = 0.7;
            // std::string score_mode = "fast";
            int width = mask.cols;
            int height = mask.rows;
            // 轮廓检测
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(mask, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
            size_t num_contours = std::min(contours.size(), max_candidates);
            for (size_t i = 0; i < num_contours; ++i)
            {
                std::vector<cv::Point> contour = contours[i];
                // 获取每个轮廓的最小外接矩形
                std::vector<cv::Point2f> points;
                float sside;
                get_mini_boxes(contour, points, sside);
                if (sside < min_size)
                {
                    continue;
                }
                float score = box_score_fast(out, points);
                if (score < box_thresh)
                {
                    continue;
                }
                std::vector<cv::Point2f> box = unclip(points);
                // sside: 最小外接矩形的宽高的最小值
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
            float *out_data = (float *)out.data;
            cv::Mat mask(out_->height(), out_->width(), CV_8UC1);
            std::uint8_t *mask_data = mask.data;
            for (int i = 0; i < count; ++i)
            {
                mask_data[i] = (out_data[i] > thresh ? 1 : 0) * 255;
            }
            boxes_from_bitmap(out, mask, src_w, src_h, boxes, scores);
        }

        std::pair<std::vector<std::vector<cv::Point2f>>, std::vector<float>> detect_resnet18(std::shared_ptr<memory::tensor<std::uint8_t>> &input)
        {
            int channels = input->channels();
            int width = input->width();
            int height = input->height();
            //float mean[] = {0.485, 0.456, 0.406};
            //float std[] = {0.229, 0.224, 0.225};
            auto input_tensor = input | memory::tensor_convert_to<float>;
            //float *input_tensor_data = input_tensor->mutable_cpu_data();
            //// div std
            //for (int c = 0; c < channels; ++c)
            //{
            //    for (int i = 0; i < width * height; ++i)
            //    {
            //        input_tensor_data[c * width * height + i] = (input_tensor_data[c * width * height + i] / 255.f - mean[c]) / std[c];
            //    }
            //}
            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> out = det_instance_.forward(input_tensor);
            // output
            std::shared_ptr<memory::tensor<float>> output = out["output"];
            std::vector<std::vector<cv::Point2f>> boxes;
            std::vector<float> scores;
            det_post_process(output, boxes, scores);
            return std::make_pair(boxes, scores);
        }

        void softmax(cv::Mat &input)
        {
            std::vector<float> y(input.total());
            float sum = 0.0f;
            float *input_data = (float *)input.data;
            for (size_t i = 0; i < input.total(); ++i)
            {
                sum += y[i] = std::exp(input_data[i]);
            }
            for (size_t i = 0; i < input.total(); ++i)
            {
                input_data[i] = y[i] / sum;
            }
        }

        void init_character(std::vector<std::string> &character)
        {
            character.reserve(6625);
            std::ifstream in(alphabet_path_);
            std::string line;
            if (in)
            {
                while (std::getline(in, line))
                {
                    character.push_back(line);
                }
                character.insert(character.begin(), "[blank]");
                character.insert(character.end(), " ");
                return;
            }
            LOG(ERROR) << "no such file";
        }

        std::pair<std::string, std::vector<float>> rec_post_process(std::shared_ptr<memory::tensor<float>> &output)
        {
            int width = output->width();
            int height = output->height();
            cv::Mat out(height, width, CV_32FC1);
            memcpy(out.data, output->cpu_data(), height * width * sizeof(float));
            // softmax
            softmax(out);
            float *out_data = (float *)out.data;
            std::vector<int> word;
            std::vector<float> prob;
            int index = 0;
            float max_val = 0;
            for (int i = 0; i < height; ++i)
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
                word.push_back(index);
                prob.push_back(max_val);
                index = 0;
                max_val = 0;
            }
            std::string result;
            std::vector<float> conf;
            std::vector<std::string> character;
            init_character(character);
            for (int i = 0; i < word.size(); ++i)
            {
                if (word[i] > 0 && !(i > 0 && word[i - 1] == word[i]))
                {
                    result.append(character[word[i]]);
                    conf.push_back(prob[i]);
                }
            }
            return std::make_pair(result, conf);
        }

        std::pair<std::string, std::vector<float>> detect_resnet34(cv::Mat &img)
        {
            // 将图片宽度resize到指定高度
            float input_h = 32.f;
            float resize_ratio = input_h / img.rows;
            cv::resize(img, img, cv::Size(0, 0), resize_ratio, resize_ratio, cv::INTER_LINEAR);
            int channels = img.channels();
            int width = img.cols;
            int height = img.rows;
            std::shared_ptr<memory::tensor<uint8_t>> input(new memory::tensor<uint8_t>(channels, height, width, -1, memory::NHWC, nullptr));
            std::copy(img.data, img.data + img.step[0] * img.rows, input->mutable_cpu_data());
            input->convert_order();
            auto input_tensor = input | memory::tensor_convert_to<float>;
            //float *input_tensor_data = input_tensor->mutable_cpu_data();
            //for (int i = 0; i < input_tensor->count(); ++i)
            //{
            //    input_tensor_data[i] /= 255.f;
            //}
            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> out = rec_instance_.forward(input_tensor);
            // output
            std::shared_ptr<memory::tensor<float>> output = out["output"];
            return rec_post_process(output);
        }

        cv::Mat crop_rect(cv::Mat &img, cv::RotatedRect &rect, float alph = 0.15)
        {
            float degree = rect.angle;
            cv::Size size = rect.size;
            cv::Point2f center = rect.center;

            cv::Mat img_rot;
            cv::Mat img_crop;
            int width = img.cols;
            int height = img.rows;
            float min_size = std::min(size.width, size.height);
            if (degree > -45)
            {
                size.width = static_cast<int>(size.width + min_size * alph);
                size.height = static_cast<int>(size.height + min_size * alph);
                cv::Mat M = cv::getRotationMatrix2D(center, degree, 1);
                cv::warpAffine(img, img_rot, M, cv::Size(width, height));
                cv::getRectSubPix(img_rot, size, center, img_crop);
            }
            else
            {
                size.width = static_cast<int>(size.width + min_size * alph);
                size.height = static_cast<int>(size.height + min_size * alph);
                degree -= 270;
                cv::Mat M = cv::getRotationMatrix2D(center, degree, 1);
                cv::warpAffine(img, img_rot, M, cv::Size(width, height));
                cv::getRectSubPix(img_rot, cv::Size(size.height, size.width), center, img_crop);
            }
            return img_crop;
        }

        // AntiClockWise 90
        cv::Mat rotateAntiClockWise90(cv::Mat src)
        {
            cv::transpose(src, src);
            cv::flip(src, src, 0);
            return src;
        }

        void run_pipeline(std::vector<box_info_internal> &results, std::vector<int> &roi)
        {
            excalibur::rectangle<int> rect((int)roi[0], (int)roi[1], (int)roi[3], (int)roi[2]);
            std::shared_ptr<memory::tensor<std::uint8_t>> tmp;
            std::shared_ptr<memory::tensor<uint8_t>> input;
            excalibur::safty_cut_cpu(cache_, tmp, &rect);
            int height = tmp->height();
            int width = tmp->width();
            int hpad = (height + 31) / 32 * 32 - height;
            int wpad = (width + 31) / 32 * 32 - width;
            excalibur::make_border(tmp, input, hpad / 2, hpad - hpad / 2, wpad / 2, wpad - wpad / 2, excalibur::border_constant);
            // convert NCHW TO NHWC
            const uint8_t *in_data = input->cpu_data();
            int step = input->width() * input->height();
            int channels = input->channels();
            cv::Mat img(input->height(), input->width(), CV_8UC3);
            for (int i = 0; i < step; ++i)
            {
                for (int c = 0; c < channels; ++c)
                {
                    *(img.data + channels * i + c) = *(in_data + input->offset(0, c) + i);
                }
            }

            std::pair<std::vector<std::vector<cv::Point2f>>, std::vector<float>> result = detect_resnet18(input);
            std::vector<std::vector<cv::Point2f>> box_list = result.first;
            std::vector<float> score_list = result.second;
            for (int i = 0; i < box_list.size(); ++i)
            {
                cv::RotatedRect rect = cv::minAreaRect(box_list[i]);
                // crop img
                cv::Mat cut_img = crop_rect(img, rect);
                int newH = cut_img.rows;
                int newW = cut_img.cols;
                if (newH > 1.5 * newW)
                {
                    cut_img = rotateAntiClockWise90(cut_img);
                }
                // run identify network
                std::pair<std::string, std::vector<float>> out = detect_resnet34(cut_img);
                box_info_internal box;
                auto location = exposing::make_param_vector<float>();
                for (int j = 0; j < box_list[i].size(); ++j)
                {
                    location.push_back(box_list[i][j].x);
                    location.push_back(box_list[i][j].y);
                }
                box.location = location;
                box.strinfo = exposing::param_string(out.first);
                box.angle = rect.angle;
                results.push_back(box);
            }
        }

    private:
        int device_;
        excalibur::pipeline<float> det_instance_;
        excalibur::pipeline<float> rec_instance_;
        std::string alphabet_path_;
        std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
    };

    ocr_net_internal::ocr_net_internal(std::string_view det_racy_path, std::string_view rec_racy_path, std::string_view alphabet_path, int device) : impl_{std::make_unique<impl>(det_racy_path, rec_racy_path, alphabet_path, device)}
    {
    }

    ocr_net_internal::ocr_net_internal(const std::vector<std::string> &det_phai, std::string_view det_racy_path, const std::vector<std::string> &rec_phai, std::string_view rec_racy_path, std::string_view alphabet_path, int device) : impl_{std::make_unique<impl>(det_phai, det_racy_path, rec_phai, rec_racy_path, alphabet_path, device)}
    {
    }

    ocr_net_internal::~ocr_net_internal()
    {
    }

    exposing::param_vector<box_info> ocr_net_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order, int x, int y, int roi_width, int roi_height) const
    {
        return impl_->detect(bitmap, channels, height, width, order, x, y, roi_width, roi_height);
    }

    std::string ocr_net_internal::version()
    {
        return impl::version();
    }
}
