#include "yolov5Deepsort_net_internal.hpp"
#include "hardcode.hpp"

#include "result_info_impl.hpp"

#include <fstream>
#include <algorithm>

#include <Excalibur/pipeline.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_resize.hpp>

#include "Primitives/tensor_conversions.hpp"
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>

#include <cfloat>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

#include "deep_sort.hpp"

namespace glasssix::pan
{
    anchor_box& operator&=(anchor_box& a, const anchor_box& b)
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

    anchor_box operator&(const anchor_box& a, const anchor_box& b)
    {
        anchor_box c = a;
        return c &= b;
    }


    class yolov5Deepsort_net_internal::impl
    {
    public:
        impl(std::string_view yolov5m_racy_path, std::string_view deepsort_racy_path, int device)
            : impl{ hardcode::get_model_params("yolov5m"), yolov5m_racy_path, hardcode::get_model_params("deepsort"), deepsort_racy_path, device }
        {
            //use other construction
        }

        impl(std::string_view yolov5m_phai, std::string_view yolov5m_racy_path, std::string_view deepsort_phai, std::string_view deepsort_racy_path, int device)
            : device_{ device }, 
            yolov5m_instance_{ std::string{yolov5m_phai}, std::string{yolov5m_racy_path}, device },
            deepsort_instance_{ std::string{deepsort_phai}, std::string{deepsort_racy_path}, device }
        {
        }

        impl(const std::vector<std::string>& yolov5m_phai, std::string_view yolov5m_racy_path, const std::vector<std::string>& deepsort_phai, std::string_view deepsort_racy_path, int device)
            : device_{ device }, 
            yolov5m_instance_{ yolov5m_phai, std::string{yolov5m_racy_path}, device },
            deepsort_instance_{ deepsort_phai, std::string{deepsort_racy_path}, device }
        {
        }
        
        inline float sigmoid(float x)
        {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

        void generate_proposals(const std::vector<float>& anchors, 
            int stride, 
            const std::shared_ptr<memory::tensor<std::uint8_t>>& in_pad, 
            const std::shared_ptr<memory::tensor<float>>& feat_blob, 
            float prob_threshold, 
            std::vector<obj_info_internal>& objects)
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

            const int num_anchors = anchors.size() / 2;

            for (int q = 0; q < num_anchors; q++)
            {
                const float anchor_w = anchors[q * 2];
                const float anchor_h = anchors[q * 2 + 1];

                const memory::tensor<float> feat = feat_blob->channel(q);

                for (int i = 0; i < num_grid_y; i++)
                {
                    for (int j = 0; j < num_grid_x; j++)
                    {
                        const float* featptr = feat.row(i * num_grid_x + j);

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
                            // pan/models/yolo.py Detect forward
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

        void qsort_descent_inplace(std::vector<obj_info_internal>& faceobjects, int left, int right)
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

        void qsort_descent_inplace(std::vector<obj_info_internal>& faceobjects)
        {
            if (faceobjects.empty())
                return;

            qsort_descent_inplace(faceobjects, 0, faceobjects.size() - 1);
        }



        inline float intersection_area(const obj_info_internal& a, const obj_info_internal& b)
        {
            anchor_box inter = a.rect & b.rect;
            return inter.width * inter.height;
        }

        void nms_sorted_bboxes(const std::vector<obj_info_internal>& faceobjects, std::vector<int>& picked, float nms_threshold)
        {
            picked.clear();
            const int n = faceobjects.size();

            std::vector<float> areas(n);
            for (int i = 0; i < n; i++)
            {
                const anchor_box& rect = faceobjects[i].rect;
                areas[i] = rect.height * rect.width;
            }

            for (int i = 0; i < n; i++)
            {
                const obj_info_internal& a = faceobjects[i];

                int keep = 1;
                for (int j = 0; j < (int)picked.size(); j++)
                {
                    const obj_info_internal& b = faceobjects[picked[j]];

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

        void detect_yolo(int channels, int height, int width, int order, std::vector<obj_info_internal>& objects) {

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


            //// pad
            //int top = (target_size - h) / 2;
            //int down = (target_size - h + 1) / 2;
            //int left = (target_size - w) / 2;
            //int right = (target_size - w + 1) / 2;

            // pad
            int stride = 32;//¡Ÿ ±
            int res_h = (target_size - h) % stride;
            int res_w = (target_size - w) % stride;
            int top = res_h / 2, down = res_h / 2;
            int left = res_w / 2, right = res_w / 2;

            excalibur::make_border(cache_forward, cache_forward, top, down, left, right, excalibur::border_constant, static_cast<std::uint8_t>(114));
            auto input_tensor = cache_forward | memory::tensor_convert_to<float>;

            //float *input_tensor_data = input_tensor->mutable_cpu_data();
            //for (int i = 0; i < input_tensor->count(); ++i)
            //{
            //    input_tensor_data[i] /= 255.f;
            //}

            std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> out = yolov5m_instance_.forward(input_tensor);

            //auto ret1_count = out["output1"].get()->count();
            //for (int i = 0; i < ret1_count; ++i)
            //{
            //    out["output1"].get().
            //}

            std::vector<obj_info_internal> proposals;
            // stride 8
            {
                std::vector<float> anchors(6);
                anchors[0] = 10.f;
                anchors[1] = 13.f;
                anchors[2] = 16.f;
                anchors[3] = 30.f;
                anchors[4] = 33.f;
                anchors[5] = 23.f;

                std::vector<obj_info_internal> objects8;
                generate_proposals(anchors, 8, cache_forward, out["output0"], prob_threshold, objects8);

                proposals.insert(proposals.end(), objects8.begin(), objects8.end());
            }

            // stride 16
            {
                std::vector<float> anchors(6);
                anchors[0] = 30.f;
                anchors[1] = 61.f;
                anchors[2] = 62.f;
                anchors[3] = 45.f;
                anchors[4] = 59.f;
                anchors[5] = 119.f;

                std::vector<obj_info_internal> objects16;
                generate_proposals(anchors, 16, cache_forward, out["output1"], prob_threshold, objects16);

                proposals.insert(proposals.end(), objects16.begin(), objects16.end());
            }

            // stride 32
            {
                std::vector<float> anchors(6);
                anchors[0] = 116.f;
                anchors[1] = 90.f;
                anchors[2] = 156.f;
                anchors[3] = 198.f;
                anchors[4] = 373.f;
                anchors[5] = 326.f;

                std::vector<obj_info_internal> objects32;
                generate_proposals(anchors, 32, cache_forward, out["output2"], prob_threshold, objects32);

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
        }

        std::vector<RESULT_DATA> detect_deepsort(exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int order, std::vector<obj_info_internal>& objects)
        {
            //¡Ÿ ±
            //convet param_span<uint8_t> bitmap to cv::Mat
            assert(channels == 1 || channels == 3);
            auto type = 0;
            channels == 1 ? type = CV_8UC1 : type = CV_8UC3;
            std::vector<int> sizes{ height, width };
            cv::Mat frame(sizes, CV_8UC3, bitmap.data());

            //¡Ÿ ±
            //convet vector<obj_info_internal> to vector<Detection>
            std::vector<Detection> detection_box;
            cv::Rect rec;
            rec.width;
            for (auto & obj : objects) {
                detection_box.push_back(
                    Detection{ 
                    cv::Rect(obj.rect.x, obj.rect.y, obj.rect.width, obj.rect.height), 
                    obj.prob, 
                    obj.label 
                    });
            }

            auto ret = deepsort_instance_.update(frame, detection_box);
            return ret;
        }

        pan::result_info detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
        {
            //only yolo
            {
#if 0
                std::vector<std::string> color{ "Black", "Blue", "Brown", "Gray", "Green", "Pink", "Red" };
                //std::vector<std::string> ori{ "Front", "Rear" };
                std::vector<exposing::param_string> type{ "sedan", "pickup", "van", "truck", "bus", "minibus", "N/A" };

                this->init_cache(bitmap, channels, height, width, order);

                std::vector<obj_info_internal> objects;
                detect_yolo(channels, height, width, order, objects);


                result_info_internal result_internal;
                result_internal.coordinates = exposing::make_param_vector<exposing::param_vector<float>>();
                result_internal.conf = exposing::make_param_vector<float>();
                result_internal.cls = exposing::make_param_vector<exposing::param_string>();

                for (auto obj : objects) {
                    //box
                    exposing::param_vector<float> box = exposing::make_param_vector<float>();

                    box.push_back(obj.rect.x);
                    box.push_back(obj.rect.y);
                    box.push_back(obj.rect.x + obj.rect.width);
                    box.push_back(obj.rect.y + obj.rect.height);
                    result_internal.coordinates.push_back(box);

                    //conf
                    result_internal.conf.push_back(obj.prob);

                    //cls
                    result_internal.cls.push_back(type[obj.label]);

                }
                auto result = result_info(exposing::make_as_first<pan::result_info_impl>(result_internal));

                return result;
#endif
            }

            //yolov+deepsort
            {
                //yolo
                std::vector<exposing::param_string> type{ "sedan", "pickup", "van", "truck", "bus", "minibus", "N/A" };

                this->init_cache(bitmap, channels, height, width, order);

                std::vector<obj_info_internal> objects;
                detect_yolo(channels, height, width, order, objects);


                //deepsort
                auto ret = detect_deepsort(bitmap, channels, height, width, order, objects);


                //convert result
                result_info_internal result_internal;
                result_internal.coordinates = exposing::make_param_vector<exposing::param_vector<float>>();
                result_internal.conf = exposing::make_param_vector<float>();
                result_internal.cls = exposing::make_param_vector<exposing::param_string>();
                result_internal.num = exposing::make_param_vector<int>();
                result_internal.speed = exposing::make_param_vector<float>();

                for (;;) {
                    //...
                    break;
                }

                auto result = result_info(exposing::make_as_first<pan::result_info_impl>(result_internal));

                return result;
            }
        }

        static std::string version()
        {
            return "1.0.0";
        }

    private:

        void init_cache(exposing::param_span<std::uint8_t>& bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order)
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


    private:
        int device_;
        excalibur::pipeline<float> yolov5m_instance_;
        //excalibur::pipeline<float> deepsort_instance_;
        DeepSort deepsort_instance_;

        std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
    };

    yolov5Deepsort_net_internal::yolov5Deepsort_net_internal(std::string_view yolov5m_racy_path, std::string_view deepsort_racy_path, int device)
        : impl_{ std::make_unique<impl>(yolov5m_racy_path, deepsort_racy_path, device) }
    {
    }

    yolov5Deepsort_net_internal::yolov5Deepsort_net_internal(std::string_view yolov5m_phai, std::string_view yolov5m_racy_path, std::string_view deepsort_phai, std::string_view deepsort_racy_path, int device)
        : impl_{ std::make_unique<impl>(yolov5m_phai, yolov5m_racy_path, deepsort_phai, deepsort_racy_path, device) }
    {
    }

    yolov5Deepsort_net_internal::yolov5Deepsort_net_internal(const std::vector<std::string>& yolov5m_phai, std::string_view yolov5m_racy_path, const std::vector<std::string>& deepsort_phai, std::string_view deepsort_racy_path, int device)
        : impl_{ std::make_unique<impl>(yolov5m_phai, yolov5m_racy_path, deepsort_phai, deepsort_racy_path, device) }
    {
    }

    yolov5Deepsort_net_internal::yolov5Deepsort_net_internal()
    {
    }

    yolov5Deepsort_net_internal::~yolov5Deepsort_net_internal()
    {
    }

    std::string yolov5Deepsort_net_internal::version()
    {
        return impl::version();
    }

    pan::result_info yolov5Deepsort_net_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
    {
        return impl_->detect(bitmap, channels, height, width, order);
    }
}