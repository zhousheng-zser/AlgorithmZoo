#include "facedetector_base.hpp"
#include "face_info_impl.hpp"

#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_safty_cut.hpp"
#include "Excalibur/operation_resize.hpp"
#include "Excalibur/operation_rgb2gray.hpp"
#include "Excalibur/operation_rotate.hpp"
#include "Primitives/tensor_conversions.hpp"
#include "hardcode.hpp"

#include "retina_net.hpp"


#include <cfloat>
#include <cmath>

namespace glasssix::longinus
{
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
            -48.752375, 79.479039105 };
    }

    facedetector_base::facedetector_base(std::string_view models_directory, int model_type, float nms_threshold, int device)
        : device_{ device }
    {
        //Excalibur needs to distinguish between float and int8 models, rknn and rknn2 does not
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        tracker_ = PrePostProcessGenPipeline::mkSharePipeline(std::string(models_directory) + "pfld_land71_simp.rknn", device);

#if defined(USE_RKNN2API)
#if defined(BUILD_RV1106) 
        matmul_weight_.resize(208 * 14);

        std::string filename = std::string(models_directory) + "/land71_supplement.dat";

        std::ifstream fin;

        fin.open(filename, std::ios::in | std::ios::binary);
        if (fin.is_open() == false)
        {
            throw exposing::abi_invalid_argument("rv1106 supplement weight dat not find");
        }
        fin.read((char*)matmul_weight_.data(), 208 * 14 * sizeof(float));
        fin.close();
#endif
#endif
#else
        tracker_ = std::shared_ptr<excalibur::pipeline<float>>(get_model_params(std::string("pfld_land71_simp")), std::string(models_directory) + "/pfld_land71_simp.racy", device);
        retina_ = PrePostProcessGenPipeline::mkSharePipeline(std::string(models_directory) + "pfld_land71_simp.bmodel", 0);
#endif
    }

    void facedetector_base::init_cache(exposing::param_span<std::uint8_t>& bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order, std::shared_ptr<memory::tensor<std::uint8_t>>& cache)
    {
        if (cache == nullptr || cache->channels() != channels || cache->height() != height || cache->width() != width || cache->order() != order)
        {
            std::vector<int> shape;
            if (order == memory::NCHW)
                shape = { static_cast<int>(1), channels, height, width };
            else if (order == memory::NHWC)
                shape = { static_cast<int>(1), height, width, channels };
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
    void facedetector_base::mat_safty_cut(cv::Mat& img, cv::Mat& dst, cv::Rect roi)
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

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
    inline void matchTemplateCpu(const cv::Mat& img, const cv::Mat& templ, cv::Mat& result)
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

    inline void tracking_corrfilter(const cv::Mat& frame, const cv::Mat& face_in_prev_frame, cv::Rect2f& track_box, float scale)
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
        facedetector_base::mat_safty_cut(gray, match_roi, search_window);
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
#else

    inline void matchTemplateCpu(const std::shared_ptr<memory::tensor<std::uint8_t>>& img, const std::shared_ptr<memory::tensor<std::uint8_t>>& templ, std::shared_ptr<memory::tensor<float>>& result)
    {
        result.reset(new memory::tensor<float>(std::vector<int>{1, 1, img->height() - templ->height() + 1, img->width() - templ->width() + 1}, -1, memory::NCHW /*, & memory::pool_allocator_default<float>::get()*/));
        const std::uint8_t* img_data = img->cpu_data();
        const std::uint8_t* templ_data = templ->cpu_data();
        float* result_data = result->mutable_cpu_data();
        for (int y = 0; y < result->height(); y++)
        {
            float* presult = result_data + y * result->width();
            for (int x = 0; x < result->width(); x++)
            {
                long sum = 0;
                for (int yy = 0; yy < templ->height(); yy++)
                {
                    const unsigned char* pimg = img_data + (y + yy) * img->width();
                    const unsigned char* ptempl = templ_data + (yy)*templ->width();
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
#endif

    inline void estimate_head_pose(const float* ldmk7_data, const float* bbox_data, float& yaw, float& pitch, float& roll)
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
        float predict[2] = { 0.0f };

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

    void facedetector_base::tracking_landmark(cv::Mat& face, face_info_internal& trackfaceinfo, int offset_x, int offset_y)
    {
        int width = face.cols;
        int height = face.rows;

        cv::resize(face, face, cv::Size(80, 80));
        auto res = tracker_->forward(face);
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
#if defined(USE_RKNN2API) && defined(BUILD_RV1106) 
        std::array<std::string, 7> intermediate_out{ "185","199","212","114","118","122","126" };
        float concat_data[208] = { 0.f };
        float* ptr = concat_data;
        for (size_t i = 0; i < intermediate_out.size(); i++)
        {
            std::memcpy(ptr, res[intermediate_out[i]]->cpu_data(), res[intermediate_out[i]]->count() * sizeof(float));
            ptr += res[intermediate_out[i]]->count();
        }

        float landmark_data[14] = { 0.f };
        excalibur::juliusblas::cblas_sgemv_AnoTrans(14, 208, 1.f, matmul_weight_.data(), 208, concat_data, 1, 0.f, landmark_data, 1);
        //for (size_t i = 0; i < 14; i++)
        //{   
        //    float lmrk = 0.f;
        //    for (size_t j = 0; j < 208; j++)
        //    {
        //        lmrk += (matmul_weight_[i*208+j]*concat_data[j]);
        //    }
        //}
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


    void facedetector_base::refine(face_info_internal& face, const int& height, const int& width, bool square)
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
    exposing::param_vector<exposing::param_vector<std::uint8_t>> facedetector_base::center_scale_align(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
        float scale, std::int32_t order)
    {
        if (bitmap.empty())
        {
            throw exposing::abi_invalid_argument("current frame is empty");
        }
        if (order != 1)
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
        mat_safty_cut(img, ori_img, ori_rect);

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

        mat_safty_cut(img, ROI, MarginRect);
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
        mat_safty_cut(rotated_ROI, final_mat, final_rect);

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

}
