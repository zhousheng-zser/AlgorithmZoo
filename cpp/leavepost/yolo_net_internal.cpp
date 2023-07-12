#include "yolo_net_internal.hpp"
#include "hardcode.hpp"

#include <algorithm>
#include "box_info_impl.hpp"
#include <Excalibur/pipeline.hpp>
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_resize.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <RKNN2Wrapper/rknn2_wrapper.hpp>
#include <cfloat>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

namespace glasssix::leavepost
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

    class yolo_net_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
                :impl{hardcode::get_model_params("hat_simp", false),  exposing::to_narrow_string(model_directory), device}
        {
        }

        impl(const std::vector<std::string> &phai, std::string model_directory, int device)
                :net_instance_(phai,  model_directory + std::string("/hat_simp.rknn"), device)
        {
        }

        exposing::param_vector<leavepost::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            std::vector<box_info_internal> objects;
            cv::Mat image(cv::Size(width, height), CV_8UC3);
            std::memcpy(image.data, bitmap.data(), sizeof (uint8_t) * channels * height * width);
            if(roi_x<0 || roi_x>width || roi_y>height || roi_y<0 ||roi_height<0 || (roi_height+roi_y) >height || roi_width<0 || (roi_width+roi_x) > width)
            {
                  throw exposing::abi_invalid_argument("incorrect roi in refvest");
            }
            cv::Mat cropped_image = image(cv::Range(roi_y,roi_y+roi_height), cv::Range(roi_x,roi_x+roi_width));
            detect_yolo(cropped_image, objects,param_map);

            auto result = exposing::make_param_vector<box_info>();
            for (auto &i : objects)
            {
                auto x=i;
                x.rect.x+=roi_x;
                x.rect.y+=roi_y;

                result.push_back(exposing::make_as_first<box_info_impl>(i));
                // cv::rectangle(image, cv::Point(i.rect.x, i.rect.y), cv::Point(i.rect.x+i.rect.width, i.rect.y+i.rect.height),        cv::Scalar(0, 0, 255), 3);
            }
            // cv::imwrite("../detets.jpg",image);
            return result;
        }

        std::string version()
		{
			const std::string algo_module_version = "1.0.0";

			std::string nn_frame_version = net_instance_.version();

			return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
		}


    private:       
        inline float intersection_area(const box_info_internal &a, const box_info_internal &b)
        {
            anchor_box inter = a.rect & b.rect;
            return inter.width * inter.height;
        }

        void qsort_descent_inplace(std::vector<box_info_internal> &faceobjects, int left, int right)
        {
            int i = left;
            int j = right;
            float p = faceobjects[(left + right) / 2].confidence;

            while (i <= j)
            {
                while (faceobjects[i].confidence > p)
                    i++;

                while (faceobjects[j].confidence < p)
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

        void qsort_descent_inplace(std::vector<box_info_internal> &faceobjects)
        {
            if (faceobjects.empty())
                return;

            qsort_descent_inplace(faceobjects, 0, faceobjects.size() - 1);
        }

        void nms_sorted_bboxes(const std::vector<box_info_internal> &faceobjects, std::vector<int> &picked, float nms_threshold)
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
                const box_info_internal &a = faceobjects[i];

                int keep = 1;
                for (int j = 0; j < (int)picked.size(); j++)
                {
                    const box_info_internal &b = faceobjects[picked[j]];

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

        void generate_proposals(const std::vector<float> &anchors, int stride, const std::shared_ptr<memory::tensor<float>> &feat_blob, float prob_threshold, std::vector<box_info_internal> &objects)
        {
            const int num_grid = feat_blob->height();
            int num_grid_x;
            int num_grid_y;

            num_grid_y = 640/ stride;
            num_grid_x = num_grid / num_grid_y;

            const int num_class = feat_blob->width() - 5;

            const int num_anchors = anchors.size() / 2;

            for (size_t q = 0; q < num_anchors; q++)
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
                        float confidence = sigmoid(box_score);

                        if (confidence >= prob_threshold)
                        {
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

                            box_info_internal obj;
                            obj.rect.x = x0;
                            obj.rect.y = y0;
                            obj.rect.width = x1 - x0;
                            obj.rect.height = y1 - y0;
                            obj.label = class_index;
                            obj.confidence = confidence;

                            objects.push_back(obj);
                        }
                    }
                }
            }
        }

        std::pair<cv::Mat, float> preprocess(cv::Mat& src,int& hpad,int& wpad ,const cv::Size& input_shape = cv::Size(640, 640) )
        {      
            float scale = std::min((float)input_shape.width/(float)src.cols, (float)input_shape.height/(float)src.rows);
            cv::Mat cut_image;
            cv::Mat mask_image(input_shape, CV_8UC3, cv::Scalar(114, 114, 114));
            if(src.cols!=input_shape.width || src.rows!=input_shape.height )
            {
                cv::resize(src, cut_image, cv::Size((int)(src.cols * scale), (int)(src.rows * scale)), cv::INTER_LINEAR);
                hpad = int((input_shape.height - cut_image.rows)  ) ; 
                wpad = int((input_shape.width - cut_image.cols)  ) ; 
                cv::copyMakeBorder(cut_image, mask_image,  hpad/2, input_shape.height-cut_image.rows-hpad/2, wpad/2 , input_shape.width-cut_image.cols-wpad/2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
            }
            else
            {
                mask_image=src;
            }
            return { mask_image,scale};
        }


        int detect_yolo(cv::Mat& image, std::vector<box_info_internal> &objects, std::map<std::string, float>& param_map)
        {
            /** Before processing **/
            const int target_size = 640;
            // const float prob_threshold = 0.5f;
            // const float nms_threshold = 0.4f;

            float prob_threshold= param_map.count("conf_thres") ? param_map["conf_thres"] : 0.5f;
            float nms_threshold = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.4f;      
            cv::Mat blob;
            int hpad=0;
            int wpad=0;
            float scale = 1.f;//scale是放缩系数
            std::tie(blob,scale) = preprocess(image,hpad,wpad);

            auto  out = net_instance_.forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);



            std::vector<box_info_internal> proposals;
            {
                std::vector<float> anchors(6);
                anchors[0] = 10.f;
                anchors[1] = 13.f;
                anchors[2] = 16.f;
                anchors[3] = 30.f;
                anchors[4] = 33.f;
                anchors[5] = 23.f;

                std::vector<box_info_internal> objects8;

                std::shared_ptr<memory::tensor<float>> temp_out(new memory::tensor<float>(3,6400, 8, -1, memory::NCHW, nullptr));
                std::copy(out["751"]->mutable_cpu_data(), out["751"]->mutable_cpu_data() + 3*6400*8, temp_out->mutable_cpu_data());

                generate_proposals(anchors, 8, temp_out, prob_threshold, objects8);
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

                std::vector<box_info_internal> objects16;

                std::shared_ptr<memory::tensor<float>> temp_out(new memory::tensor<float>(3,1600, 8, -1, memory::NCHW, nullptr));
                std::copy(out["1060"]->mutable_cpu_data(), out["1060"]->mutable_cpu_data() + 3*1600*8, temp_out->mutable_cpu_data());

                generate_proposals(anchors, 16, temp_out, prob_threshold, objects16);
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

                std::vector<box_info_internal> objects32;

                std::shared_ptr<memory::tensor<float>> temp_out(new memory::tensor<float>(3,400, 8, -1, memory::NCHW, nullptr));
                std::copy(out["1369"]->mutable_cpu_data(), out["1369"]->mutable_cpu_data() + 3*400*8, temp_out->mutable_cpu_data());

                generate_proposals(anchors, 32, temp_out, prob_threshold, objects32);
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
                float x0 = (objects[i].rect.x - (wpad / 2)) / scale;
                float y0 = (objects[i].rect.y - (hpad / 2)) / scale;
                float x1 = (objects[i].rect.x + objects[i].rect.width - (wpad / 2)) / scale;
                float y1 = (objects[i].rect.y + objects[i].rect.height - (hpad / 2)) / scale;

                // clip
                x0 = std::max(std::min(x0, (float)(image.cols - 1)), 0.f);
                y0 = std::max(std::min(y0, (float)(image.rows - 1)), 0.f);
                x1 = std::max(std::min(x1, (float)(image.cols - 1)), 0.f);
                y1 = std::max(std::min(y1, (float)(image.rows - 1)), 0.f);

                objects[i].rect.x = x0;
                objects[i].rect.y = y0;
                objects[i].rect.width = x1 - x0;
                objects[i].rect.height = y1 - y0;
            }

            return 0;
        }
    
    private:
        int device_;
        rknnwrapper::rknn_wrapper  net_instance_;
        std::string model_directory_;
        std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
    };

    yolo_net_internal::yolo_net_internal(std::string_view model_directory, int device) 
    : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    yolo_net_internal::~yolo_net_internal()
    {
    }

    exposing::param_vector<leavepost::box_info> yolo_net_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const
    {
          return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
    }

	std::string yolo_net_internal::version()
	{
		return impl_->version();
	}
}
