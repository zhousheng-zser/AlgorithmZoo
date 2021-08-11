#include "yolov5s_net_internal.hpp"
#include "hardcode.hpp"
#include "result_info_impl.hpp"
#include "vp_info_impl.hpp"

#include <fstream>
#include <algorithm>
#include <Excalibur/pipeline.hpp>
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include "Primitives/tensor_conversions.hpp"

#include <cfloat>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

namespace glasssix::valklyrs
{
    anchor_box &operator&=(anchor_box &a, const anchor_box &b)
    {
        float x1 = std::max(a.x, b.x);
        float y1 = std::max(a.y, b.y);
        a.width = std::min(a.x + a.width, b.x + b.width) - x1;
        a.height = std::min(a.y + a.height, b.y + b.height) - y1;
        a.x = x1;
        a.y = y1;
        if (a.width <= 0 || a.height <= 0)
            a = anchor_box();
        return a;
    }

    anchor_box operator&(const anchor_box &a, const anchor_box &b)
    {
        anchor_box c = a;
        return c &= b;
    }

    class yolov5s_net_internal::impl
    {
    public:
        impl(std::string_view yolov5s_racy_path, std::string_view vehicle_racy_path, std::string_view person_racy_path, int device) : impl{hardcode::get_model_params("yolov5s"), yolov5s_racy_path, hardcode::get_model_params("vehicle_attri"), vehicle_racy_path, hardcode::get_model_params("person_attri"), person_racy_path, device}
        {
        }

        impl(const std::vector<std::string> &yolov5s_phai, std::string_view yolov5s_racy_path, const std::vector<std::string> &vehicle_phai, std::string_view vehicle_racy_path, const std::vector<std::string> &person_phai, std::string_view person_racy_path, int device) : device_{device}, yolov5s_instance_{yolov5s_phai, std::string{yolov5s_racy_path}, device}, vehicle_instance_{vehicle_phai, std::string{vehicle_racy_path}, device}, person_instance_{person_phai, std::string{person_racy_path}, device}
        {
        }

        exposing::param_vector<result_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);
            init_cache(bitmap, channels, height, width, order);

            result_info_internal results;
            std::vector<vp_info_internal> vehicle_list;
            std::vector<vp_info_internal> person_list;
            run_pipeline(channels, height, width, order, vehicle_list, person_list);
            auto result = exposing::make_param_vector<result_info>();
            auto vehicle_list_ = exposing::make_param_vector<vp_info>();
            auto person_list_ = exposing::make_param_vector<vp_info>();

            for (auto &i : vehicle_list)
            {
                vehicle_list_.push_back(exposing::make_as_first<vp_info_impl>(i));
            }
            for (auto &i : person_list)
            {
                person_list_.push_back(exposing::make_as_first<vp_info_impl>(i));
            }
            result_info_internal internal;
            internal.vehicle_list = vehicle_list_;
            internal.person_list = person_list_;
            result.push_back(exposing::make_as_first<result_info_impl>(internal));
            return result;
        }

        static std::string version()
        {
            return "1.0.0";
        }

    private:
        void init_cache(exposing::param_span<std::uint8_t> &bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order)
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

                cache_ = std::make_shared<memory::tensor<std::uint8_t>>(shape, -1, (memory::orderType)order/*, &memory::pool_allocator_default<std::uint8_t>::get()*/);
            }

            if (cache_->device() > 0)
            {
#ifdef USE_CUDA
                cudaMemcpy(cache->mutable_gpu_data(), bitmap, channels * height * width, cudaMemcpyHostToDevice);
#else
                NO_GPU;
#endif
            }
            else
                std::copy(bitmap.begin(), bitmap.end(), cache_->mutable_cpu_data());

            if (order == memory::NHWC)
                cache_->convert_order();
        }

        inline float intersection_area(const obj_info_internal &a, const obj_info_internal &b)
        {
            anchor_box inter = a.rect & b.rect;
            return inter.width * inter.height;
        }

        void qsort_descent_inplace(std::vector<obj_info_internal> &faceobjects, int left, int right)
        {
            int i = left;
            int j = right;
            float p = faceobjects[(left + right) / 2].prob;

            while (i <= j)
            {
                while (faceobjects[i].prob > p)
                    i++;

                while (faceobjects[j].prob < p)
                    j--;

                if (i <= j)
                {
                    // swap
                    std::swap(faceobjects[i], faceobjects[j]);

                    i++;
                    j--;
                }
            }

#pragma omp parallel sections
            {
#pragma omp section
                {
                    if (left < j)
                        qsort_descent_inplace(faceobjects, left, j);
                }
#pragma omp section
                {
                    if (i < right)
                        qsort_descent_inplace(faceobjects, i, right);
                }
            }
        }

        void qsort_descent_inplace(std::vector<obj_info_internal> &faceobjects)
        {
            if (faceobjects.empty())
                return;

            qsort_descent_inplace(faceobjects, 0, faceobjects.size() - 1);
        }

        void nms_sorted_bboxes(const std::vector<obj_info_internal> &faceobjects, std::vector<int> &picked, float nms_threshold)
        {
            picked.clear();
            const int n = faceobjects.size();

            std::vector<float> areas(n);
            for (int i = 0; i < n; i++)
            {
                const anchor_box &rect = faceobjects[i].rect;
                areas[i] = rect.height * rect.width;
            }

            for (int i = 0; i < n; i++)
            {
                const obj_info_internal &a = faceobjects[i];

                int keep = 1;
                for (int j = 0; j < (int)picked.size(); j++)
                {
                    const obj_info_internal &b = faceobjects[picked[j]];

                    // intersection over union
                    float inter_area = intersection_area(a, b);
                    float union_area = areas[i] + areas[picked[j]] - inter_area;
                    // float IoU = inter_area / union_area
                    if (inter_area / union_area > nms_threshold)
                        keep = 0;
                }

                if (keep)
                    picked.push_back(i);
            }
        }

        inline float sigmoid(float x)
        {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

        void generate_proposals(const memory::tensor<float> &anchors, int stride, const std::shared_ptr<memory::tensor<std::uint8_t>> &in_pad, const std::shared_ptr<memory::tensor<float>> &feat_blob, float prob_threshold, std::vector<obj_info_internal> &objects)
        {
            const int num_grid = feat_blob->height();

            int num_grid_x;
            int num_grid_y;
            if (in_pad->width() > in_pad->height())
            {
                num_grid_x = in_pad->width() / stride;
                num_grid_y = num_grid / num_grid_x;
            }
            else
            {
                num_grid_y = in_pad->height() / stride;
                num_grid_x = num_grid / num_grid_y;
            }

            const int num_class = feat_blob->width() - 5;

            const int num_anchors = anchors.width() / 2;

            for (int q = 0; q < num_anchors; q++)
            {
                const float anchor_w = anchors[q * 2];
                const float anchor_h = anchors[q * 2 + 1];

                const memory::tensor<float> feat = feat_blob->channel(q);

                for (int i = 0; i < num_grid_y; i++)
                {
                    for (int j = 0; j < num_grid_x; j++)
                    {
                        const float *featptr = feat.row(i * num_grid_x + j);

                        // find class index with max class score
                        int class_index = 0;
                        float class_score = -FLT_MAX;
                        for (int k = 0; k < num_class; k++)
                        {
                            float score = featptr[5 + k];
                            if (score > class_score)
                            {
                                class_index = k;
                                class_score = score;
                            }
                        }

                        float box_score = featptr[4];

                        // float confidence = sigmoid(box_score) * sigmoid(class_score);
                        float confidence = sigmoid(box_score);

                        if (confidence >= prob_threshold)
                        {
                            // yolov5/models/yolo.py Detect forward
                            // y = x[i].sigmoid()
                            // y[..., 0:2] = (y[..., 0:2] * 2. - 0.5 + self.grid[i].to(x[i].device)) * self.stride[i]  # xy
                            // y[..., 2:4] = (y[..., 2:4] * 2) ** 2 * self.anchor_grid[i]  # wh

                            float dx = sigmoid(featptr[0]);
                            float dy = sigmoid(featptr[1]);
                            float dw = sigmoid(featptr[2]);
                            float dh = sigmoid(featptr[3]);

                            float pb_cx = (dx * 2.f - 0.5f + j) * stride;
                            float pb_cy = (dy * 2.f - 0.5f + i) * stride;

                            float pb_w = pow(dw * 2.f, 2) * anchor_w;
                            float pb_h = pow(dh * 2.f, 2) * anchor_h;

                            float x0 = pb_cx - pb_w * 0.5f;
                            float y0 = pb_cy - pb_h * 0.5f;
                            float x1 = pb_cx + pb_w * 0.5f;
                            float y1 = pb_cy + pb_h * 0.5f;

                            obj_info_internal obj;
                            obj.rect.x = x0;
                            obj.rect.y = y0;
                            obj.rect.width = x1 - x0;
                            obj.rect.height = y1 - y0;
                            obj.label = class_index;
                            obj.prob = confidence;

                            objects.push_back(obj);
                        }
                    }
                }
            }
        }

        int detect_yolo(int channels, int height, int width, int order, std::vector<obj_info_internal> &objects)
        {
            /** Before processing **/
            const int target_size = 640;
            const float prob_threshold = 0.5f;
            const float nms_threshold = 0.4f;

            // letterbox pad to multiple of 32
            int w = width;
            int h = height;
            float scale = 1.f;
            if (w > h)
            {
                scale = (float)target_size / w;
                w = target_size;
                h = h * scale;
            }
            else
            {
                scale = (float)target_size / h;
                h = target_size;
                w = w * scale;
            }
            // resize
            std::shared_ptr<memory::tensor<std::uint8_t>> cache_forward;
            excalibur::resize_cpu(cache_, cache_forward, h, w);

            // pad
            int top = (target_size - h) / 2;
            int down = (target_size - h + 1) / 2;
            int left = (target_size - w) / 2;
            int right = (target_size - w + 1) / 2;
            excalibur::make_border(cache_forward, cache_forward, top, down, left, right, excalibur::border_constant, static_cast<std::uint8_t>(114));
            auto input_tensor = cache_forward | memory::tensor_convert_to<float>;
            float *input_tensor_data = input_tensor->mutable_cpu_data();
            for (int i = 0; i < input_tensor->count(); ++i)
            {
                input_tensor_data[i] /= 255.f;
            }

            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> out = yolov5s_instance_.forward(input_tensor);

            std::vector<obj_info_internal> proposals;
            // stride 8
            {
                memory::tensor<float> anchors(6);
                anchors[0] = 10.f;
                anchors[1] = 13.f;
                anchors[2] = 16.f;
                anchors[3] = 30.f;
                anchors[4] = 33.f;
                anchors[5] = 23.f;

                std::vector<obj_info_internal> objects8;
                generate_proposals(anchors, 8, cache_forward, out["751"], prob_threshold, objects8);

                proposals.insert(proposals.end(), objects8.begin(), objects8.end());
            }

            // stride 16
            {
                memory::tensor<float> anchors(6);
                anchors[0] = 30.f;
                anchors[1] = 61.f;
                anchors[2] = 62.f;
                anchors[3] = 45.f;
                anchors[4] = 59.f;
                anchors[5] = 119.f;

                std::vector<obj_info_internal> objects16;
                generate_proposals(anchors, 16, cache_forward, out["1060"], prob_threshold, objects16);

                proposals.insert(proposals.end(), objects16.begin(), objects16.end());
            }

            // stride 32
            {
                memory::tensor<float> anchors(6);
                anchors[0] = 116.f;
                anchors[1] = 90.f;
                anchors[2] = 156.f;
                anchors[3] = 198.f;
                anchors[4] = 373.f;
                anchors[5] = 326.f;

                std::vector<obj_info_internal> objects32;
                generate_proposals(anchors, 32, cache_forward, out["1369"], prob_threshold, objects32);

                proposals.insert(proposals.end(), objects32.begin(), objects32.end());
            }

            // sort all proposals by score from highest to lowest
            qsort_descent_inplace(proposals);

            // apply nms with nms_threshold
            std::vector<int> picked;
            nms_sorted_bboxes(proposals, picked, nms_threshold);

            int count = picked.size();

            objects.resize(count);
            for (int i = 0; i < count; i++)
            {
                objects[i] = proposals[picked[i]];

                // adjust offset to original unpadded
                float x0 = (objects[i].rect.x - left) / scale;
                float y0 = (objects[i].rect.y - top) / scale;
                float x1 = (objects[i].rect.x + objects[i].rect.width - left) / scale;
                float y1 = (objects[i].rect.y + objects[i].rect.height - top) / scale;

                // clip
                x0 = std::max(std::min(x0, (float)(width - 1)), 0.f);
                y0 = std::max(std::min(y0, (float)(height - 1)), 0.f);
                x1 = std::max(std::min(x1, (float)(width - 1)), 0.f);
                y1 = std::max(std::min(y1, (float)(height - 1)), 0.f);

                objects[i].rect.x = x0;
                objects[i].rect.y = y0;
                objects[i].rect.width = x1 - x0;
                objects[i].rect.height = y1 - y0;
            }

            return 0;
        }

        int get_max_val_index(std::vector<float> &output, int start, int end)
        {
            if (start + 1 == end)
            {
                return output[start] > 0 ? 0 : 1;
            }
            std::vector<float>::iterator begin_it = output.begin() + start;
            std::vector<float>::iterator end_it = output.begin() + end;
            return static_cast<int>(std::max_element(begin_it, end_it) - begin_it);
        }

        void vehicle_post_process(std::shared_ptr<memory::tensor<float>> &output, anchor_box &rect, std::vector<vp_info_internal> &vehicle_list)
        {
            std::vector<std::string> color{"Black", "Blue", "Brown", "Gray", "Green", "Pink", "Red", "White", "Yellow"};
            std::vector<std::string> ori{"Front", "Rear"};
            std::vector<std::string> type{"passengerCar", "saloonCar", "shopTruck", "suv", "trailer", "truck", "van", "waggon"};
            std::vector<float> result_vehi;
            // result_vehi.reserve(output->count());
            std::copy(output->cpu_data(), output->cpu_data() + output->count(), std::back_inserter(result_vehi));

            int index_color = get_max_val_index(result_vehi, 0, 9);
            int index_ori = get_max_val_index(result_vehi, 9, 11);
            int index_type = get_max_val_index(result_vehi, 11, 19);

            vp_info_internal vi;
            vi.coordinates = exposing::make_param_vector<float>();
            vi.attributes = exposing::make_param_vector<exposing::param_string>();
            vi.coordinates.push_back(rect.x);
            vi.coordinates.push_back(rect.y);
            vi.coordinates.push_back(rect.width);
            vi.coordinates.push_back(rect.height);

            vi.attributes.push_back(exposing::to_param_string(color[index_color]));
            vi.attributes.push_back(exposing::to_param_string(ori[index_ori]));
            vi.attributes.push_back(exposing::to_param_string(type[index_type]));
            vehicle_list.push_back(vi);
        }

        void person_post_process(std::shared_ptr<memory::tensor<float>> &output, anchor_box &rect, std::vector<vp_info_internal> &person_list)
        {
            std::vector<float> result_person;
            std::copy(output->cpu_data(), output->cpu_data() + output->count(), std::back_inserter(result_person));

            int index_gender = get_max_val_index(result_person, 0, 1);
            int index_age = get_max_val_index(result_person, 1, 4);
            int index_vehi_ori = get_max_val_index(result_person, 4, 7);
            int index_hat = get_max_val_index(result_person, 7, 8);
            int index_glass = get_max_val_index(result_person, 8, 9);
            int index_handbag = get_max_val_index(result_person, 9, 10);
            int index_shoulderbag = get_max_val_index(result_person, 10, 11);
            int index_backpack = get_max_val_index(result_person, 11, 12);
            int index_sleeve = get_max_val_index(result_person, 13, 15);
            int index_texture = get_max_val_index(result_person, 17, 19);
            int index_lower_type = get_max_val_index(result_person, 22, 25);

            vp_info_internal vi;
            vi.coordinates = exposing::make_param_vector<float>();
            vi.attributes = exposing::make_param_vector<exposing::param_string>();
            vi.coordinates.push_back(rect.x);
            vi.coordinates.push_back(rect.y);
            vi.coordinates.push_back(rect.width);
            vi.coordinates.push_back(rect.height);

            vi.attributes.push_back(person_attribute::gender[index_gender]);
            vi.attributes.push_back(person_attribute::age[index_age]);
            vi.attributes.push_back(person_attribute::ori[index_vehi_ori]);
            vi.attributes.push_back(person_attribute::hat[index_hat]);
            vi.attributes.push_back(person_attribute::glass[index_glass]);
            vi.attributes.push_back(person_attribute::handbag[index_handbag]);
            vi.attributes.push_back(person_attribute::shoulderbag[index_shoulderbag]);
            vi.attributes.push_back(person_attribute::backpack[index_backpack]);
            vi.attributes.push_back(person_attribute::sleeve[index_sleeve]);
            vi.attributes.push_back(person_attribute::texture[index_texture]);
            vi.attributes.push_back(person_attribute::lower_type[index_lower_type]);
            person_list.push_back(vi);
        }

        void detect(std::vector<float> &box, std::shared_ptr<memory::tensor<float>> &output, std::vector<float> mean_ = {}, std::vector<float> std_ = {})
        {
            // cut img
            std::shared_ptr<memory::tensor<std::uint8_t>> input;
            excalibur::rectangle<int> rect((int)box[0], (int)box[1], (int)box[3], (int)box[2]);
            excalibur::safty_cut_cpu(cache_, input, &rect);
            // pre process
            int channels = input->channels();
            // resize img to 3 * 224 * 224
            int w = 224;
            int h = 224;
            excalibur::resize_cpu(input, input, h, w);
            std::shared_ptr<memory::tensor<float>> input_tensor = input | memory::tensor_convert_to<float>;
            float *input_data = input_tensor->mutable_cpu_data();
            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> out;
            if (mean_.size() && std_.size())
            {
                for (int c = 0; c < channels; ++c)
                {
                    float *input_data_ = input_data + c * w * h;
                    for (int i = 0, size = w * h; i < size; ++i)
                    {
                        input_data_[i] = (input_data_[i] / 255.f - mean_[c]) / std_[c];
                    }
                }
                // detect
                out = vehicle_instance_.forward(input_tensor);
            }
            else
            {
                for (int i = 0; i < input->count(); ++i)
                {
                    input_data[i] /= 255.f;
                }
                // detect
                out = person_instance_.forward(input_tensor);
            }
            output = out["output"];
        }

        void vehicle_detect(anchor_box &rect, std::vector<vp_info_internal> &vehicle_list)
        {
            std::vector<float> box{rect.x, rect.y, rect.width, rect.height};
            std::vector<float> mean_{0.485, 0.456, 0.406};
            std::vector<float> std_{0.229, 0.224, 0.225};
            std::shared_ptr<memory::tensor<float>> output;
            detect(box, output, mean_, std_);
            vehicle_post_process(output, rect, vehicle_list);
        }

        void person_detect(anchor_box &rect, std::vector<vp_info_internal> &person_list)
        {
            std::vector<float> box{rect.x, rect.y, rect.width, rect.height};
            std::shared_ptr<memory::tensor<float>> output;
            detect(box, output);
            person_post_process(output, rect, person_list);
        }

        void run_pipeline(int channels, int height, int width, int order, std::vector<vp_info_internal> &vehicle_list, std::vector<vp_info_internal> &person_list)
        {
            std::vector<obj_info_internal> objects;
            // object detection
            detect_yolo(channels, height, width, order, objects);
            for (size_t i = 0, size = objects.size(); i < size; ++i)
            {
                if (objects[i].label) // vehicle
                {
                    vehicle_detect(objects[i].rect, vehicle_list);
                }
                else // person
                {
                    person_detect(objects[i].rect, person_list);
                }
            }
        }

    private:
        int device_;
        excalibur::pipeline<float> yolov5s_instance_;
        excalibur::pipeline<float> vehicle_instance_;
        excalibur::pipeline<float> person_instance_;
        std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
    };

    yolov5s_net_internal::yolov5s_net_internal(std::string_view yolov5s_racy_path, std::string_view vehicle_racy_path, std::string_view person_racy_path, int device) : impl_{std::make_unique<impl>(yolov5s_racy_path, vehicle_racy_path, person_racy_path, device)}
    {
    }

    yolov5s_net_internal::yolov5s_net_internal(const std::vector<std::string> &yolov5s_phai, std::string_view yolov5s_racy_path, const std::vector<std::string> &vehicle_phai, std::string_view vehicle_racy_path, const std::vector<std::string> &person_phai, std::string_view person_racy_path, int device) : impl_{std::make_unique<impl>(yolov5s_phai, yolov5s_racy_path, vehicle_phai, vehicle_racy_path, person_phai, person_racy_path, device)}
    {
    }

    yolov5s_net_internal::~yolov5s_net_internal()
    {
    }

    exposing::param_vector<result_info> yolov5s_net_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
    {
        return impl_->detect(bitmap, channels, height, width, order);
    }

    std::string yolov5s_net_internal::version()
    {
        return impl::version();
    }
}