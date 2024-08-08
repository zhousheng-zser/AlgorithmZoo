#include <iostream>
#include <cmath>
#include <tuple>
#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include "logger.hpp"
#include <chrono>
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
    #include <GenPipeline/GenPipeline.hpp>
    #include <YoloFamily/Yolo_wrapper.hpp>
#elif defined(USE_BMNN)
    #include <sophonyolov8/SophonYolov8Wrapper.hpp>
#endif
#include "general.hpp"
#include <abi/param_vector.hpp>
#include <utility>

namespace glasssix::pump_mask
{
    struct boxes_conf
    {
        float top_x;
        float top_y;
        float bot_x;
        float bot_y;
        float conf;
        int category;

        std::vector<float> key_points;

        float x1;
        float y1;
        float x2;
        float y2;
    };

    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device)
            : model_directory_{ std::string(model_directory) }, device_{ device }
        {
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::string model_ext{ ".rknn" };
#elif defined(USE_BMNN)
            std::string model_ext{ ".bmodel" };
#else
            std::string model_ext{ ".onnx" };
#endif
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            net_mask_ = std::make_shared<GenPipeline>(model_directory_ + "/pump_mask" + model_ext, device);
            yolov8_instance_mask = std::make_shared<Yolov8_Complement<GenPipeline, true, false>>(160, 160, net_mask_); //2个模板变量分别对应 Yolov8_Complement ，(通用yolov8)是否是李鑫尧的yolo  第三个参数默认为false
            net_head_ = std::make_shared<GenPipeline>(model_directory_ + "/pump_mask_head" + model_ext, device);
            yolov8_instance_head = std::make_shared<Yolov8_Complement<GenPipeline, true, false>>(1152, 640, net_head_); //2个模板变量分别对应 GenPipeline ，(通用yolov8)是否是李鑫尧的yolo  第三个参数默认为false
            net_detect_face = std::make_shared<GenPipeline>(model_directory_ + "/pump_mask_face" + model_ext, device);
#elif defined(USE_BMNN)
            yolov8_instance_mask = std::make_shared<SophonYolov8Wrapper>(model_directory_ + "/pump_mask" + model_ext, device);
            yolov8_instance_mask->init();
            yolov8_instance_head = std::make_shared<SophonYolov8Wrapper>(model_directory_ + "/pump_mask_head" + model_ext, device);
            yolov8_instance_head->init();
            net_detect_face = std::make_shared<SophonYolov8Wrapper>(model_directory_ + "/pump_mask_face" + model_ext, device);
            net_detect_face->init();
#endif
        }

        std::string version()
        {
            const std::string algo_module_version = "2.0.0";
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            std::string nn_frame_version = "1.0.0";
#else
            std::string nn_frame_version = "1.0.0";
#endif
            return fmt::format(R"({{"nn_frame_version":"{}", "algo_module_version":"{}"}})", nn_frame_version, algo_module_version);
        }

        std::vector<int> convert_center_xywh_to_xyxy(int x, int y, int w, int h) {
            return { x - (w >> 1), y - (h >> 1), x + (w >> 1), y + (h >> 1) };
        }

        std::vector<int> convert_xyxy_to_center_xywh(int x1, int y1, int x2, int y2) {
            return { x1 + x2 >> 1, y1 + y2 >> 1, x2 - x1, y2 - y1 };
        }

        std::vector<int> change_box_of_width_and_height(std::vector<int> box, int width, int height, int image_w, int image_h, bool add_boundary = false) {
            auto center = convert_xyxy_to_center_xywh(box[0], box[1], box[2], box[3]);
            int x_center = center[0];
            int y_center = center[1];
            int w = center[2];
            int h = center[3];

            int new_w = add_boundary ? (w + width) : width;
            int new_h = add_boundary ? (h + height) : height;

            std::vector<int> xyxy = convert_center_xywh_to_xyxy(x_center, y_center, new_w, new_h);

            int x1 = std::max(0, xyxy[0]);
            int y1 = std::max(0, xyxy[1]);
            int x2 = std::min(image_w, xyxy[2]);
            int y2 = std::min(image_h, xyxy[3]);

            return { x1, y1, x2, y2 };
        }

        cv::Mat process_of_image_by_stage1(cv::Mat& image, int image_w, int image_h, Bbox head_box, int& move_x, int& move_y)
        {
            int x1 = head_box.x1;
            int y1 = head_box.y1;
            int x2 = head_box.x2;
            int y2 = head_box.y2;
            int w = x2 - x1;
            int h = y2 - y1;
            int new_width = std::max(w, h) * 1.5;

            std::vector<int> new_head_box = change_box_of_width_and_height({ x1, y1, x2, y2 }, new_width, new_width, image_w, image_h, false);

            cv::Mat cropped_image = image(cv::Range(new_head_box[1], new_head_box[3]), cv::Range(new_head_box[0], new_head_box[2])).clone();
            move_x = new_head_box[0];
            move_y = new_head_box[1];
            return cropped_image;
        }

        inline float ComputeArea(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2) {
            float x = std::max(0, std::min(ax2, bx2) - std::max(ax1, bx1));
            float y = std::max(0, std::min(ay2, by2) - std::max(ay1, by1));
            return x * y;
        }
        float iou(Bbox box1, Bbox box2, bool is_a = true, bool is_b = true)
        {
            int ax1 = box1.x1;
            int ay1 = box1.y1;
            int ax2 = box1.x2;
            int ay2 = box1.y2;
            int bx1 = box2.x1;
            int by1 = box2.y1;
            int bx2 = box2.x2;
            int by2 = box2.y2;
            float over_area = ComputeArea(ax1, ay1, ax2, ay2, bx1, by1, bx2, by2);
            float sum_a = (ax2 - ax1) * (ay2 - ay1) + 0.0001;
            float sum_b = (bx2 - bx1) * (by2 - by1) + 0.0001;

            if (is_a == 0 && is_b == 0)
                return 0;
            if (is_a && is_b)
                return over_area / (sum_a + sum_b - over_area);
            if (is_a)
                return over_area / sum_a;

            return over_area / sum_b;
        }

        std::vector<Bbox> detect_wear_mask_by_result_stage_2(std::vector<Bbox>& cropped_result, std::vector<Bbox>& face_box_list)
        {
            std::vector<Bbox> head_box_list;
            std::vector<Bbox> face_mask_box_list;
            std::vector<Bbox> gas_mask_box_list;
            for (int i = 0; i < cropped_result.size(); i++)
            {
                if (cropped_result[i].category == 0 && (cropped_result[i].x2 - cropped_result[i].x1 > 30 || cropped_result[i].y2 - cropped_result[i].y1 > 30))
                    head_box_list.push_back(cropped_result[i]);
                else if (cropped_result[i].category == 1)
                    face_box_list.push_back(cropped_result[i]);
                else if (cropped_result[i].category == 2)
                    face_mask_box_list.push_back(cropped_result[i]);
                else if (cropped_result[i].category == 3)
                    gas_mask_box_list.push_back(cropped_result[i]);
            }
            std::vector<Bbox>unwear_gas_mask_head_list;
            for (auto& head_box : head_box_list)
            {
                std::vector<Bbox> the_gas_mask_box_list;
                for (int i = 0; i < gas_mask_box_list.size(); i++)
                {
                    if (iou(gas_mask_box_list[i], head_box, true, false) >= 0.9)
                        the_gas_mask_box_list.push_back(gas_mask_box_list[i]);
                }
                std::sort(the_gas_mask_box_list.begin(), the_gas_mask_box_list.end(), [](Bbox a, Bbox b) {return a.score > b.score; });
                if (the_gas_mask_box_list.size() && the_gas_mask_box_list[0].score > 0)
                    continue;


                std::vector<Bbox> the_face_box_list;
                for (int i = 0; i < face_box_list.size(); i++)
                {
                    if (iou(face_box_list[i], head_box, true, false) >= 0.9)
                        the_face_box_list.push_back(face_box_list[i]);
                }
                double face_box_conf = 0;
                std::sort(the_face_box_list.begin(), the_face_box_list.end(), [](Bbox a, Bbox b) {return a.score > b.score; });
                if (the_face_box_list.size())
                    face_box_conf = the_face_box_list[0].score;


                std::vector<Bbox> the_face_mask_box_list;
                for (int i = 0; i < face_mask_box_list.size(); i++)
                {
                    if (iou(face_mask_box_list[i], head_box, true, false) >= 0.9)
                        the_face_mask_box_list.push_back(face_mask_box_list[i]);
                }
                double face_mask_box_conf = 0;
                std::sort(the_face_mask_box_list.begin(), the_face_mask_box_list.end(), [](Bbox a, Bbox b) {return a.score > b.score; });
                if (the_face_mask_box_list.size())
                    face_mask_box_conf = the_face_mask_box_list[0].score;
                if (face_mask_box_conf == 0 && face_box_conf == 0)
                    continue;
                if (face_box_conf > face_mask_box_conf) {
                    head_box.score = face_box_conf;
                    head_box.category = 2;   //face
                }
                else {
                    head_box.score = face_mask_box_conf;
                    head_box.category = 3;   //face_mask
                }
                unwear_gas_mask_head_list.push_back(head_box);

            }
            return unwear_gas_mask_head_list;
        }

        std::tuple<cv::Mat, float> face_imgprocess(cv::Mat& src, int& pad_h, int& pad_w, cv::Size input_shape = cv::Size(640, 640)) {
            float scale = std::min((float)input_shape.width / (float)src.cols, (float)input_shape.height / (float)src.rows);
            cv::Mat cut_image;
            cv::Mat mask_image(input_shape, CV_8UC3, cv::Scalar(114, 114, 114));
            if (src.rows != input_shape.height || src.cols != input_shape.width)
            {
                cv::resize(src, cut_image, cv::Size((int)(src.cols * scale), (int)(src.rows * scale)), cv::INTER_LINEAR);

                pad_h = int((input_shape.height - cut_image.rows) / 2);
                pad_w = int((input_shape.width - cut_image.cols) / 2);
                cv::copyMakeBorder(cut_image, mask_image, pad_h, input_shape.height - cut_image.rows - pad_h, pad_w, input_shape.width - cut_image.cols - pad_w, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
            }
            else
            {
                src.copyTo(mask_image);
            }
            cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);
            return { mask_image,scale };
        }

        inline std::vector<boxes_conf> xywh2xyxy(std::vector<std::vector<float>>& src, float conf_thres = 0.f)
        {
            int index = 0;
            std::vector<boxes_conf> res;
            for (auto& it : src)
            {
                float top_x = it[0] - it[2] / 2;
                float top_y = it[1] - it[3] / 2;
                float bot_x = it[0] + it[2] / 2;
                float bot_y = it[1] + it[3] / 2;
                float conf = it[4];
                int maxPosition = 5;
                if (it[maxPosition] * conf > conf_thres)
                {
                    // std::cout<<it[maxPosition]<<std::endl;
                    boxes_conf temp{};
                    temp.top_x = top_x;
                    temp.top_y = top_y;
                    temp.bot_x = bot_x;
                    temp.bot_y = bot_y;
                    temp.conf = it[maxPosition] * conf;
                    temp.category = maxPosition - 5;

                    std::vector<float> points(15);
                    std::copy(it.data() + 6, it.data() + 21, points.data());

                    temp.key_points = points;

                    res.push_back(temp);
                }
                index++;
            }
            return res;
        }

        std::vector<Bbox> nms(std::vector<Bbox>& boxes, float threshold)
        {
            std::vector<Bbox>resluts;
            std::sort(boxes.begin(), boxes.end(), [](Bbox box1, Bbox box2) {
                return box1.score > box2.score ? true : false;
                });
            while (boxes.size() > 0)
            {
                resluts.push_back(boxes[0]);
                int index = 1;
                while (index < boxes.size()) {
                    float iou_value = iou(boxes[0], boxes[index]);
                    if (iou_value > threshold) {
                        boxes.erase(boxes.begin() + index);
                    }
                    else {
                        index++;
                    }
                }
                boxes.erase(boxes.begin());
            }
            return  resluts;
        }

        std::vector<Bbox> computeNmsInput(std::vector<boxes_conf>& src, float ratio, int pad_h, int pad_w)
        {
            std::vector<Bbox> boxes;
            std::vector<float> scores;
            std::vector<int> category;
            for (auto& it : src)
            {
                Bbox temp((it.top_x - pad_w) * ratio, (it.top_y - pad_h) * ratio,
                    (it.bot_x - pad_w) * ratio, (it.bot_y - pad_h) * ratio);
                temp.score = it.conf;
                temp.category = it.category;
                for (size_t i = 0; i < 5; i++)
                {
                    it.key_points[i * 3 + 0] = (it.key_points[i * 3 + 0] - pad_w) * ratio;
                    it.key_points[i * 3 + 1] = (it.key_points[i * 3 + 1] - pad_h) * ratio;
                }

                temp.key_points = it.key_points;
                boxes.push_back(temp);
            }
            return boxes;
        }

        void judge_valid_face(std::vector<Bbox>& boxes, std::vector<Bbox>& ans) {
            for (auto& it : boxes)
            {
                if (it.key_points.size() == 0)
                    continue;
                bool flag = true;
                if (it.key_points[0 * 3] < it.key_points[2 * 3] && it.key_points[2 * 3] < it.key_points[1 * 3] &&
                    it.key_points[1 * 3] - it.key_points[0 * 3] >= 5) {
                    ;
                }
                else
                    continue;
                if (std::max(it.key_points[0 * 3 + 1], it.key_points[1 * 3 + 1]) > it.key_points[2 * 3 + 1])
                    continue;
                for (int i = 0; flag && i < 5; ++i) {
                    if (it.key_points[i * 3 + 2] < 0.8)
                        flag = false;
                }
                if (flag)
                    ans.emplace_back(it);
            }
        }

        std::vector<Bbox> non_max_suppression(std::vector<std::vector<float>>& prediction, float conf_thres, float iou_thres, float ratio, int pad_h, int pad_w)
        {
            auto compute_box = xywh2xyxy(prediction, conf_thres);

            // Batched NMS
            int max_wh = 4096;
            std::vector<Bbox> boxes;
            std::vector<float> scores;
            std::vector<int> classes;

            boxes = computeNmsInput(compute_box, ratio, pad_h, pad_w);//此处做分类处理，因为有四类 而非以前的单类

            std::vector<Bbox> temp, ans;
            temp = nms(boxes, iou_thres);
            ans.clear();
            judge_valid_face(temp, ans);

            return std::move(ans);
        }

        //transpose  1*18*h*w -> 1*3*h*w*6
        void transpose(const float* in, float* out, int data_num)
        {
            int channel = 3;
            int info_struct = 21;
            int hw = data_num / (channel * info_struct);

            const float* src = in;
            float* dst = out;
            for (int i = 0; i < 3; i++)
            {
                for (size_t j = 0; j < 21; j++)
                {
                    for (size_t k = 0; k < hw; k++)
                    {
                        dst[i * hw * info_struct + k * info_struct + j] = src[i * info_struct * hw + j * hw + k];
                    }
                }
            }
            int l = 0;
        }

        exposing::param_vector<pump_mask::box_info> detect(const exposing::param_span<std::uint8_t>& bitmap, int channels, int height, int width,
            std::map<std::string, float>& param_map)
        {
            float con_thres = param_map.count("conf_thres") ? param_map["conf_thres"] : 0.6f;
            float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;
            float detect_thres = 0.3;

            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            CHECK_EQ(channels, 3);

            cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
            std::vector<Object> frame_result = yolov8_instance_head->get_objects(image, detect_thres, iou_thres);   //检测人体 人头
            std::vector<Bbox> person_box_list;
            std::vector<Bbox> head_box_list;
            std::vector<Bbox> valid_head_box_list;    ///在人体框里的有效人头

            for (Object& it : frame_result) {
                if (it.category == 0)
                    person_box_list.push_back(Bbox{ it.x1,it.y1,it.x2,it.y2,it.category,it.score,0 });
                else if (it.x2 - it.x1 > 30 || it.y2 - it.y1 > 30)
                    head_box_list.push_back(Bbox{ it.x1,it.y1,it.x2,it.y2,it.category,it.score,0 });
            }
            std::vector<box_info_internal> result;
            for (int j = 0; j < person_box_list.size(); ++j)
            {
                for (int i = 0; i < head_box_list.size(); ++i) {
                    if (iou(head_box_list[i], person_box_list[j], true, false) >= 0.99999) {
                        valid_head_box_list.push_back(head_box_list[i]);
                        break;
                    }
                }
            }
            auto fin_result = exposing::make_param_vector<pump_mask::box_info>();
            if (valid_head_box_list.size() == 0)
                return  fin_result;      ///没有人头直接返回空数组
            std::vector<Object> face_box_list_temp = net_detect_face->get_objects(image);// 检测人脸
            std::vector<Bbox> face_box_list;
            for (auto& face : face_box_list_temp)
            {
                Bbox box;
                box.x1 = face.x1;
                box.x2 = face.x2;
                box.y1 = face.y1;
                box.y2 = face.y2;
                face_box_list.push_back(box);
            }
            //std::vector<Bbox> face_box_list = net_detect_face->get_objects(image);// 检测人脸 
            std::vector<Bbox> unwear_gas_mask_head_box_list;
            for (int i = 0; i < valid_head_box_list.size(); i++)
            {
                int move_x, move_y;
                cv::Mat cropped_image = process_of_image_by_stage1(image, image.cols, image.rows, valid_head_box_list[i], move_x, move_y);
                //0: 'head', 1: 'face', 2: 'face_mask', 3: 'gas_mask'  #没用这个里的face
                std::vector<Object> cropped_result = yolov8_instance_mask->get_objects(cropped_image, con_thres, iou_thres);// 防护面罩检测
                std::vector<Bbox> frame_result;
                for (Object& it : cropped_result) {
                    frame_result.push_back(Bbox{ it.x1,it.y1,it.x2,it.y2,it.category,it.score,0 });
                }

                std::vector<Bbox> the_face_box_list;
                for (int j = 0; j < face_box_list.size(); ++j) {
                    if (iou(face_box_list[j], valid_head_box_list[i], true, false) >= 0.9)
                    {
                        Bbox temp = face_box_list[j];
                        temp.x1 -= move_x;
                        temp.x2 -= move_x;
                        temp.y1 -= move_y;
                        temp.y2 -= move_y;
                        the_face_box_list.push_back(temp);
                    }
                }
                auto fin_result_temp = detect_wear_mask_by_result_stage_2(frame_result, the_face_box_list);
                for (int j = 0; j < fin_result_temp.size(); j++) {
                    fin_result_temp[j].x1 += move_x, fin_result_temp[j].x2 += move_x;
                    fin_result_temp[j].y1 += move_y, fin_result_temp[j].y2 += move_y;
                    unwear_gas_mask_head_box_list.push_back(fin_result_temp[j]);
                }
            }

            for (auto& val : unwear_gas_mask_head_box_list)
            {
                if (val.score >= con_thres) {
                    box_info_internal temp_result;
                    temp_result.x1 = val.x1;
                    temp_result.y1 = val.y1;
                    temp_result.x2 = val.x2;
                    temp_result.y2 = val.y2;
                    temp_result.category = val.category;
                    temp_result.score = val.score;
                    fin_result.push_back(exposing::make_as_first<box_info_impl>(temp_result));
                }
            }

            return fin_result;
        }

    private:
        std::string model_directory_;
        int device_;
        int img_size = 1280;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::shared_ptr<GenPipeline> net_head_;
        std::shared_ptr<Yolov8_Complement<GenPipeline, true, false>> yolov8_instance_head;//人体人头
        std::shared_ptr<GenPipeline> net_mask_;
        std::shared_ptr<Yolov8_Complement<GenPipeline, true, false>> yolov8_instance_mask;// 防护面罩
        std::shared_ptr<GenPipeline> net_detect_face;// 人脸
#elif defined(USE_BMNN)
        std::shared_ptr<SophonYolov8Wrapper> yolov8_instance_head;//人体人头
        std::shared_ptr<SophonYolov8Wrapper> yolov8_instance_mask;// 防护面罩
        std::shared_ptr<SophonYolov8Wrapper> net_detect_face;// 人脸
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

    exposing::param_vector<pump_mask::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap,
        int channels, int height, int width, std::map<std::string, float>& param_map) const
    {
        return impl_->detect(bitmap, channels, height, width, param_map);
    }
}
