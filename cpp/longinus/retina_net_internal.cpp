#include <vector>
#include <functional>
#include <map>
#include <cfloat>
#include <cmath>
#include "retina_net_internal.hpp"
#include "face_info_impl.hpp"
#include "Excalibur/pipeline.hpp"
#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_safty_cut.hpp"
#include "Excalibur/operation_resize.hpp"
#include "Excalibur/operation_rgb2gray.hpp"
#include "Excalibur/operation_rotate.hpp"
#include "Primitives/tensor_conversions.hpp"
#include "hardcode.hpp"

#ifdef USE_RKNNAPI
#include "RKNNWrapper/rknn_wrapper.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

#endif

#if defined(BUILD_RV1106) 
    #include <fstream>
#endif
namespace
{
    static float estimate_head_pose_weights[] =
        {
            -88.16000008, 19.16736698,
            15.29246944, 133.74215091,
            70.45322778, -0.26062090,
            -23.13496952, -80.01102625,
            67.55717493, 39.87895452,
            -34.70160224, -69.74298174,
            -103.38437793, -67.36540879,
            19.02850753, 201.29906886,
            215.69865520, -18.39539477,
            -10.33704663, -334.39622374,
            -30.19078293, -7.23233403,
            22.79330967, 70.73228422,
            -29.22699468, 23.91464714,
            -0.30067024, -0.01195406,
            -48.752375, 79.479039105};
}

namespace glasssix::longinus
{
    class retina_net_internal::impl
    {
    public:
        impl(const exposing::param_string racy_path, const exposing::param_string tracker_racy_path, float nms_threshold = 0.4, int device = -1) : impl{get_model_params("longinus", false), racy_path, get_model_params("pfld_land71_simp", false), tracker_racy_path, nms_threshold, device}
        {
        }

        impl(const std::vector<std::string> &phai, const exposing::param_string racy_path, const std::vector<std::string> &tracker_phai, const exposing::param_string tracker_racy_path, float nms_threshold = 0.4, int device = -1)
            : retina_{phai, exposing::to_narrow_string(racy_path), device}, tracker_{tracker_phai, exposing::to_narrow_string(tracker_racy_path), -1}, nms_threshold_(nms_threshold), device_(device)
        {
            ratio_ = {1.0};
            //anchor setting
            feat_stride_fpn_ = {32, 16, 8};
            anchor_cfg tmp;
            tmp.SCALES = {32, 16};
            tmp.BASE_SIZE = 16;
            tmp.RATIOS = ratio_;
            tmp.ALLOWED_BORDER = 9999;
            tmp.STRIDE = 32;
            cfg_.push_back(tmp);

            tmp.SCALES = {8, 4};
            tmp.BASE_SIZE = 16;
            tmp.RATIOS = ratio_;
            tmp.ALLOWED_BORDER = 9999;
            tmp.STRIDE = 16;
            cfg_.push_back(tmp);

            tmp.SCALES = {2, 1};
            tmp.BASE_SIZE = 16;
            tmp.RATIOS = ratio_;
            tmp.ALLOWED_BORDER = 9999;
            tmp.STRIDE = 8;
            cfg_.push_back(tmp);

            bool dense_anchor = false;
            std::vector<std::vector<anchor_box>> anchors_fpn = generate_anchors_fpn(dense_anchor, cfg_);
            for (size_t i = 0; i < anchors_fpn.size(); i++)
            {
                std::string key = "stride" + std::to_string(feat_stride_fpn_[i]);
                anchors_fpn_[key] = anchors_fpn[i];
                num_anchors_[key] = anchors_fpn[i].size();
            }

#if defined(BUILD_RV1106) 
        matmul_weight.resize(208*14);

        std::string filename;

        std::string track_path( tracker_racy_path);
        std::cout<<tracker_racy_path<<" tracker_racy_path: "<<track_path<<std::endl;
        std::size_t found = track_path.find_last_of("/");
        std::cout<<found<<std::endl;

        if(std::string::npos==found)
        {
                //need implement
        }
        else
        {
                std::string supplement_path(track_path,0,found );
                filename=supplement_path+ R"(/land71_supplement.dat)";
                // std::cout<<supplement_path<<std::endl;
        }

        std::ifstream fin;

        fin.open(filename, std::ios::in | std::ios::binary);
        if (fin.is_open() == false)
        {
             throw exposing::abi_invalid_argument("rv1106 supplement weight dat not find");
        }
        while (fin.read((char*)matmul_weight.data(), 208*14*sizeof(float)))
        { }      
        fin.close(); 
#endif


        }

        ~impl()
        {
        }

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        exposing::param_vector<face_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int min_size, float threshold, int order, bool do_attributing)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }

            if (order != 1)
                throw exposing::abi_invalid_argument("Not supported order");

            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            //#if 0
            cv::Mat cache_temp(height, width, CV_8UC3, bitmap.data());
            const int fix_size = 320;
            int max_edge = std::max(width, height);
            float scale = max_edge * 1.0f / fix_size;

            int ws = fix_size;
            int hs = fix_size;
            cv::Mat cache_forward;
            cv::resize(cache_temp, cache_forward, cv::Size(std::round(width / scale), std::round(height / scale)));
            cv::copyMakeBorder(cache_forward, cache_forward, 0, hs - std::round(height / scale), 0, ws - std::round(width / scale), cv::BORDER_CONSTANT, cv::Scalar::all(0));

#ifdef USE_RKNNAPI
            const char* score_suffix[3] = { "_74_125","_98_128","_122_131" };
            const char* bbox_suffix[3] = { "_75_126","_99_129","_123_132" };
            const char* landmark_suffix[3] = { "_76_127","_100_130","_124_133" };
#else
            const char* score_suffix[3] = { "","","" };
            const char* bbox_suffix[3] = { "","","" };
            const char* landmark_suffix[3] = { "","","" };
#endif
            auto blob_data = retina_.forward(cache_forward.data, { 1, hs, ws, 3 }, RKNN_TENSOR_NHWC);

            std::string name_bbox = "face_rpn_bbox_pred_";
            std::string name_score = "face_rpn_cls_prob_reshape_";
            std::string name_landmark = "face_rpn_landmark_pred_";

            std::vector<face_info_internal> face_infos;
            for (size_t i = 0; i < feat_stride_fpn_.size(); i++)
            {
                std::string key = "stride" + std::to_string(feat_stride_fpn_[i]);
                int stride = feat_stride_fpn_[i];

                std::string str = name_score + key + score_suffix[i];
                auto score_blob = blob_data[str];
                auto score_blob_count = score_blob->count();
                const float* scoreB = score_blob->cpu_data() + score_blob_count / 2;
                const float* scoreE = scoreB + score_blob_count / 2;
                std::vector<float> score = std::vector<float>(scoreB, scoreE);

                str = name_bbox + key + bbox_suffix[i];
                auto bbox_blob = blob_data[str];
                auto bbox_blob_count = bbox_blob->count();
                const float* bboxB = bbox_blob->cpu_data();
                const float* bboxE = bboxB + bbox_blob_count;
                std::vector<float> bbox_delta = std::vector<float>(bboxB, bboxE);

                str = name_landmark + key + landmark_suffix[i];
                auto landmark_blob = blob_data[str];
                auto landmark_blob_count = landmark_blob->count();
                const float* landmarkB = landmark_blob->cpu_data();
                const float* landmarkE = landmarkB + landmark_blob_count;
                std::vector<float> landmark_delta = std::vector<float>(landmarkB, landmarkE);

                int score_width = score_blob->width();
                int score_height = score_blob->height();
                size_t count = score_width * score_height;
                size_t num_anchor = num_anchors_[key];

                //store order: h * w * num_anchor
                std::vector<anchor_box> anchors = anchors_plane(score_height, score_width, stride, anchors_fpn_[key]);

                for (size_t num = 0; num < num_anchor; num++)
                {
                    for (size_t j = 0; j < count; j++)
                    {
                        float conf = score[j + count * num];
                        if (conf <= threshold)
                        {
                            continue;
                        }

                        float dx = bbox_delta[j + count * (0 + num * 4)];
                        float dy = bbox_delta[j + count * (1 + num * 4)];
                        float dw = bbox_delta[j + count * (2 + num * 4)];
                        float dh = bbox_delta[j + count * (3 + num * 4)];
                        auto regress = std::vector<float>{ dx, dy, dw, dh };

                        // regression face bbox
                        anchor_box rect = bbox_pred(anchors[j + count * num], regress);
                        //Out of bounds
                        clip_box(rect, ws, hs);

                        face_pts pts;
                        for (size_t k = 0; k < 5; k++)
                        {
                            pts.x[k] = landmark_delta[j + count * (num * 10 + k * 2)];
                            pts.y[k] = landmark_delta[j + count * (num * 10 + k * 2 + 1)];
                        }
                        //regression facial landmark
                        face_pts landmarks = landmark_pred(anchors[j + count * num], pts);

                        face_info_internal tmp;
                        tmp.score = conf;
                        tmp.rect = rect;
                        tmp.pts = landmarks;
                        face_infos.push_back(tmp);
                    }
                }
            }

            face_infos = nms(face_infos, nms_threshold_);

            std::vector<face_info_internal> temp_vec;
            for (auto& face : face_infos)
            {
                if (scale != 1.0f)
                {
                    face.rect.x *= scale;
                    face.rect.y *= scale;
                    face.rect.h *= scale;
                    face.rect.w *= scale;
                    for (size_t i = 0; i < std::size(face.pts.x); i++)
                    {
                        face.pts.x[i] *= scale;
                        face.pts.y[i] *= scale;
                    }
                }

                refine(face, height, width, true);

                if (do_attributing)
                {

                    if (face.rect.h * face.rect.w <= 0)
                        throw exposing::abi_invalid_argument("face.rect.h * face.rect.w <= 0");

                    cv::Rect rect(face.rect.x, face.rect.y, face.rect.w, face.rect.h);
                    cv::Mat faceROI_in_frame;
                    safty_cut(cache_temp, faceROI_in_frame, rect);
                    face.headpose[0] = face.headpose[1] = face.headpose[2] = std::numeric_limits<float>::min();
                    face.clarity = std::numeric_limits<float>::min();
                    face.is_alive = false;
                    face.has_mask = std::numeric_limits<float>::min();

                    //float score = face.score;
                    tracking_landmark(faceROI_in_frame, face, rect.x, rect.y);
                    //face.score = score;
                    refine(face, height, width, true);
                }

                excalibur::point<float> center_eye = excalibur::point<float>((face.pts.x[0] + face.pts.x[1]) / 2, (face.pts.y[0] + face.pts.y[1] / 2));
                excalibur::point<float> center_mouth = excalibur::point<float>((face.pts.x[3] + face.pts.x[4]) / 2, (face.pts.y[3] + face.pts.y[4]) / 2);
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
                cache0_ = cache1_;
                cache1_ = cache_temp.clone();
            }

            auto faces = exposing::make_param_vector<face_info>();
            for (auto& i : temp_vec)
                faces.push_back(exposing::make_as_first<face_info_impl>(i));

            return faces;
        }
#else
        exposing::param_vector<face_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int min_size, float threshold, int order, bool do_attributing)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }

            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            std::shared_ptr<memory::tensor<std::uint8_t>> cache_temp;
            init_cache(bitmap, channels, height, width, order, cache_temp);

            if (min_size < 16)
                min_size = 16;

            float scale = min_size / 16.0f;
            int ws = (int(width / scale) + 31) / 32 * 32;
            int hs = (int(height / scale) + 31) / 32 * 32;

            std::shared_ptr<memory::tensor<std::uint8_t>> cache_forward;
            excalibur::resize_cpu(cache_temp, cache_forward, int(height / scale), int(width / scale));
            excalibur::make_border(cache_forward, cache_forward, 0, hs - int(height / scale), 0, ws - int(width / scale));

            auto blob_data = retina_.forward(cache_forward | memory::tensor_convert_to<float>);

            std::string name_bbox = "face_rpn_bbox_pred_";
            std::string name_score = "face_rpn_cls_prob_reshape_";
            std::string name_landmark = "face_rpn_landmark_pred_";

            std::vector<face_info_internal> face_infos;
            for (size_t i = 0; i < feat_stride_fpn_.size(); i++)
            {
                std::string key = "stride" + std::to_string(feat_stride_fpn_[i]);
                int stride = feat_stride_fpn_[i];

                std::string str = name_score + key;
                auto score_blob = blob_data[str];
                auto score_blob_count = score_blob->count();
                const float *scoreB = score_blob->cpu_data() + score_blob_count / 2;
                const float *scoreE = scoreB + score_blob_count / 2;
                std::vector<float> score = std::vector<float>(scoreB, scoreE);

                str = name_bbox + key;
                auto bbox_blob = blob_data[str];
                auto bbox_blob_count = bbox_blob->count();
                const float *bboxB = bbox_blob->cpu_data();
                const float *bboxE = bboxB + bbox_blob_count;
                std::vector<float> bbox_delta = std::vector<float>(bboxB, bboxE);

                str = name_landmark + key;
                auto landmark_blob = blob_data[str];
                auto landmark_blob_count = landmark_blob->count();
                const float *landmarkB = landmark_blob->cpu_data();
                const float *landmarkE = landmarkB + landmark_blob_count;
                std::vector<float> landmark_delta = std::vector<float>(landmarkB, landmarkE);

                int score_width = score_blob->width();
                int score_height = score_blob->height();
                size_t count = score_width * score_height;
                size_t num_anchor = num_anchors_[key];

                //store order: h * w * num_anchor
                std::vector<anchor_box> anchors = anchors_plane(score_height, score_width, stride, anchors_fpn_[key]);

                for (size_t num = 0; num < num_anchor; num++)
                {
                    for (size_t j = 0; j < count; j++)
                    {
                        float conf = score[j + count * num];
                        if (conf <= threshold)
                        {
                            continue;
                        }

                        float dx = bbox_delta[j + count * (0 + num * 4)];
                        float dy = bbox_delta[j + count * (1 + num * 4)];
                        float dw = bbox_delta[j + count * (2 + num * 4)];
                        float dh = bbox_delta[j + count * (3 + num * 4)];
                        auto regress = std::vector<float>{dx, dy, dw, dh};

                        // regression face bbox
                        anchor_box rect = bbox_pred(anchors[j + count * num], regress);
                        //Out of bounds
                        clip_box(rect, ws, hs);

                        face_pts pts;
                        for (size_t k = 0; k < 5; k++)
                        {
                            pts.x[k] = landmark_delta[j + count * (num * 10 + k * 2)];
                            pts.y[k] = landmark_delta[j + count * (num * 10 + k * 2 + 1)];
                        }
                        //regression facial landmark
                        face_pts landmarks = landmark_pred(anchors[j + count * num], pts);

                        face_info_internal tmp;
                        tmp.score = conf;
                        tmp.rect = rect;
                        tmp.pts = landmarks;
                        face_infos.push_back(tmp);
                    }
                }
            }

            face_infos = nms(face_infos, nms_threshold_);

            std::vector<face_info_internal> temp_vec;
            for (auto &face : face_infos)
            {
                if (scale != 1.0f)
                {
                    face.rect.x *= scale;
                    face.rect.y *= scale;
                    face.rect.h *= scale;
                    face.rect.w *= scale;
                    for (size_t i = 0; i < std::size(face.pts.x); i++)
                    {
                        face.pts.x[i] *= scale;
                        face.pts.y[i] *= scale;
                    }
                }

                refine(face, height, width, true);

                if (do_attributing)
                {

                    if (face.rect.h * face.rect.w <= 0)
                        throw exposing::abi_invalid_argument("face.rect.h * face.rect.w <= 0");

                    excalibur::rectangle<float> rect(face.rect.x, face.rect.y, face.rect.h, face.rect.w);
                    std::shared_ptr<memory::tensor<std::uint8_t>> faceROI_in_frame;
                    excalibur::safty_cut_cpu(cache_temp, faceROI_in_frame, &rect);

                    face.headpose[0] = face.headpose[1] = face.headpose[2] = std::numeric_limits<float>::min();
                    face.clarity = std::numeric_limits<float>::min();
                    face.is_alive = false;
                    face.has_mask = std::numeric_limits<float>::min();

                    //float score = face.score;
                    tracking_landmark(faceROI_in_frame, face, rect.x, rect.y);
                    //face.score = score;
                    refine(face, height, width, true);
                }

                excalibur::point<float> center_eye = excalibur::point<float>((face.pts.x[0] + face.pts.x[1]) / 2, (face.pts.y[0] + face.pts.y[1] / 2));
                excalibur::point<float> center_mouth = excalibur::point<float>((face.pts.x[3] + face.pts.x[4]) / 2, (face.pts.y[3] + face.pts.y[4]) / 2);
                double distance = std::sqrt((center_eye.x - center_mouth.x) * (center_eye.x - center_mouth.x) + (center_eye.y - center_mouth.y) * (center_eye.y - center_mouth.y));

                if (face.score > threshold && distance > std::numeric_limits<double>::epsilon())
                {
                    temp_vec.push_back(face);
                }
            }

            std::sort(temp_vec.begin(), temp_vec.end(), [](const face_info_internal &a, const face_info_internal &b)
                      { return a.rect.h * a.rect.w > b.rect.h * b.rect.w; });

            if (temp_vec.size() > 0)
            {
                cache0_.swap(cache1_);
                cache1_.swap(cache_temp);
            }

            auto faces = exposing::make_param_vector<face_info>();
            for (auto &i : temp_vec)
                faces.push_back(exposing::make_as_first<face_info_impl>(i));

            return faces;
        }
#endif

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        face_info single_trace(face_info& face, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
        {
            if (bitmap.empty())
                throw exposing::abi_invalid_argument("current frame is empty");

            if (order != 1)
                throw exposing::abi_invalid_argument("Not supported order");

            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            if (cache1_.empty())
                throw exposing::abi_invalid_argument("previous frame cache is empty");

            cv::Rect2f track_box(face.x(), face.y(), face.width(), face.height());
            if (track_box.height * track_box.width <= 0)
                throw exposing::abi_invalid_argument("track_box.h * track_box.w <= 0");

            cv::Mat face_in_prev_frame;
            safty_cut(cache1_, face_in_prev_frame, track_box);

            cv::Mat cache_temp(height, width, CV_8UC3, bitmap.data());

            int min_edge = std::min(track_box.height, track_box.width);
            float scale = min_edge / 20.0f;
            if (scale < 1.0)
                scale = 1.0;

            tracking_corrfilter(cache_temp, face_in_prev_frame, track_box, scale);
            cv::Mat faceROI_in_frame;
            safty_cut(cache_temp, faceROI_in_frame, track_box);
            face_info_internal face_internal;
            face_internal.headpose[0] = face_internal.headpose[1] = face_internal.headpose[2] = std::numeric_limits<float>::min();
            face_internal.clarity = std::numeric_limits<float>::min();
            face_internal.is_alive = false;
            face_internal.has_mask = std::numeric_limits<float>::min();
            tracking_landmark(faceROI_in_frame, face_internal, track_box.x, track_box.y);
            refine(face_internal, height, width, true);

            excalibur::point<float> center_eye = excalibur::point<float>((face_internal.pts.x[0] + face_internal.pts.x[1]) / 2, (face_internal.pts.y[0] + face_internal.pts.y[1] / 2));
            excalibur::point<float> center_mouth = excalibur::point<float>((face_internal.pts.x[3] + face_internal.pts.x[4]) / 2, (face_internal.pts.y[3] + face_internal.pts.y[4]) / 2);
            double distance = std::sqrt((center_eye.x - center_mouth.x) * (center_eye.x - center_mouth.x) + (center_eye.y - center_mouth.y) * (center_eye.y - center_mouth.y));

            if (face_internal.score > 0.1f && distance > std::numeric_limits<double>::epsilon())
            {
                cache0_ = cache1_;
                cache1_ = cache_temp.clone();
            }

            if (distance <= std::numeric_limits<double>::epsilon())
                face_internal.score = 0.0f;

            return exposing::make_as_first<face_info_impl>(face_internal);
        }
#else
        face_info single_trace(face_info &face, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
        {
            if (bitmap.empty())
                throw exposing::abi_invalid_argument("current frame is empty");

            CHECK_EQ(channels, 3);
            CHECK_EQ(bitmap.size(), channels * height * width);

            if (cache1_->empty())
                throw exposing::abi_invalid_argument("previous frame cache is empty");

            excalibur::rectangle<float> track_box(face.x(), face.y(), face.height(), face.width());
            if (track_box.h * track_box.w <= 0)
                throw exposing::abi_invalid_argument("track_box.h * track_box.w <= 0");

            std::shared_ptr<memory::tensor<std::uint8_t>> face_in_prev_frame;
            excalibur::safty_cut_cpu(cache1_, face_in_prev_frame, &track_box);

            std::shared_ptr<memory::tensor<std::uint8_t>> cache_temp;
            init_cache(bitmap, channels, height, width, order, cache_temp);

            int min_edge = std::min(track_box.h, track_box.w);
            float scale = min_edge / 20.0f;
            if (scale < 1.0)
                scale = 1.0;

            tracking_corrfilter(cache_temp, face_in_prev_frame, track_box, scale);
            std::shared_ptr<memory::tensor<std::uint8_t>> faceROI_in_frame;
            excalibur::safty_cut_cpu(cache_temp, faceROI_in_frame, &track_box);

            face_info_internal face_internal;
            face_internal.headpose[0] = face_internal.headpose[1] = face_internal.headpose[2] = std::numeric_limits<float>::min();
            face_internal.clarity = std::numeric_limits<float>::min();
            face_internal.is_alive = false;
            face_internal.has_mask = std::numeric_limits<float>::min();
            tracking_landmark(faceROI_in_frame, face_internal, track_box.x, track_box.y);
            refine(face_internal, height, width, true);

            excalibur::point<float> center_eye = excalibur::point<float>((face_internal.pts.x[0] + face_internal.pts.x[1]) / 2, (face_internal.pts.y[0] + face_internal.pts.y[1] / 2));
            excalibur::point<float> center_mouth = excalibur::point<float>((face_internal.pts.x[3] + face_internal.pts.x[4]) / 2, (face_internal.pts.y[3] + face_internal.pts.y[4]) / 2);
            double distance = std::sqrt((center_eye.x - center_mouth.x) * (center_eye.x - center_mouth.x) + (center_eye.y - center_mouth.y) * (center_eye.y - center_mouth.y));

            if (face_internal.score > 0.1f && distance > std::numeric_limits<double>::epsilon())
            {
                cache0_.swap(cache1_);
                cache1_.swap(cache_temp);
            }

            if (distance <= std::numeric_limits<double>::epsilon())
                face_internal.score = 0.0f;

            return exposing::make_as_first<face_info_impl>(face_internal);
        }
#endif

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        exposing::param_vector<exposing::param_vector<std::uint8_t>> center_scale_align(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
            float scale, std::int32_t order)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            if(order != 1)
                throw exposing::abi_invalid_argument("Not supported order");

            cv::Mat img(height, width, CV_8UC3, bitmap.data());

            int center_x = width / 2;
            int center_y = height / 2;
            int ori_width = width * scale;
            int ori_height = height * scale;

            int ori_x = center_x - ori_width / 2;
            int ori_y = center_y - ori_height / 2;

            face_info_internal ori_face;
            ori_face.rect.x = ori_x;
            ori_face.rect.y = ori_y;
            ori_face.rect.h = ori_height;
            ori_face.rect.w = ori_width;

            refine(ori_face, height, width, true);

            cv::Rect ori_rect(ori_face.rect.x, ori_face.rect.y, ori_face.rect.w, ori_face.rect.h);
            cv::Mat ori_img;
            safty_cut(img, ori_img, ori_rect);

            ori_face.headpose[0] = ori_face.headpose[1] = ori_face.headpose[2] = std::numeric_limits<float>::min();
            ori_face.clarity = std::numeric_limits<float>::min();
            ori_face.is_alive = false;
            ori_face.has_mask = std::numeric_limits<float>::min();

            tracking_landmark(ori_img, ori_face, ori_rect.x, ori_rect.y);
            refine(ori_face, height, width, true);


            cv::Mat ROI, rotated_ROI, final_mat, final_mat_gray, resized_color_img;
            auto res = exposing::make_param_vector<std::uint8_t, 2>();

            cv::Rect MarginRect(ori_face.rect.x - ori_face.rect.w * 0.0f,
                ori_face.rect.y - ori_face.rect.h * 0.0f,
                ori_face.rect.w * 1.0f,
                ori_face.rect.h * 1.0f);

            safty_cut(img, ROI, MarginRect);
            float min_edge = std::min(MarginRect.width, MarginRect.height);
            float ROI_scale = 160.f / min_edge;
            if (ROI_scale < 1.0f)
                cv::resize(ROI, ROI, cv::Size(ROI.cols * ROI_scale, ROI.rows * ROI_scale));
            else
                ROI_scale = 1.0f;

            cv::Point2f ldmk5[5];
            for (size_t j = 0; j < 5; j++)
            {
                ldmk5[j] = cv::Point2f(ori_face.pts.x[j] - MarginRect.x, ori_face.pts.y[j] - MarginRect.y);
            }
            cv::Point2f center_eye((ldmk5[0].x + ldmk5[1].x) / 2, (ldmk5[0].y + ldmk5[1].y) / 2);
            cv::Point2f center_mouth((ldmk5[3].x + ldmk5[4].x) / 2, (ldmk5[3].y + ldmk5[4].y) / 2);
            double tan = (center_eye.x - center_mouth.x) / (center_eye.y - center_mouth.y);
            double arctan = atan(tan) * 180 / 3.1415926;

            cv::Point2f center((center_eye.x + center_mouth.x) * ROI_scale / 2, (center_eye.y + center_mouth.y) * ROI_scale / 2);
            cv::Mat rot_mat = cv::getRotationMatrix2D(center, -1 * arctan, 1.0);
            cv::warpAffine(ROI, rotated_ROI, rot_mat, ROI.size(), cv::INTER_CUBIC, cv::BORDER_CONSTANT, cv::Scalar::all(0));

            double distance = std::sqrt((center_eye.x - center_mouth.x) * (center_eye.x - center_mouth.x) + (center_eye.y - center_mouth.y) * (center_eye.y - center_mouth.y));

            if (distance < std::numeric_limits<double>::epsilon())
            {
                throw exposing::abi_invalid_argument("Illegal distance. Error landmarks.");
            }

            double cos = (center_mouth.y - center_eye.y) / distance;
            double sin = (center_mouth.x - center_eye.x) / distance;
            cv::Point2f new_center_eye(center_eye.x + (float)(sin * distance / 2), (float)(center_eye.y - (1 - cos) * distance / 2));
            cv::Point2f new_center_mouth(center_mouth.x - (float)(sin * distance / 2), (float)(center_mouth.y + (1 - cos) * distance / 2));
           cv::Rect2f final_rect((new_center_eye.x - distance * 1.25f) * ROI_scale,
                (new_center_eye.y - distance * 0.75f) * ROI_scale,
                distance * 2.5f * ROI_scale, distance * 2.5f * ROI_scale);
            safty_cut(rotated_ROI, final_mat, final_rect);

            cv::resize(final_mat, resized_color_img, cv::Size(128, 128));
            std::vector<cv::Mat> img_channel_vec;
            cv::split(resized_color_img, img_channel_vec);

            auto temp_vec = exposing::make_param_vector<std::uint8_t>();
            temp_vec.resize(static_cast<size_t>(3 * 128 * 128));
            for (size_t j = 0; j < 3; j++)
                temp_vec.copy_from({ img_channel_vec[j].data, 128 * 128 }, j * 128 * 128);

            res.push_back(temp_vec);

            return res;
        }
#else
        exposing::param_vector<exposing::param_vector<std::uint8_t>> center_scale_align(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
            float scale, std::int32_t order)
        {
            if (bitmap.empty())
            {
                throw exposing::abi_invalid_argument("current frame is empty");
            }
            auto img = std::make_shared<memory::tensor<uint8_t>>(channels, height, width, -1, static_cast<memory::orderType>(order));
            std::copy(bitmap.begin(), bitmap.end(), img->mutable_cpu_data());
            if (order == memory::NHWC)
                img->convert_order();

            int center_x = width / 2;
            int center_y = height / 2;
            int ori_width = width * scale;
            int ori_height = height * scale;

            int ori_x = center_x - ori_width / 2;
            int ori_y = center_y - ori_height / 2;

            face_info_internal ori_face;
            ori_face.rect.x = ori_x;
            ori_face.rect.y = ori_y;
            ori_face.rect.h = ori_height;
            ori_face.rect.w = ori_width;

            refine(ori_face, height, width, true);

            excalibur::rectangle<int> ori_rect = excalibur::rectangle<int>(ori_face.rect.x, ori_face.rect.y, ori_face.rect.h, ori_face.rect.w);
            std::shared_ptr<memory::tensor<uint8_t>> ori_img;
            excalibur::safty_cut_cpu(img, ori_img, &ori_rect);

            ori_face.headpose[0] = ori_face.headpose[1] = ori_face.headpose[2] = std::numeric_limits<float>::min();
            ori_face.clarity = std::numeric_limits<float>::min();
            ori_face.is_alive = false;
            ori_face.has_mask = std::numeric_limits<float>::min();

            tracking_landmark(ori_img, ori_face, ori_rect.x, ori_rect.y);
            refine(ori_face, height, width, true);


            std::shared_ptr<memory::tensor<uint8_t>> ROI, rotated_ROI, final_mat, final_mat_gray, resized_color_img;
            std::vector<std::shared_ptr<memory::tensor<uint8_t>>> src_vector;
            auto res = exposing::make_param_vector<std::uint8_t, 2>();

            excalibur::rectangle<int> MarginRect = excalibur::rectangle<int>(ori_face.rect.x - ori_face.rect.w * 0.0f,
                ori_face.rect.y - ori_face.rect.h * 0.0f,
                ori_face.rect.h * 1.0f,
                ori_face.rect.w * 1.0f);

            excalibur::safty_cut_cpu(img, ROI, &MarginRect);

            excalibur::point<float> ldmk5[5];
            for (size_t j = 0; j < 5; j++)
            {
                ldmk5[j] = excalibur::point<float>(ori_face.pts.x[j] - MarginRect.x, ori_face.pts.y[j] - MarginRect.y);
            }
            excalibur::point<float> center_eye = excalibur::point<float>((ldmk5[0].x + ldmk5[1].x) / 2, (ldmk5[0].y + ldmk5[1].y) / 2);
            excalibur::point<float> center_mouth = excalibur::point<float>((ldmk5[3].x + ldmk5[4].x) / 2, (ldmk5[3].y + ldmk5[4].y) / 2);
            excalibur::point<float> center = excalibur::point<float>((center_eye.x + center_mouth.x) / 2, (center_eye.y + center_mouth.y) / 2);
            double tan = (center_eye.x - center_mouth.x) / (center_eye.y - center_mouth.y);
            double arctan = atan(tan) * 180 / 3.1415926;

            excalibur::rotate_with_points_cpu(ROI, rotated_ROI, center, -1 * arctan);

            double distance = std::sqrt((center_eye.x - center_mouth.x) * (center_eye.x - center_mouth.x) + (center_eye.y - center_mouth.y) * (center_eye.y - center_mouth.y));

            if (distance < std::numeric_limits<double>::epsilon())
            {
                throw exposing::abi_invalid_argument("Illegal distance. Error landmarks.");
            }

            double cos = (center_mouth.y - center_eye.y) / distance;
            double sin = (center_mouth.x - center_eye.x) / distance;
            excalibur::point<float> new_center_eye = excalibur::point<float>(center_eye.x + (float)(sin * distance / 2), (float)(center_eye.y - (1 - cos) * distance / 2));
            excalibur::point<float> new_center_mouth = excalibur::point<float>(center_mouth.x - (float)(sin * distance / 2), (float)(center_mouth.y + (1 - cos) * distance / 2));
            excalibur::rectangle<float> final_rect = excalibur::rectangle<float>(new_center_eye.x - distance * 1.25f,
                new_center_eye.y - distance * 0.75f,
                distance * 2.5f, distance * 2.5f);
            excalibur::safty_cut_cpu(rotated_ROI, final_mat, &final_rect);

            excalibur::resize_cpu(final_mat, resized_color_img, 128, 128);

            auto temp_vec = exposing::make_param_vector<std::uint8_t>();
            temp_vec.resize(static_cast<size_t>(resized_color_img->count()));
            temp_vec.copy_from({ resized_color_img->cpu_data(), static_cast<size_t>(resized_color_img->count()) }, 0);

            res.push_back(temp_vec);

            return res;
        }
#endif

        static std::string version()
        {
            return "1.0.0";
        }

    private:
        void init_cache(exposing::param_span<std::uint8_t> &bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order, std::shared_ptr<memory::tensor<std::uint8_t>> &cache)
        {
            if (cache == nullptr || cache->channels() != channels || cache->height() != height || cache->width() != width || cache->order() != order)
            {
                std::vector<int> shape;
                if (order == memory::NCHW)
                    shape = {static_cast<int>(1), channels, height, width};
                else if (order == memory::NHWC)
                    shape = {static_cast<int>(1), height, width, channels};
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

            if (order == memory::NHWC)
                cache->convert_order();
        }

        //processing
        inline anchor_win whctrs(anchor_box anchor)
        {
            //Return width, height, x center, and y center for an anchor (window).
            anchor_win win;
            win.w = anchor.w;
            win.h = anchor.h;
            win.x_ctr = anchor.x + 0.5 * (win.w - 1);
            win.y_ctr = anchor.y + 0.5 * (win.h - 1);

            return win;
        }

        inline anchor_box make_anchors(anchor_win win)
        {
            //Given a vector of widths (ws) and heights (hs) around a center
            //(x_ctr, y_ctr), output a set of anchors (windows).
            anchor_box anchor;
            anchor.x = win.x_ctr - 0.5 * (win.w - 1);
            anchor.y = win.y_ctr - 0.5 * (win.h - 1);
            anchor.w = win.w;
            anchor.h = win.h;

            return anchor;
        }

        inline std::vector<anchor_box> ratio_enum(anchor_box anchor, std::vector<float> ratios)
        {
            //Enumerate a set of anchors for each aspect ratio wrt an anchor.
            std::vector<anchor_box> anchors;
            for (size_t i = 0; i < ratios.size(); i++)
            {
                anchor_win win = whctrs(anchor);
                float size = win.w * win.h;
                float scale = size / ratios[i];

                win.w = std::round(sqrt(scale));
                win.h = std::round(win.w * ratios[i]);

                anchor_box tmp = make_anchors(win);
                anchors.push_back(tmp);
            }

            return anchors;
        }

        inline std::vector<anchor_box> scale_enum(anchor_box anchor, std::vector<int> scales)
        {
            //Enumerate a set of anchors for each scale wrt an anchor.
            std::vector<anchor_box> anchors;
            for (size_t i = 0; i < scales.size(); i++)
            {
                anchor_win win = whctrs(anchor);

                win.w = win.w * scales[i];
                win.h = win.h * scales[i];

                anchor_box tmp = make_anchors(win);
                anchors.push_back(tmp);
            }

            return anchors;
        }

        inline std::vector<anchor_box> generate_anchors(int base_size = 16, std::vector<float> ratios = {0.5, 1, 2},
                                                        std::vector<int> scales = {8, 64}, int stride = 16, bool dense_anchor = false)
        {
            //Generate anchor (reference) windows by enumerating aspect ratios X
            //scales wrt a reference (0, 0, 15, 15) window.

            anchor_box base_anchor;
            base_anchor.x = 0;
            base_anchor.y = 0;
            base_anchor.h = base_size;
            base_anchor.w = base_size;

            std::vector<anchor_box> ratio_anchors;
            ratio_anchors = ratio_enum(base_anchor, ratios);

            std::vector<anchor_box> anchors;
            for (size_t i = 0; i < ratio_anchors.size(); i++)
            {
                std::vector<anchor_box> tmp = scale_enum(ratio_anchors[i], scales);
                anchors.insert(anchors.end(), tmp.begin(), tmp.end());
            }

            if (dense_anchor)
            {
                assert(stride % 2 == 0);
                std::vector<anchor_box> anchors2 = anchors;
                for (size_t i = 0; i < anchors2.size(); i++)
                {
                    anchors2[i].x += stride / 2;
                    anchors2[i].y += stride / 2;
                }
                anchors.insert(anchors.end(), anchors2.begin(), anchors2.end());
            }

            return anchors;
        }

        inline std::vector<std::vector<anchor_box>> generate_anchors_fpn(bool dense_anchor = false, std::vector<anchor_cfg> cfg = {})
        {
            //Generate anchor (reference) windows by enumerating aspect ratios X
            //scales wrt a reference (0, 0, 15, 15) window.

            std::vector<std::vector<anchor_box>> anchors;
            for (size_t i = 0; i < cfg.size(); i++)
            {
                //stride [32 16 8]
                anchor_cfg tmp = cfg[i];
                int bs = tmp.BASE_SIZE;
                std::vector<float> ratios = tmp.RATIOS;
                std::vector<int> scales = tmp.SCALES;
                int stride = tmp.STRIDE;

                std::vector<anchor_box> r = generate_anchors(bs, ratios, scales, stride, dense_anchor);
                anchors.push_back(r);
            }

            return anchors;
        }

        inline std::vector<anchor_box> anchors_plane(int height, int width, int stride, std::vector<anchor_box> base_anchors)
        {
            /*
			height: height of plane
			width:  width of plane
			stride: stride ot the original image
			anchors_base: a base set of anchors
			*/

            std::vector<anchor_box> all_anchors;
            for (size_t k = 0; k < base_anchors.size(); k++)
            {
                for (int ih = 0; ih < height; ih++)
                {
                    int sh = ih * stride;
                    for (int iw = 0; iw < width; iw++)
                    {
                        int sw = iw * stride;

                        anchor_box tmp;
                        tmp.x = base_anchors[k].x + sw;
                        tmp.y = base_anchors[k].y + sh;
                        tmp.h = base_anchors[k].h;
                        tmp.w = base_anchors[k].w;
                        all_anchors.push_back(tmp);
                    }
                }
            }

            return all_anchors;
        }

        inline void clip_boxes(std::vector<anchor_box> &boxes, int width, int height)
        {
            //Clip boxes to image boundaries.
            for (size_t i = 0; i < boxes.size(); i++)
            {
                if (boxes[i].x < 0)
                {
                    boxes[i].x = 0;
                }
                if (boxes[i].y < 0)
                {
                    boxes[i].y = 0;
                }
                if (boxes[i].x + boxes[i].w > width)
                {
                    boxes[i].w = width - boxes[i].x;
                }
                if (boxes[i].y + boxes[i].h > height)
                {
                    boxes[i].h = height - boxes[i].y;
                }
                //        boxes[i].x1 = std::max<float>(std::min<float>(boxes[i].x1, width - 1), 0);
                //        boxes[i].y1 = std::max<float>(std::min<float>(boxes[i].y1, height - 1), 0);
                //        boxes[i].x2 = std::max<float>(std::min<float>(boxes[i].x2, width - 1), 0);
                //        boxes[i].y2 = std::max<float>(std::min<float>(boxes[i].y2, height - 1), 0);
            }
        }

        inline void clip_box(anchor_box &box, int width, int height)
        {
            //Clip boxes to image boundaries.
            if (box.x < 0)
            {
                box.x = 0;
            }
            if (box.y < 0)
            {
                box.y = 0;
            }
            if (box.x + box.w > width)
            {
                box.w = width - box.x;
            }
            if (box.y + box.h > height)
            {
                box.h = height - box.y;
            }
            //    boxes[i].x1 = std::max<float>(std::min<float>(boxes[i].x1, width - 1), 0);
            //    boxes[i].y1 = std::max<float>(std::min<float>(boxes[i].y1, height - 1), 0);
            //    boxes[i].x2 = std::max<float>(std::min<float>(boxes[i].x2, width - 1), 0);
            //    boxes[i].y2 = std::max<float>(std::min<float>(boxes[i].y2, height - 1), 0);
        }

        inline std::vector<anchor_box> bbox_pred(std::vector<anchor_box> anchors, std::vector<std::vector<float>> regress)
        {
            //"""
            //  Transform the set of class-agnostic boxes into class-specific boxes
            //  by applying the predicted offsets (box_deltas)
            //  :param boxes: !important [N 4]
            //  :param box_deltas: [N, 4 * num_classes]
            //  :return: [N 4 * num_classes]
            //  """

            std::vector<anchor_box> rects(anchors.size());
            for (size_t i = 0; i < anchors.size(); i++)
            {
                float width = anchors[i].w;
                float height = anchors[i].h;
                float ctr_x = anchors[i].x + 0.5 * (width - 1.0);
                float ctr_y = anchors[i].y + 0.5 * (height - 1.0);

                float pred_ctr_x = regress[i][0] * width + ctr_x;
                float pred_ctr_y = regress[i][1] * height + ctr_y;
                float pred_w = exp(regress[i][2]) * width;
                float pred_h = exp(regress[i][3]) * height;

                rects[i].x = pred_ctr_x - 0.5 * (pred_w - 1.0);
                rects[i].y = pred_ctr_y - 0.5 * (pred_h - 1.0);
                rects[i].w = pred_w;
                rects[i].h = pred_h;
            }

            return rects;
        }

        inline void refine(face_info_internal &face, const int &height, const int &width, bool square)
        {
            float bbw = 0, bbh = 0, maxSide = 0, minSide = 0;
            float h = 0, w = 0;
            float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
            bbw = face.rect.w - 1;
            bbh = face.rect.h - 1;
            x1 = face.rect.x;
            y1 = face.rect.y;

            if (square)
            {
                maxSide = (bbh > bbw) ? bbh : bbw;
                x1 = x1 + bbw * 0.5 - maxSide * 0.5;
                y1 = y1 + bbh * 0.5 - maxSide * 0.5;
                face.rect.w = round(maxSide + 1);
                face.rect.h = round(maxSide + 1);
                face.rect.x = round(x1);
                face.rect.y = round(y1);
            }

            //boundary check
            if (face.rect.x < 0)
                face.rect.x = 0;
            if (face.rect.y < 0)
                face.rect.y = 0;
            if (face.rect.x + face.rect.w - 1 > width)
                face.rect.w = width - face.rect.x;
            if (face.rect.y + face.rect.h - 1 > height)
                face.rect.h = height - face.rect.y;

            minSide = (face.rect.h > face.rect.w) ? face.rect.w : face.rect.h;
            face.rect.h = minSide;
            face.rect.w = minSide;
        }

        inline anchor_box bbox_pred(anchor_box anchor, std::vector<float> regress)
        {
            anchor_box rect;

            float width = anchor.w;
            float height = anchor.h;
            float ctr_x = anchor.x + 0.5 * (width - 1.0);
            float ctr_y = anchor.y + 0.5 * (height - 1.0);

            float pred_ctr_x = regress[0] * width + ctr_x;
            float pred_ctr_y = regress[1] * height + ctr_y;
            float pred_w = exp(regress[2]) * width;
            float pred_h = exp(regress[3]) * height;

            rect.x = pred_ctr_x - 0.5 * (pred_w - 1.0);
            rect.y = pred_ctr_y - 0.5 * (pred_h - 1.0);
            rect.w = pred_w;
            rect.h = pred_h;

            return rect;
        }

        inline std::vector<face_pts> landmark_pred(std::vector<anchor_box> anchors, std::vector<face_pts> facepts)
        {
            std::vector<face_pts> pts(anchors.size());
            for (size_t i = 0; i < anchors.size(); i++)
            {
                float width = anchors[i].w;
                float height = anchors[i].h;
                float ctr_x = anchors[i].x + 0.5 * (width - 1.0);
                float ctr_y = anchors[i].y + 0.5 * (height - 1.0);

                for (size_t j = 0; j < 5; j++)
                {
                    pts[i].x[j] = facepts[i].x[j] * width + ctr_x;
                    pts[i].y[j] = facepts[i].y[j] * height + ctr_y;
                }
            }

            return pts;
        }

        inline face_pts landmark_pred(anchor_box anchor, face_pts facePt)
        {
            face_pts pt;
            float width = anchor.w;
            float height = anchor.h;
            float ctr_x = anchor.x + 0.5 * (width - 1.0);
            float ctr_y = anchor.y + 0.5 * (height - 1.0);

            for (size_t j = 0; j < 5; j++)
            {
                pt.x[j] = facePt.x[j] * width + ctr_x;
                pt.y[j] = facePt.y[j] * height + ctr_y;
            }

            return pt;
        }

        inline bool compare_bbox(const face_info_internal &a, const face_info_internal &b)
        {
            return a.score > b.score;
        }

        inline std::vector<face_info_internal> nms(std::vector<face_info_internal> &bboxes, float threshold)
        {
            std::vector<face_info_internal> bboxes_nms;
            std::sort(bboxes.begin(), bboxes.end(), std::bind(&impl::compare_bbox, this, std::placeholders::_1, std::placeholders::_2));

            int32_t select_idx = 0;
            int32_t num_bbox = static_cast<int32_t>(bboxes.size());
            std::vector<int32_t> mask_merged(num_bbox, 0);
            bool all_merged = false;

            while (!all_merged)
            {
                while (select_idx < num_bbox && mask_merged[select_idx] == 1)
                    select_idx++;

                if (select_idx == num_bbox)
                {
                    all_merged = true;
                    continue;
                }

                bboxes_nms.push_back(bboxes[select_idx]);
                mask_merged[select_idx] = 1;

                anchor_box select_bbox = bboxes[select_idx].rect;
                float area1 = static_cast<float>((select_bbox.w) * (select_bbox.h));
                float x1 = static_cast<float>(select_bbox.x);
                float y1 = static_cast<float>(select_bbox.y);
                float x2 = static_cast<float>(select_bbox.w + select_bbox.x - 1);
                float y2 = static_cast<float>(select_bbox.h + select_bbox.y - 1);

                select_idx++;
                for (int32_t i = select_idx; i < num_bbox; i++)
                {
                    if (mask_merged[i] == 1)
                        continue;

                    anchor_box &bbox_i = bboxes[i].rect;
                    float x = std::max<float>(x1, static_cast<float>(bbox_i.x));
                    float y = std::max<float>(y1, static_cast<float>(bbox_i.y));
                    float w = std::min<float>(x2, static_cast<float>(bbox_i.w + bbox_i.x - 1)) - x + 1; //<- float �Ͳ���1
                    float h = std::min<float>(y2, static_cast<float>(bbox_i.h + bbox_i.y - 1)) - y + 1;
                    if (w <= 0 || h <= 0)
                        continue;

                    float area2 = static_cast<float>((bbox_i.w) * (bbox_i.h));
                    float area_intersect = w * h;

                    if (static_cast<float>(area_intersect) / (area1 + area2 - area_intersect) > threshold)
                    {
                        mask_merged[i] = 1;
                    }
                }
            }

            return bboxes_nms;
        }

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        inline void tracking_corrfilter(const cv::Mat &frame, const cv::Mat &face_in_prev_frame, cv::Rect2f& track_box, float scale)
        {
            track_box.x /= scale;
            track_box.y /= scale;
            track_box.height /= scale;
            track_box.width /= scale;
            int zeroadd_x = 0;
            int zeroadd_y = 0;
            cv::Mat frame_;
            cv::Mat model_;
            cv::resize(frame, frame_, cv::Size(frame.cols / scale, frame.rows / scale));
            cv::resize(face_in_prev_frame, model_, cv::Size(face_in_prev_frame.cols / scale, face_in_prev_frame.rows / scale));
            cv::Mat gray;
            cv::cvtColor(frame_, gray, CV_BGR2GRAY);
            cv::Mat gray_model;
            cv::cvtColor(model_, gray_model, CV_BGR2GRAY);
            cv::Rect2f search_window;
            search_window.width = track_box.width * 3;
            search_window.height = track_box.height * 3;
            search_window.x = track_box.x + track_box.width * 0.5 - search_window.width * 0.5;
            search_window.y = track_box.y + track_box.height * 0.5 - search_window.height * 0.5;
            search_window &= cv::Rect2f(0, 0, frame_.cols, frame_.rows);

            cv::Mat similarity;
            cv::Mat match_roi;
            safty_cut(gray, match_roi, search_window);
            matchTemplateCpu(match_roi, gray_model, similarity);
            cv::Point minpoint;
            //find min-distance point
            cv::minMaxLoc(similarity, 0, 0, &minpoint, 0);
            track_box.x = minpoint.x + search_window.x;
            track_box.y = minpoint.y + search_window.y;
            track_box.x *= scale;
            track_box.y *= scale;
            track_box.height *= scale;
            track_box.width *= scale;
        }

        void matchTemplateCpu(const cv::Mat& img, const cv::Mat& templ, cv::Mat& result)
        {
            result = cv::Mat(img.rows - templ.rows + 1, img.cols - templ.cols + 1, CV_32FC1);
            const std::uint8_t* img_data = img.data;
            const std::uint8_t* templ_data = templ.data;
            float* result_data = reinterpret_cast<float*>(result.data);
            for (int y = 0; y < result.rows; y++)
            {
                float* presult = result_data + y * result.cols;
                for (int x = 0; x < result.cols; x++)
                {
                    long sum = 0;
                    for (int yy = 0; yy < templ.rows; yy++)
                    {
                        const unsigned char* pimg = img_data + (y + yy) * img.cols;
                        const unsigned char* ptempl = templ_data + (yy)*templ.cols;
                        for (int xx = 0; xx < templ.cols; xx++)
                        {
                            int diff = pimg[x + xx] - ptempl[xx];
                            sum += (diff * diff);
                        }
                    }
                    presult[x] = sum;
                }
            }
        }
#else
        inline void tracking_corrfilter(const std::shared_ptr<memory::tensor<std::uint8_t>>& frame, const std::shared_ptr<memory::tensor<std::uint8_t>>& face_in_prev_frame, excalibur::rectangle<float>& track_box, float scale)
        {
            track_box.x /= scale;
            track_box.y /= scale;
            track_box.h /= scale;
            track_box.w /= scale;
            int zeroadd_x = 0;
            int zeroadd_y = 0;
            std::shared_ptr<memory::tensor<std::uint8_t>> frame_;
            std::shared_ptr<memory::tensor<std::uint8_t>> model_;
            excalibur::resize_cpu(frame, frame_, frame->height() / scale, frame->width() / scale);
            excalibur::resize_cpu(face_in_prev_frame, model_, face_in_prev_frame->height() / scale, face_in_prev_frame->width() / scale);
            std::shared_ptr<memory::tensor<std::uint8_t>> gray;
            excalibur::rgb2gray_cpu(frame_, gray);
            std::shared_ptr<memory::tensor<std::uint8_t>> gray_model;
            excalibur::rgb2gray_cpu(model_, gray_model);
            excalibur::rectangle<float> search_window;
            search_window.w = track_box.w * 3;
            search_window.h = track_box.h * 3;
            search_window.x = track_box.x + track_box.w * 0.5 - search_window.w * 0.5;
            search_window.y = track_box.y + track_box.h * 0.5 - search_window.h * 0.5;
            search_window &= excalibur::rectangle<float>(0, 0, frame_->height(), frame_->width());

            std::shared_ptr<memory::tensor<float>> similarity;
            std::shared_ptr<memory::tensor<std::uint8_t>> match_roi;
            excalibur::safty_cut_cpu(gray, match_roi, &search_window);
            matchTemplateCpu(match_roi, gray_model, similarity);
            excalibur::point<int> minpoint;
            //find min-distance point
            minMaxLoc(similarity, 0, 0, &minpoint, 0);
            track_box.x = minpoint.x + search_window.x;
            track_box.y = minpoint.y + search_window.y;
            track_box.x *= scale;
            track_box.y *= scale;
            track_box.h *= scale;
            track_box.w *= scale;
        }

        void matchTemplateCpu(const std::shared_ptr<memory::tensor<std::uint8_t>> &img, const std::shared_ptr<memory::tensor<std::uint8_t>> &templ, std::shared_ptr<memory::tensor<float>> &result)
        {
            result.reset(new memory::tensor<float>(std::vector<int>{1, 1, img->height() - templ->height() + 1, img->width() - templ->width() + 1}, -1, memory::NCHW /*, & memory::pool_allocator_default<float>::get()*/));
            const std::uint8_t *img_data = img->cpu_data();
            const std::uint8_t *templ_data = templ->cpu_data();
            float *result_data = result->mutable_cpu_data();
            for (int y = 0; y < result->height(); y++)
            {
                float *presult = result_data + y * result->width();
                for (int x = 0; x < result->width(); x++)
                {
                    long sum = 0;
                    for (int yy = 0; yy < templ->height(); yy++)
                    {
                        const unsigned char *pimg = img_data + (y + yy) * img->width();
                        const unsigned char *ptempl = templ_data + (yy)*templ->width();
                        for (int xx = 0; xx < templ->width(); xx++)
                        {
                            int diff = pimg[x + xx] - ptempl[xx];
                            sum += (diff * diff);
                        }
                    }
                    presult[x] = sum;
                }
            }
        }

        inline void minMaxIdx_(const float* src, float* _minVal, float* _maxVal,
            size_t* _minIdx, size_t* _maxIdx, int len, size_t startIdx)
        {
            float minVal = std::numeric_limits<float>::infinity(), maxVal = -minVal;
            size_t minIdx = 0, maxIdx = 0;

            for (int i = 0; i < len; i++)
            {
                float val = src[i];
                if (val < minVal)
                {
                    minVal = val;
                    minIdx = startIdx + i;
                }
                if (val > maxVal)
                {
                    maxVal = val;
                    maxIdx = startIdx + i;
                }
            }

            *_minIdx = minIdx;
            *_maxIdx = maxIdx;
            *_minVal = minVal;
            *_maxVal = maxVal;
        }

        inline void ofs2idx(const std::shared_ptr<memory::tensor<float>>& a, size_t ofs, excalibur::point<int>* loc)
        {
            if (ofs > 0)
            {
                ofs--;
                loc->x = (int)(ofs % a->width());
                loc->y = (int)(ofs / a->width());
            }
            else
            {
                loc->x = -1;
                loc->y = -1;
            }
        }

        inline void minMaxLoc(const std::shared_ptr<memory::tensor<float>>& _src, float* minVal, float* maxVal,
            excalibur::point<int>* minLoc, excalibur::point<int>* maxLoc)
        {
            size_t minidx = 0, maxidx = 0;
            size_t startidx = 1;
            int planeSize = _src->height() * _src->width();
            float minval, maxval;
            minMaxIdx_(_src->cpu_data(), &minval, &maxval, &minidx, &maxidx, planeSize, startidx);

            if (minVal)
                *minVal = minval;
            if (maxVal)
                *maxVal = maxval;

            if (minLoc)
                ofs2idx(_src, minidx, minLoc);
            if (maxLoc)
                ofs2idx(_src, maxidx, maxLoc);
        }
#endif

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        void tracking_landmark(cv::Mat& face, face_info_internal& trackfaceinfo, int offset_x, int offset_y)
        {
            int width = face.cols;
            int height = face.rows;

            cv::resize(face, face, cv::Size(80, 80));
            auto res = tracker_.forward(face.data, { 1, 80, 80, 3 }, RKNN_TENSOR_NHWC);
#ifdef USE_RKNNAPI
            trackfaceinfo.score = res["Softmax_Softmax_103/out0_2"]->cpu_data()[1];
            const float* glass_data = res["Softmax_Softmax_76/out0_3"]->cpu_data();
            const float* mask_data = res["Softmax_Softmax_79/out0_4"]->cpu_data();
            const float* landmark_data = res["MatMul_MatMul_124/out0_1"]->cpu_data();
            const float* bbox_data = res["MatMul_MatMul_113/out0_0"]->cpu_data();
#else
            trackfaceinfo.score = res["188"]->cpu_data()[1];
            const float* glass_data = res["157"]->cpu_data();
            const float* mask_data = res["161"]->cpu_data();
#if defined(BUILD_RV1106) 
            std::vector<std::string> intermediate_out{"185","199","212","114","118","122","126" };
            float *concat_ptr = concat.data();
            for (size_t i = 0; i < intermediate_out.size(); i++)
            {
                memcpy(concat_ptr, network_result1[intermediate_out[i]]->mutable_cpu_data(), network_result1[intermediate_out[i]]->count()*sizeof(float));
                concat_ptr+=network_result1[intermediate_out[i]]->count();
            }
            concat_ptr=nullptr; 

            auto result_215 = std::make_shared<glasssix::memory::tensor<float>>(shape1, -1, glasssix::memory::NCHW);
           
            const float* landmark_data=result_215->mutable_cpu_data();
            for (size_t i = 0; i < 14; i++)
            {   
                landmark_data[i]=0.f;
                for (size_t j = 0; j < 208; j++)
                {
                    landmark_data[i]+=(matmul_weight[i*208+j]*concat[j]);
                }  
            }
#else
            const float* landmark_data = res["215"]->cpu_data();
#endif
            const float* bbox_data = res["output"]->cpu_data();
#endif

            int x1 = bbox_data[0] * width / 10 + offset_x;
            int y1 = bbox_data[1] * height / 10 + offset_y;
            int x2 = bbox_data[2] * width / 10 + width + offset_x;
            int y2 = bbox_data[3] * height / 10 + height + offset_y;
            trackfaceinfo.rect.x = x1;
            trackfaceinfo.rect.w = x2 - x1 + 1;
            trackfaceinfo.rect.y = y1;
            trackfaceinfo.rect.h = y2 - y1 + 1;
            trackfaceinfo.glass_index = std::max_element(glass_data, glass_data + 3) - glass_data;
            trackfaceinfo.mask_index = std::max_element(mask_data, mask_data + 2) - mask_data;
            for (size_t i = 0; i < 2; i++)
            {
                trackfaceinfo.pts.x[i] = (landmark_data[4 * i] + landmark_data[4 * i + 2]) * width / 80 + offset_x;
                trackfaceinfo.pts.y[i] = (landmark_data[4 * i + 1] + landmark_data[4 * i + 3]) * height / 80 + offset_y;
            }

            for (size_t i = 2; i < 5; i++)
            {
                trackfaceinfo.pts.x[i] = landmark_data[2 * (i - 2) + 8] * width / 40 + offset_x;
                trackfaceinfo.pts.y[i] = landmark_data[2 * (i - 2) + 9] * height / 40 + offset_y;
            }

            float yaw, pitch, roll;
            estimate_head_pose(landmark_data, bbox_data, yaw, pitch, roll);
            trackfaceinfo.headpose[0] = yaw;
            trackfaceinfo.headpose[1] = pitch;

            float y_sub_eye = trackfaceinfo.pts.y[0] - trackfaceinfo.pts.y[1];
            float x_sub_eye = trackfaceinfo.pts.x[0] - trackfaceinfo.pts.x[1];
            float y_sub_mouth = trackfaceinfo.pts.y[3] - trackfaceinfo.pts.y[4];
            float x_sub_mouth = trackfaceinfo.pts.x[3] - trackfaceinfo.pts.x[4];

            float l2_eye = std::sqrt(y_sub_eye * y_sub_eye + x_sub_eye * x_sub_eye);
            float l2_mouth = std::sqrt(y_sub_mouth * y_sub_mouth + x_sub_mouth * x_sub_mouth);

            int n = 0;
            float mean_slope = 0.f;
            if ((l2_eye > std::numeric_limits<float>::epsilon()) && (x_sub_eye != 0))
            {
                mean_slope += y_sub_eye / x_sub_eye;
                n++;
            }

            if ((l2_mouth > std::numeric_limits<float>::epsilon()) && (x_sub_mouth != 0))
            {
                mean_slope += y_sub_mouth / x_sub_mouth;
                n++;
            }

            if (n != 0)
                mean_slope /= n;

            trackfaceinfo.headpose[2] = atan(mean_slope) * 180 / 3.1415926;
        }
#else
        void tracking_landmark(std::shared_ptr<memory::tensor<std::uint8_t>>& face, face_info_internal& trackfaceinfo, int offset_x, int offset_y)
        {

            int width = face->width();
            int height = face->height();
            excalibur::resize_cpu(face, face, 80, 80);
            auto res = tracker_.forward(face | memory::tensor_convert_to<float>);
            trackfaceinfo.score = res["188"]->cpu_data()[1];
            const float* glass_data = res["157"]->cpu_data();
            const float* mask_data = res["161"]->cpu_data();
            const float* landmark_data = res["215"]->cpu_data();
            const float *bbox_data = res["output"]->cpu_data();

            int x1 = bbox_data[0] * width / 10 + offset_x;
            int y1 = bbox_data[1] * height / 10 + offset_y;
            int x2 = bbox_data[2] * width / 10 + width + offset_x;
            int y2 = bbox_data[3] * height / 10 + height + offset_y;
            trackfaceinfo.rect.x = x1;
            trackfaceinfo.rect.w = x2 - x1 + 1;
            trackfaceinfo.rect.y = y1;
            trackfaceinfo.rect.h = y2 - y1 + 1;
            trackfaceinfo.glass_index = std::max_element(glass_data, glass_data + 3) - glass_data;
            trackfaceinfo.mask_index = std::max_element(mask_data, mask_data + 2) - mask_data;
            for (size_t i = 0; i < 2; i++)
            {
                trackfaceinfo.pts.x[i] = (landmark_data[4 * i] + landmark_data[4 * i + 2]) * width / 80 + offset_x;
                trackfaceinfo.pts.y[i] = (landmark_data[4 * i + 1] + landmark_data[4 * i + 3]) * height / 80 + offset_y;
            }

            for (size_t i = 2; i < 5; i++)
            {
                trackfaceinfo.pts.x[i] = landmark_data[2 * (i - 2) + 8] * width / 40 + offset_x;
                trackfaceinfo.pts.y[i] = landmark_data[2 * (i - 2) + 9] * height / 40 + offset_y;
            }

            float yaw, pitch, roll;
            estimate_head_pose(landmark_data, bbox_data, yaw, pitch, roll);
            trackfaceinfo.headpose[0] = yaw;
            trackfaceinfo.headpose[1] = pitch;
            
            float y_sub_eye = trackfaceinfo.pts.y[0] - trackfaceinfo.pts.y[1];
            float x_sub_eye = trackfaceinfo.pts.x[0] - trackfaceinfo.pts.x[1];
            float y_sub_mouth = trackfaceinfo.pts.y[3] - trackfaceinfo.pts.y[4];
            float x_sub_mouth = trackfaceinfo.pts.x[3] - trackfaceinfo.pts.x[4];

            float l2_eye = std::sqrt(y_sub_eye * y_sub_eye + x_sub_eye * x_sub_eye);
            float l2_mouth = std::sqrt(y_sub_mouth * y_sub_mouth + x_sub_mouth * x_sub_mouth);

            int n = 0;
            float mean_slope = 0.f;
            if ((l2_eye > std::numeric_limits<float>::epsilon()) && (x_sub_eye != 0))
            {
                mean_slope += y_sub_eye / x_sub_eye;
                n++;
            }

            if ((l2_mouth > std::numeric_limits<float>::epsilon()) && (x_sub_mouth != 0))
            {
                mean_slope += y_sub_mouth / x_sub_mouth;
                n++;
            }

            if (n != 0)
                mean_slope /= n;

            trackfaceinfo.headpose[2] = atan(mean_slope) * 180 / 3.1415926;
        }
#endif

        inline void estimate_head_pose(const float *ldmk7_data, const float *bbox_data, float &yaw, float &pitch, float &roll)
        {
            float ratio = 0.0f;
            ratio = (1.0f - bbox_data[0] / 10 + bbox_data[2] / 10);

            float ldmk_mat[2 * 7 + 1];
            for (size_t i = 0; i < 7; i++)
            {
                ldmk_mat[i * 2 + 0] = (ldmk7_data[i * 2 + 0] - bbox_data[0]) / 40 / ratio;
                ldmk_mat[i * 2 + 1] = (ldmk7_data[i * 2 + 1] - bbox_data[1]) / 40 / ratio;
            }

            ldmk_mat[2 * 7] = 1.0f;

            //ldmk_mat * weights_mat
            float predict[2] = {0.0f};

            for (size_t i = 0; i < 2; i++)
            {
                for (size_t j = 0; j < 15; j++)
                {
                    predict[i] += ldmk_mat[j] * estimate_head_pose_weights[j * 2 + i];
                }
            }

            yaw = predict[0];
            pitch = predict[1];
        }

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        inline void safty_cut(cv::Mat& img, cv::Mat& dst, cv::Rect roi)
        {
            int width = roi.width;
            int height = roi.height;
            int x = roi.x;
            int y = roi.y;

            cv::Mat mat(height, width, img.type(), cv::Scalar(0));
            int _x = x;
            int _y = y;
            int _width = width;
            int _height = height;
            if (x < 0)
            {
                _x = 0;
                _width = width + x;
            }

            if (_x + _width > img.cols)
                _width = img.cols - _x;

            if (y < 0)
            {
                _y = 0;
                _height = height + y;
            }

            if (_y + _height > img.rows)
                _height = img.rows - _y;

            img(cv::Rect(_x, _y, _width, _height)).copyTo(mat(cv::Rect(_x - x, _y - y, _width, _height)));
            dst = mat;
        }
#endif

    private:
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
//#if 0
        rknnwrapper::rknn_wrapper retina_;
        rknnwrapper::rknn_wrapper tracker_;
#else
        glasssix::excalibur::pipeline<float> retina_;
        glasssix::excalibur::pipeline<float> tracker_;
#endif

        int device_;
        float nms_threshold_;
        std::vector<float> ratio_;
        std::vector<anchor_cfg> cfg_;

        std::vector<int> feat_stride_fpn_;
        //each layer anchor shape of fpn
        std::map<std::string, std::vector<anchor_box>> anchors_fpn_;
        //each layer anchor of every points
        std::map<std::string, std::vector<anchor_box>> anchors_;
        //each layer's fpn has how many shapes of anchor = number of ratio * number of scales
        std::map<std::string, int> num_anchors_;

#if defined(BUILD_RV1106) 
        std::vector<float> matmul_weight;
#endif

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        //#if 0
        cv::Mat cache0_;
        cv::Mat cache1_;
#else
        std::shared_ptr<memory::tensor<std::uint8_t>> cache0_;
        std::shared_ptr<memory::tensor<std::uint8_t>> cache1_;
#endif
    };

    //######################################################################
    //retina_net_internal
    //######################################################################

    retina_net_internal::retina_net_internal(const exposing::param_string racy_path, const exposing::param_string tracker_racy_path, float nms_threshold, int device) : impl_{std::make_unique<impl>(racy_path, tracker_racy_path, nms_threshold, device)}
    {
    }

    retina_net_internal::retina_net_internal(const std::vector<std::string> &phai, const exposing::param_string racy_path, const std::vector<std::string> &tracker_phai, const exposing::param_string tracker_racy_path, float nms_threshold, int device)
        : impl_{std::make_unique<impl>(phai, racy_path, tracker_phai, tracker_racy_path, nms_threshold, device)}
    {
    }

    retina_net_internal::~retina_net_internal()
    {
    }

    exposing::param_vector<face_info> retina_net_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int min_size, float threshold, int order, bool do_attributing)
    {
        return impl_->detect(bitmap, channels, height, width, min_size, threshold, order, do_attributing);
    }

    face_info retina_net_internal::single_trace(face_info face, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
    {
        return impl_->single_trace(face, bitmap, channels, height, width, order);
    }

    exposing::param_vector<exposing::param_vector<std::uint8_t>> retina_net_internal::center_scale_align(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
        float scale, std::int32_t order) const
    {
        return impl_->center_scale_align(bitmap, channels, height, width, scale, order);
    }

    std::string retina_net_internal::version()
    {
        return impl::version();
    }
}
