#include "yolov7_net.hpp"
#include "hardcode.hpp"
#include "face_info_impl.hpp"
#include <opencv2/opencv.hpp>

#include "tensor_conversions.hpp"
#include "Excalibur/operation_safty_cut.hpp"

namespace glasssix::longinus
{
    namespace
    {
        struct boxes_conf
        {
            float top_x;
            float top_y;
            float bot_x;
            float bot_y;
            float conf;
            int category;
        };

        typedef struct Bbox
        {
            int x;
            int y;
            int w;
            int h;
            float score;
            int category;
        }Bbox;

        struct location_char
        {
            int x1;
            int y1;
            int x2;
            int y2;
            int category;
            float confidence;
        };

        static inline std::tuple<cv::Mat, float> preprocess_detection(cv::Mat& src, int& pad_h, int& pad_w, cv::Size input_shape = cv::Size(640, 640))
        {
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

        //transpose  1*18*h*w -> 1*3*h*w*6
        static inline void transpose(const float* in, float* out, int data_num)
        {
            int hw = data_num / (3 * 6);

            const float* src = in;
            float* dst = out;
            for (int i = 0; i < 3; i++)
            {
                // src+=( 3*hw );
                // dst+=( 3*hw );
                for (size_t j = 0; j < 6; j++)
                {
                    for (size_t k = 0; k < hw; k++)
                    {
                        dst[k * 6 + j + i * hw * 6] = src[j * hw + k + i * hw * 6];
                    }
                }
            }

            int l = 0;
        }

        static inline float sigmoid_x(float x)
        {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

        static inline std::vector<std::vector<float>> concat(std::vector<float*>& outs, float conf_thres)
        {

            int img_size = 320;
            int category = 6;
            const float anchors[3][6] = { {4,5, 6,8, 10,12}, {15,19, 23,30, 39,52}, {72,97, 123,164, 209,297} };
            const float stride[3] = { 8.0, 16.0, 32.0 };//80 40 20 ->   30 15 60
            std::vector<std::vector<float>> result;
            for (int n = 0; n < 3; n++)
            {
                int num_grid_x = (int)(img_size / stride[n]);
                int num_grid_y = (int)(img_size / stride[n]);

                int ind = 0;
                float* ptr_out = outs[n];
                for (int q = 0; q < 3; q++)
                {
                    const float anchor_w = anchors[n][q * 2];
                    const float anchor_h = anchors[n][q * 2 + 1];
                    for (int i = 0; i < num_grid_x; i++)
                    {
                        for (int j = 0; j < num_grid_y; j++)
                        {
                            float* pdata = ptr_out + ind * category;
                            float box_score = sigmoid_x(pdata[4]);

                            float cx = (sigmoid_x(pdata[0]) * 2.f - 0.5f + j) * stride[n];  //cx
                            float cy = (sigmoid_x(pdata[1]) * 2.f - 0.5f + i) * stride[n];  //cy
                            float w = powf(sigmoid_x(pdata[2]) * 2.f, 2.f) * anchor_w;      //w
                            float h = powf(sigmoid_x(pdata[3]) * 2.f, 2.f) * anchor_h;      //h

                            std::vector<float> element = { cx, cy, w, h, box_score, sigmoid_x(pdata[5]) };
                            result.push_back(element);

                            ind++;
                        }
                    }
                }
            }
            return result;
        }

        static inline std::vector<boxes_conf> xywh2xyxy(std::vector<std::vector<float>>& src, float conf_thres = 0.f)
        {
            int index = 0;
            std::vector<boxes_conf> res;
            for (auto it : src)
            {

                float top_x = it[0] - it[2] / 2;
                float top_y = it[1] - it[3] / 2;
                float bot_x = it[0] + it[2] / 2;
                float bot_y = it[1] + it[3] / 2;
                float conf = it[4];
                int maxPosition = std::max_element(it.begin() + 5, it.end()) - it.begin();
                if (it[maxPosition] * conf > conf_thres)
                {
                    boxes_conf temp{};
                    temp.top_x = top_x;
                    temp.top_y = top_y;
                    temp.bot_x = bot_x;
                    temp.bot_y = bot_y;
                    temp.conf = it[maxPosition] * conf;
                    temp.category = maxPosition - 5;
                    res.push_back(temp);

                }
                index++;
            }
            return res;
        }

        static inline float iou(Bbox box1, Bbox box2)
        {
            int x1 = std::max(box1.x, box2.x);
            int y1 = std::max(box1.y, box2.y);
            int x2 = std::min(box1.x + box1.w, box2.x + box2.w);
            int y2 = std::min(box1.y + box1.h, box2.y + box2.h);
            int w = std::max(0, x2 - x1);
            int h = std::max(0, y2 - y1);
            float over_area = w * h;
            return over_area / (box1.w * box1.h + box2.w * box2.h - over_area);
        }

        static inline bool sort_score(Bbox box1, Bbox box2) {
            return box1.score > box2.score ? true : false;
        }

        static inline std::vector<Bbox> nms(std::vector<Bbox>& boxes, float threshold)
        {
            std::vector<Bbox>resluts;
            std::sort(boxes.begin(), boxes.end(), sort_score);
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

        static inline std::vector<Bbox> computeNmsInput(std::vector<boxes_conf>& src, int max_wh, float ratio, int pad_h, int pad_w)
        {
            std::vector<Bbox> boxes;
            std::vector<float> scores;
            std::vector<int> category;
            for (auto const& it : src)
            {

                // int c = max_wh * it.conf;
                Bbox temp;
                temp.x = static_cast<double>(it.top_x - pad_w) * ratio;
                temp.y = static_cast<double>(it.top_y - pad_h) * ratio;
                temp.w = static_cast<double>(it.bot_x - it.top_x) * ratio;
                temp.h = static_cast<double>(it.bot_y - it.top_y) * ratio;
                temp.score = it.conf;
                temp.category = it.category;
                // std::cout<<it.top_x<<" "<< temp.x<<std::endl;
                //if (temp.category == 0 || temp.category == 4)
                //{
                boxes.push_back(temp);
                //}
            }
            return boxes;
        }

        /**
         * @fun non_max_suppression
         * @param prediction, conf_thres, iou_thres
         * @return std::vector(boxes, classes)
         * @details Non-Maximum Suppression (NMS) on inference results
         */
        static inline std::vector<location_char> non_max_suppression(std::vector<std::vector<float>>& prediction, float conf_thres, float iou_thres, float ratio, int pad_h, int pad_w)
        {
            // std::cout<<"nms inpu size "<<prediction.size()<<std::endl;
            //std::cout<<ratio<<std::endl;
            auto compute_box = xywh2xyxy(prediction, conf_thres);

            // Batched NMS
            int max_wh = 4096;
            std::vector<Bbox> boxes;
            std::vector<float> scores;
            std::vector<int> classes;

            boxes = computeNmsInput(compute_box, max_wh, ratio, pad_h, pad_w);//此处做分类处理，因为有四类 而非以前的单类

            // std::cout<<"ori_size:"<<boxes.size()<<std::endl;
            std::vector<Bbox> class_work;
            std::vector<Bbox> class_other;
            for (auto& box : boxes)
            {
                class_work.emplace_back(box);
            }
            auto bboxes_work = nms(class_work, iou_thres);
            std::vector<location_char> output;

            for (auto it : bboxes_work)
            {
                location_char temp;
                temp.x1 = it.x;
                temp.x2 = it.x + it.w;
                temp.y1 = it.y;
                temp.y2 = it.y + it.h;
                temp.category = it.category;
                temp.confidence = it.score;
                output.emplace_back(temp);
            }
            return output;
        }
    }

    yolov7_net::yolov7_net(std::string_view models_directory, int model_type, float nms_threshold, int device)
        : facedetector_base{ models_directory, model_type, nms_threshold, device },
        model_type_{ model_type },
        nms_threshold_{ nms_threshold }
    {
        //Excalibur needs to distinguish between float and int8 models, rknn and rknn2 does not
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        switch (model_type)
        {
        case 0:
            yolov7_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params(std::string("yolov7")), std::string(models_directory) + "/yolov7_320.rknn", device);
            break;
        case 1:
            yolov7_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params(std::string("yolov7")), std::string(models_directory) + "/yolov7_640.rknn", device);
            break;
        default:
            throw exposing::abi_invalid_argument("Invalid model_type param!");
            break;
        }
#else
        yolov7_ = std::make_unique<excalibur::pipeline<float>>(get_model_params(std::string("yolov7")), std::string(models_directory) + "/yolov7.racy", device);
#endif
    }

    yolov7_net::~yolov7_net()
    {
    }

    exposing::param_vector<longinus::face_info> yolov7_net::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int min_size, float threshold, int order, bool do_attributing)
    {
        int img_size = 320;

        if (model_type_ == 1)
        {
            img_size = 640;
        }

        if (bitmap.empty())
        {
            throw exposing::abi_invalid_argument("current frame is empty");
        }

        if (order != 1)
            throw exposing::abi_invalid_argument("Not supported order");

        CHECK_EQ(channels, 3);
        CHECK_EQ(bitmap.size(), channels * height * width);

#if !defined(USE_RKNNAPI) || !defined(USE_RKNNAPI2)
        std::shared_ptr<memory::tensor<std::uint8_t>> cache_temp;
        init_cache(bitmap, channels, height, width, order, cache_temp);
#endif
        cv::Mat cache_temp_mat(height, width, CV_8UC3, bitmap.data());

        cv::Mat blob;
        int pad_h = 0;
        int pad_w = 0;
        float ratio = 0;
        auto new_shape = cv::Size(img_size, img_size);
        std::tie(blob, ratio) = preprocess_detection(cache_temp_mat, pad_h, pad_w, new_shape);


        std::vector<std::string>  temp_out_names = { "481","495","509" };
        std::vector<std::shared_ptr<memory::tensor<float>>> model_result;

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        auto  network_result = yolov7_->forward(blob.data, { 1, blob.rows, blob.cols,blob.channels() }, RKNN_TENSOR_NHWC);
#else
        auto input_tensor_u8 = std::make_shared<memory::tensor<std::uint8_t>>(blob.channels(), blob.rows, blob.cols, -1, memory::NHWC);
        std::copy(blob.data, blob.data + blob.channels() * blob.rows * blob.cols, input_tensor_u8->mutable_cpu_data());
        auto  network_result = yolov7_->forward(input_tensor_u8 | memory::tensor_convert_to<float>);
#endif

        for (size_t i = 0; i < temp_out_names.size(); i++)
        {
            model_result.push_back(network_result[temp_out_names[i]]);
        }

        std::shared_ptr<glasssix::memory::tensor<float>> tensor_stride8(new memory::tensor<float>(std::vector<int>{3, img_size / 8, img_size / 8, 6}, -1, memory::NCHW));
        std::shared_ptr<glasssix::memory::tensor<float>> tensor_stride16(new memory::tensor<float>(std::vector<int>{3, img_size / 16, img_size / 16, 6}, -1, memory::NCHW));
        std::shared_ptr<glasssix::memory::tensor<float>> tensor_stride32(new memory::tensor<float>(std::vector<int>{3, img_size / 32, img_size / 32, 6}, -1, memory::NCHW));

        std::vector<std::string>  real_out_names = { "output","508","522" };
        std::vector<float*> transpose_result;
        transpose_result.push_back(tensor_stride8->mutable_cpu_data());
        transpose_result.push_back(tensor_stride16->mutable_cpu_data());
        transpose_result.push_back(tensor_stride32->mutable_cpu_data());
        for (size_t i = 0; i < real_out_names.size(); i++)//get output for concat operation
        {
            transpose(model_result[i]->cpu_data(), transpose_result[i], model_result[i]->count());
        }

        float conf_threshold = threshold;
        float iou_threshold = 0.45f;

        auto result = concat(transpose_result, conf_threshold);

        auto nms_result = non_max_suppression(result, conf_threshold, iou_threshold, 1 / ratio, pad_h, pad_w);

        std::vector<face_info_internal> face_infos;

        for (auto it : nms_result)
        {
            face_info_internal temp_face_info;
            temp_face_info.rect.x = it.x1;
            temp_face_info.rect.y = it.y1;
            temp_face_info.rect.w = it.x2 - it.x1;
            temp_face_info.rect.h = it.y2 - it.y1;
            temp_face_info.score = it.confidence;
            face_infos.push_back(temp_face_info);
        }

        std::vector<face_info_internal> temp_vec;
        for (auto& face : face_infos)
        {
            refine(face, height, width, true);

            if (do_attributing)
            {
                if (face.rect.h * face.rect.w <= 0)
                    throw exposing::abi_invalid_argument("face.rect.h * face.rect.w <= 0");

                face.headpose[0] = face.headpose[1] = face.headpose[2] = std::numeric_limits<float>::min();
                face.clarity = std::numeric_limits<float>::min();
                face.is_alive = false;
                face.has_mask = std::numeric_limits<float>::min();

#if defined(USE_RKNNAPI) || defined(USE_RKNNAPI2)
                cv::Rect rect(face.rect.x, face.rect.y, face.rect.w, face.rect.h);
                cv::Mat faceROI_in_frame_mat;
                mat_safty_cut(cache_temp_mat, faceROI_in_frame_mat, rect);
                tracking_landmark(faceROI_in_frame_mat, face, rect.x, rect.y);
#else
                excalibur::rectangle<float> rect(face.rect.x, face.rect.y, face.rect.h, face.rect.w);
                std::shared_ptr<memory::tensor<std::uint8_t>> faceROI_in_frame;
                excalibur::safty_cut_cpu(cache_temp, faceROI_in_frame, &rect);
                tracking_landmark(faceROI_in_frame, face, rect.x, rect.y);
#endif
                //float score = face.score;
                //face.score = score;
                refine(face, height, width, true);
            }

            cv::Point2f center_eye((face.pts.x[0] + face.pts.x[1]) / 2, (face.pts.y[0] + face.pts.y[1] / 2));
            cv::Point2f center_mouth((face.pts.x[3] + face.pts.x[4]) / 2, (face.pts.y[3] + face.pts.y[4]) / 2);
            double distance = std::sqrt((center_eye.x - center_mouth.x) * (center_eye.x - center_mouth.x) + (center_eye.y - center_mouth.y) * (center_eye.y - center_mouth.y));

            if (face.score > threshold && distance > std::numeric_limits<double>::epsilon())
            {
                temp_vec.push_back(face);
            }
        }

        std::sort(temp_vec.begin(), temp_vec.end(), [](const face_info_internal& a, const face_info_internal& b)
            { return a.rect.h * a.rect.w > b.rect.h * b.rect.w; });

        if (temp_vec.size() > 0)
        {
#if defined(USE_RKNNAPI) || defined(USE_RKNNAPI2)
            cache0_ = cache1_;
            cache1_ = cache_temp_mat.clone();
#else
            cache0_.swap(cache1_);
            cache1_.swap(cache_temp);
#endif
        }

        auto faces = exposing::make_param_vector<longinus::face_info>();
        for (auto& i : temp_vec)
            faces.push_back(exposing::make_as_first<face_info_impl>(i));

        return faces;
    }

    std::string yolov7_net::version() const
    {
        return "1.0.0";
    }
}