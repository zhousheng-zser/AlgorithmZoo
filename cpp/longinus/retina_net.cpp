#include <vector>
#include <functional>
#include <map>
#include <cfloat>
#include <cmath>
#include "retina_net.hpp"
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
#if defined(BUILD_RV1106) 
#include <fstream>
#include "Julius/julius_gemv.hpp"
#endif
#endif


namespace glasssix::longinus
{
    namespace
    {
        //processing
        static inline anchor_win whctrs(anchor_box anchor)
        {
            //Return width, height, x center, and y center for an anchor (window).
            anchor_win win;
            win.w = anchor.w;
            win.h = anchor.h;
            win.x_ctr = anchor.x + 0.5 * (win.w - 1);
            win.y_ctr = anchor.y + 0.5 * (win.h - 1);

            return win;
        }

        static inline anchor_box make_anchors(anchor_win win)
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

        static inline std::vector<anchor_box> ratio_enum(anchor_box anchor, std::vector<float> ratios)
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

        static inline std::vector<anchor_box> scale_enum(anchor_box anchor, std::vector<int> scales)
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

        static inline std::vector<anchor_box> generate_anchors(int base_size = 16, std::vector<float> ratios = { 0.5, 1, 2 },
            std::vector<int> scales = { 8, 64 }, int stride = 16, bool dense_anchor = false)
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

        static inline std::vector<std::vector<anchor_box>> generate_anchors_fpn(bool dense_anchor = false, std::vector<anchor_cfg> cfg = {})
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

        static inline std::vector<anchor_box> anchors_plane(int height, int width, int stride, std::vector<anchor_box> base_anchors)
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

        static inline void clip_boxes(std::vector<anchor_box>& boxes, int width, int height)
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

        static inline void clip_box(anchor_box& box, int width, int height)
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

        static inline std::vector<anchor_box> bbox_pred(std::vector<anchor_box> anchors, std::vector<std::vector<float>> regress)
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

        //static inline void refine(face_info_internal &face, const int &height, const int &width, bool square)
        //{
        //    float bbw = 0, bbh = 0, maxSide = 0, minSide = 0;
        //    float h = 0, w = 0;
        //    float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        //    bbw = face.rect.w - 1;
        //    bbh = face.rect.h - 1;
        //    x1 = face.rect.x;
        //    y1 = face.rect.y;

        //    if (square)
        //    {
        //        maxSide = (bbh > bbw) ? bbh : bbw;
        //        x1 = x1 + bbw * 0.5 - maxSide * 0.5;
        //        y1 = y1 + bbh * 0.5 - maxSide * 0.5;
        //        face.rect.w = round(maxSide + 1);
        //        face.rect.h = round(maxSide + 1);
        //        face.rect.x = round(x1);
        //        face.rect.y = round(y1);
        //    }

        //    //boundary check
        //    if (face.rect.x < 0)
        //        face.rect.x = 0;
        //    if (face.rect.y < 0)
        //        face.rect.y = 0;
        //    if (face.rect.x + face.rect.w - 1 > width)
        //        face.rect.w = width - face.rect.x;
        //    if (face.rect.y + face.rect.h - 1 > height)
        //        face.rect.h = height - face.rect.y;

        //    minSide = (face.rect.h > face.rect.w) ? face.rect.w : face.rect.h;
        //    face.rect.h = minSide;
        //    face.rect.w = minSide;
        //}

        static inline anchor_box bbox_pred(anchor_box anchor, std::vector<float> regress)
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

        static inline std::vector<face_pts> landmark_pred(std::vector<anchor_box> anchors, std::vector<face_pts> facepts)
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

        static inline face_pts landmark_pred(anchor_box anchor, face_pts facePt)
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

        static inline bool compare_bbox(const face_info_internal& a, const face_info_internal& b)
        {
            return a.score > b.score;
        }

        static inline std::vector<face_info_internal> nms(std::vector<face_info_internal>& bboxes, float threshold)
        {
            std::vector<face_info_internal> bboxes_nms;
            std::sort(bboxes.begin(), bboxes.end(), std::bind(&compare_bbox, std::placeholders::_1, std::placeholders::_2));

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

                    anchor_box& bbox_i = bboxes[i].rect;
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
    }

    retina_net::retina_net(std::string_view models_directory, int model_type, float nms_threshold, int device)
        : facedetector_base{ models_directory, model_type, nms_threshold, device },
        model_type_{ model_type },
        nms_threshold_{ nms_threshold }
    {
        //Excalibur needs to distinguish between float and int8 models, rknn and rknn2 does not
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        switch (model_type)
        {
        case 0:
            retina_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params(std::string("retina")), std::string(models_directory) + "/retina_320.rknn", device);
            break;
        case 1:
            retina_ = std::make_unique<rknnwrapper::rknn_wrapper>(get_model_params(std::string("retina")), std::string(models_directory) + "/retina_640.rknn", device);
            break;
        default:
            throw exposing::abi_invalid_argument("Invalid model_type param!");
            break;
        }
#else
        retina_ = std::make_unique<excalibur::pipeline<float>>(get_model_params(std::string("retina")), std::string(models_directory) + "/retina.racy", device);
#endif

        ratio_ = { 1.0 };
        //anchor setting
        feat_stride_fpn_ = { 32, 16, 8 };
        anchor_cfg tmp;
        tmp.SCALES = { 32, 16 };
        tmp.BASE_SIZE = 16;
        tmp.RATIOS = ratio_;
        tmp.ALLOWED_BORDER = 9999;
        tmp.STRIDE = 32;
        cfg_.push_back(tmp);

        tmp.SCALES = { 8, 4 };
        tmp.BASE_SIZE = 16;
        tmp.RATIOS = ratio_;
        tmp.ALLOWED_BORDER = 9999;
        tmp.STRIDE = 16;
        cfg_.push_back(tmp);

        tmp.SCALES = { 2, 1 };
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
    }

    retina_net::~retina_net()
    {
    }

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
    exposing::param_vector<face_info> retina_net::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int min_size, float threshold, int order, bool do_attributing)
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
        int fix_size = 320;
        if (model_type_ == 1)
            fix_size = 640;

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
        auto blob_data = retina_->forward(cache_forward.data, { 1, hs, ws, 3 }, RKNN_TENSOR_NHWC);

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
                mat_safty_cut(cache_temp, faceROI_in_frame, rect);
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
    exposing::param_vector<face_info> retina_net::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int min_size, float threshold, int order, bool do_attributing)
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

        auto blob_data = retina_->forward(cache_forward | memory::tensor_convert_to<float>);

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

    std::string retina_net::version() const
    {
        return "1.0.0";
    }
}
