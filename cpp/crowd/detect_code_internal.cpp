#include <iostream>
#include <cmath>
#include <tuple>

#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"
#include "find_cluster_num.hpp"

#include <abi/param_vector.hpp>
#include <utility>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <iomanip>
#include <GenPipeline/GenPipeline.hpp>

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
    #include <RKNN2Wrapper/rknn2_wrapper.hpp>
#endif
const int molel_w = 512;
const int molel_h = 576;


namespace glasssix::crowd
{
    class detect_code_internal::impl
    {
    public:
        impl(const exposing::param_string model_directory, int device = -1)
            : impl{ exposing::to_narrow_string(model_directory), device }
        {
        }

        impl(std::string model_directory, int device)
        {
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::vector<std::string> phai;
            net_segment0_ = std::make_unique<rknnwrapper::rknn_wrapper>(phai, std::string(model_directory) + std::string("/crowdcount_sim0.rknn"), device),
                net_segment1_ = std::make_unique<rknnwrapper::rknn_wrapper>(phai, std::string(model_directory) + std::string("/crowdcount_sim1.rknn"), device);
#elif defined(USE_BMNN)
            net_crowd_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/crowdcount_sim.bmodel", device);
            net_crowd_->manual_possible_normalization(std::array<float, 3>{113.7f, 104.4f, 100.7f}, std::array<float, 3>{0.01360544, 0.014084507, 0.01383125864});
#endif

        }

        exposing::param_vector<crowd::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, int min_cluster_size, std::map<std::string, float>& param_map)
        {
            if (min_cluster_size <= 0)
                throw exposing::abi_invalid_argument("min_cluster_size < = 0");

            if (bitmap.empty())
                throw exposing::abi_invalid_argument("current frame is empty");

            int min_area_threshold = std::round(param_map.count("area_threshold") ? param_map["area_threshold"] : 15.f);

            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
            if (roi_x<0 || roi_x>width || roi_y > height || roi_y < 0 || roi_height<0 || (roi_height + roi_y) >height || roi_width<0 || (roi_width + roi_x) > width)
                throw exposing::abi_invalid_argument("incorrect roi in crowd");

            cv::Mat cropped_image = image(cv::Range(roi_y, roi_y + roi_height), cv::Range(roi_x, roi_x + roi_width));
            float pic_scale = cropped_image.cols > cropped_image.rows ? static_cast<float>(cropped_image.rows) * 1.0f / molel_h : static_cast<float>(cropped_image.cols) * 1.0f / molel_w;

            cv::resize(cropped_image, cropped_image, cv::Size((int)(cropped_image.cols / pic_scale), (int)(cropped_image.rows / pic_scale)), cv::INTER_LINEAR);

            int cropwidth = cropped_image.cols;
            int cropheight = cropped_image.rows;
            int xslice = (cropwidth + molel_w - 1) / molel_w;
            int yslice = (cropheight + molel_h - 1) / molel_h;
            int pad_h = yslice * molel_h - cropheight;
            int pad_w = xslice * molel_w - cropwidth;

            if (pad_h > 0 || pad_w > 0)
                cv::copyMakeBorder(cropped_image, cropped_image, 0, pad_h, 0, pad_w, cv::BORDER_CONSTANT, cv::Scalar{ 0,0,0 });

            cropwidth = cropped_image.cols;
            cropheight = cropped_image.rows;

            std::vector<cluster_info> detection_points;
            for (int i = 0; i < yslice; i++)
            {
                for (size_t j = 0; j < xslice; j++)
                {
                    int cropx1 = j * molel_w;
                    int cropx2 = (j + 1) * molel_w > cropwidth ? cropwidth : (j + 1) * molel_w;
                    int cropy1 = i * molel_h;
                    int cropy2 = (i + 1) * molel_h > cropheight ? cropheight : (i + 1) * molel_h;

                    auto result = run_segment(cropped_image, cropx1, cropx2, cropy1, cropy2, min_area_threshold);
                    for (auto& it : result)
                    {
                        it.x1 *= pic_scale;
                        it.x2 *= pic_scale;
                        it.y1 *= pic_scale;
                        it.y2 *= pic_scale;

                        it.x1 += roi_x;
                        it.x2 += roi_x;
                        it.y1 += roi_y;
                        it.y2 += roi_y;
                        detection_points.push_back(cluster_info{ .x1 = it.x1 ,.y1 = it.y1 ,.x2 = it.x2 ,
                        .y2 = it.y2 ,.x = (it.x1 + it.x2) * 0.5,.y = (it.y1 + it.y2) * 0.5 });
                    }

                }
            }

            std::vector<crowd::box_info> cluster_list = find_cluster_num(detection_points, min_cluster_size);
            auto results = exposing::make_param_vector<crowd::box_info>();
            if (cluster_list.size() == 0)
                return results;
            crowd::box_info cluster_key = find_cluster_key(cluster_list);

            int trigger_delay = std::round(param_map.count("trigger_delay") ? param_map["trigger_delay"] : 30.f);
            int device_id = std::round(param_map.count("device_id") ? param_map["device_id"] : 0.f);
            int max_area_list = std::round(param_map.count("max_area_list") ? param_map["max_area_list"] : 5.f);
            float nms_threshold = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.5f;
            bool is_crow = check_crow(cluster_key, trigger_delay, device_id, max_area_list, nms_threshold);

            if (is_crow)
            {
                for (auto val : cluster_list)
                {
                    if (val.category() == cluster_key.category())
                        results.push_back(val);
                }
            }
            return results;

        }

        std::string version()
        {
            const std::string algo_module_version = "2.3.0";

            std::string nn_frame_version = "dsd";

            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);

        }

        static std::mutex list_crowd_area_mutex;
        static std::map<int, std::list<crowd::box_info> > area1_map;
        static std::map<int, std::list<crowd::box_info> > area2_map;
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

        inline int ComputeArea(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2) {
            int x = std::max(0, std::min(ax2, bx2) - std::max(ax1, bx1));
            int y = std::max(0, std::min(ay2, by2) - std::max(ay1, by1));
            return x * y;
        }

        float area_iou(const std::list<crowd::box_info>& list_crowd_area1, crowd::box_info cluster_key)
        {
            if (list_crowd_area1.size() == 0)
                return  0;
            int average_x1{ 0 }, average_x2{ 0 }, average_y1{ 0 }, average_y2{ 0 };
            for (auto& val : list_crowd_area1)
            {
                average_x1 += val.x1();
                average_y1 += val.y1();
                average_x2 += val.x2();
                average_y2 += val.y2();
            }
            average_x1 /= list_crowd_area1.size();
            average_y1 /= list_crowd_area1.size();
            average_x2 /= list_crowd_area1.size();
            average_y2 /= list_crowd_area1.size();

            float inter_area = ComputeArea(cluster_key.x1(), cluster_key.y1(), cluster_key.x2(), cluster_key.y2(),
                average_x1, average_y1, average_x2, average_y2) * 1.0;
            float union_area = (average_y2 - average_y1) * (average_x2 - average_x1) + (cluster_key.y2() - cluster_key.y1()) * (cluster_key.x2() - cluster_key.x1()) - inter_area;
            return  std::min(1.0f, std::max(0.0f, inter_area / union_area));
        }

        bool check_crow(crowd::box_info cluster_key, int trigger_delay, int device_id, int max_area_list, float nms_threshold)
        {
            std::lock_guard<std::mutex> lock(list_crowd_area_mutex);
            bool flag = false;
            std::list<crowd::box_info>& list_crowd_area1 = area1_map[device_id];
            std::list<crowd::box_info>& list_crowd_area2 = area2_map[device_id];
            if (list_crowd_area1.size() == 0)
                list_crowd_area1.push_back(cluster_key);
            else
            {
                float iou_ratio = area_iou(list_crowd_area1, cluster_key);
                if (iou_ratio > nms_threshold)
                    list_crowd_area1.push_back(cluster_key);
                else
                    list_crowd_area2.push_back(cluster_key);
            }
            if (list_crowd_area2.size() > max_area_list)
            {
                list_crowd_area1.clear();
                list_crowd_area2.clear();
                list_crowd_area1.push_back(cluster_key);
                flag = false;
            }
            else if (list_crowd_area1.size() > trigger_delay / 5)
            {
                flag = true;
                list_crowd_area1.pop_front();//删除第一个元素
                //printf("list_crowd_area1 len =%llu\n", list_crowd_area1.size());
            }
            //printf("list_crowd_area2 len =%llu\n", list_crowd_area2.size()); 
            return flag;
        }

        crowd::box_info find_cluster_key(std::vector<crowd::box_info>& cluster_list)
        {
            struct info {
                int cnt;
                int x1, x2, y1, y2;
            };
            std::unordered_map<int, info>dd;
            for (auto val : cluster_list)
            {
                int category_ = val.category();
                if (dd.find(category_) == dd.end())
                {
                    info now = info{ .cnt = 1 , .x1 = val.x1(), .x2 = val.x2(), .y1 = val.y1(), .y2 = val.y2() };
                    dd[val.category()] = now;
                }
                else
                {
                    dd[val.category()].x1 = std::min(dd[val.category()].x1, val.x1());
                    dd[val.category()].y1 = std::min(dd[val.category()].y1, val.y1());
                    dd[val.category()].x2 = std::max(dd[val.category()].x2, val.x2());
                    dd[val.category()].y2 = std::max(dd[val.category()].y2, val.y2());
                    dd[val.category()].cnt++;
                }
            }
            int ans_id = cluster_list[0].category();
            for (const auto val : cluster_list)
            {
                if (dd[val.category()].cnt > dd[ans_id].cnt)
                {
                    ans_id = val.category();
                }
                else if (dd[val.category()].cnt == dd[ans_id].cnt && dd[val.category()].x2 - dd[val.category()].x1 < dd[ans_id].x2 - dd[ans_id].x1)
                {
                    ans_id = val.category();
                }
            }

            crowd::box_info_internal ans;
            ans.x1 = dd[ans_id].x1;
            ans.y1 = dd[ans_id].y1;
            ans.x2 = dd[ans_id].x2;
            ans.y2 = dd[ans_id].y2;
            ans.category = ans_id;
            return glasssix::exposing::make_as_first<box_info_impl>(ans);
        }

        std::vector<crowd::box_info> find_cluster_num(const std::vector<cluster_info>& detection_points, int min_cluster_size)
        {
            std::vector<crowd::box_info> results;
            if (detection_points.size() < 3)
                return results;
            cluster_num scaler;
            std::vector<int> cluster_num_ans = scaler.find_cluster_num(detection_points, min_cluster_size);

            //crowd::box_info_internal
            //results.push_back(glasssix::exposing::make_as_first<box_info_impl>(it));
            for (int i = 0; i < detection_points.size(); ++i)
            {
                if (cluster_num_ans[i] == 0 || cluster_num_ans[i] == -1)
                    continue;
                crowd::box_info_internal temp;
                temp.x1 = detection_points[i].x1;
                temp.y1 = detection_points[i].y1;
                temp.x2 = detection_points[i].x2;
                temp.y2 = detection_points[i].y2;
                temp.category = cluster_num_ans[i];
                results.push_back(glasssix::exposing::make_as_first<box_info_impl>(temp));
            }
            return results;
        }

        std::vector<detect_list> get_boxInfo_from_Binar_map(std::vector<int>& binar_map, int min_area = 15)
        {
            std::vector<detect_list> result_part;
            int width = molel_w;
            int height = molel_h;
            cv::Mat grayImage(height, width, CV_8UC1);
            for (int i = 0; i < height; i++)
                for (int j = 0; j < width; j++)
                {
                    int index = i * width + j;
                    grayImage.at<uchar>(i, j) = static_cast<uchar>(binar_map[index] * 255);
                }

            cv::Mat labeledImage;
            cv::Mat stats;
            cv::Mat centroids;

            int numLabels = cv::connectedComponentsWithStats(grayImage, labeledImage, stats, centroids, 4);

            for (int i = 1; i < numLabels; ++i)  // 忽略背景标签 0
            {
                if (stats.at<int>(i, cv::CC_STAT_AREA) > min_area)
                {
                    detect_list box;
                    box.x1 = stats.at<int>(i, cv::CC_STAT_LEFT);
                    box.y1 = stats.at<int>(i, cv::CC_STAT_TOP);
                    box.x2 = stats.at<int>(i, cv::CC_STAT_LEFT) + stats.at<int>(i, cv::CC_STAT_WIDTH);
                    box.y2 = stats.at<int>(i, cv::CC_STAT_TOP) + stats.at<int>(i, cv::CC_STAT_HEIGHT);

                    result_part.push_back(box);
                }
            }
            return result_part;
        }

        std::vector<detect_list> post_process(const float* pred_map, const float* predict, int area_threshold, int size = 512)
        {
            int len = molel_w * molel_h;
            std::vector<int> binar_map(len);
            for (size_t i = 0; i < len; i++)
            {
                if (pred_map[i] >= predict[i])
                    binar_map[i] = 1;
                else
                    binar_map[i] = 0;
            }
            return get_boxInfo_from_Binar_map(binar_map, area_threshold);
        }

        std::vector<crowd::box_info_internal>  run_segment(cv::Mat& images, int cropx1, int cropx2, int cropy1, int cropy2, int area_threshold)
        {
            std::vector<box_info_internal> output;
            cv::Rect rect{ cropx1, cropy1, cropx2 - cropx1, cropy2 - cropy1 };
            cv::Mat blobs = images(rect).clone();
            if (blobs.cols != molel_w || blobs.rows != molel_h)
                throw exposing::abi_invalid_argument("img size error in crowd");

            cv::cvtColor(blobs, blobs, cv::COLOR_BGR2RGB);

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forward_result;
            auto  network_results = net_segment0_->forward(blobs.data, { 1, blobs.rows, blobs.cols,blobs.channels() }, RKNN_TENSOR_NHWC);
            forward_result.push_back(network_results["input.2292"]);
            forward_result.push_back(network_results["pred_map"]);
            std::vector<int> shape1 = { 1, 720, 72, 64 };
            auto Mul_268 = std::make_shared<glasssix::memory::tensor<float>>(shape1, -1, glasssix::memory::NCHW);
            std::vector<int> shape2 = { 1, 1, 72, 64 };
            auto Mul_263 = std::make_shared<glasssix::memory::tensor<float>>(shape2, -1, glasssix::memory::NCHW);
            auto input_188 = std::make_shared<glasssix::memory::tensor<float>>(shape1, -1, glasssix::memory::NCHW);

            resize_nearst(forward_result[0]->cpu_data(), Mul_268->mutable_cpu_data(), 144, 128, 72, 64, 720);
            resize_nearst(forward_result[1]->cpu_data(), Mul_263->mutable_cpu_data(), 576, 512, 72, 64, 1);
            Mul_77(Mul_268->cpu_data(), Mul_263->cpu_data(), input_188->mutable_cpu_data(), 72 * 64, 720);

            std::vector<float> input_data(720 * 72 * 64);

            nchw2Nhwc(input_188->mutable_cpu_data(), input_data.data(), 1, 720, 72, 64);
            auto  network_result1 = net_segment1_->forward(input_data.data(), { 1, 72, 64,720 }, RKNN_TENSOR_NHWC);

            /**
            * @fun forward part end
            * @param  none
            * @return tensor(preprocess(image))
            */

            const float* predict = network_result1["predict"]->cpu_data();
            const float* pred_map = forward_result[1]->cpu_data();


#elif defined(USE_BMNN)
            auto  net_crowd__result = net_crowd_->forward(blobs);
            const float* predict = net_crowd__result["predict_Resize_f32"]->cpu_data();
            const float* pred_map = net_crowd__result["pred_map_Sigmoid"]->cpu_data();
#endif
            auto result_part = post_process(pred_map, predict, area_threshold);

            for (auto& iter : result_part)
            {
                box_info_internal  headp;
                headp.x1 = iter.x1 + cropx1;
                headp.x2 = iter.x2 + cropx1;
                headp.y1 = iter.y1 + cropy1;
                headp.y2 = iter.y2 + cropy1;
                output.push_back(headp);
            }
            return output;
        }


    private:
        std::string model_directory_;
        int device_;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::unique_ptr<glasssix::rknnwrapper::rknn_wrapper> net_segment0_;
        std::unique_ptr<glasssix::rknnwrapper::rknn_wrapper> net_segment1_;

#elif defined(USE_BMNN)
        std::shared_ptr<GenPipeline> net_crowd_;
#endif


    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
        : impl_{ std::make_unique<impl>(model_directory, device) }
    {
    }

    detect_code_internal::~detect_code_internal() = default;

    std::string detect_code_internal::version()
    {
        return impl_->version();
    }

    exposing::param_vector<crowd::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, int min_cluster_size, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, min_cluster_size, param_map);
    }

    std::mutex detect_code_internal::impl::list_crowd_area_mutex;
    std::map<int, std::list<crowd::box_info> >detect_code_internal::impl::area1_map;
    std::map<int, std::list<crowd::box_info> >detect_code_internal::impl::area2_map;
}
