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
        tracker_ = PrePostProcessGenPipeline::mkSharePipeline(std::string(models_directory) + "/pfld_land71_simp.rknn", device);
            pipeline_ = std::make_shared<GenPipeline>(std::string(models_directory) + "/face_landmark.rknn", 0);

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
            pipeline_ = std::make_shared<GenPipeline>(std::string(models_directory) + "/face_landmark.bmodel", 0);
			pipeline_->manual_possible_normalization(127.5f, 1.f / 127.5f);
        tracker_ = PrePostProcessGenPipeline::mkSharePipeline(std::string(models_directory) + "/pfld_land71_simp.bmodel", 0);
        tracker_->manual_possible_normalization(std::array<float,3>{104.f,117.f,124.f},std::array<float,3>{0.00961538f,0.008547f,0.00806451f});

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


    inline void estimate_head_pose(const float* ldmk7_data, const float* bbox_data, float& yaw, float& pitch, float& roll)
    {
        float ratio = 0.0f;
        ratio = (1.0f - bbox_data[0] / 10 + bbox_data[2] / 10);

        float ldmk_mat[2 * 7 + 1];
        for (size_t i = 0; i < 7; i++)
        {
            ldmk_mat[i * 2 + 0] = (ldmk7_data[i * 2 + 0] - bbox_data[0]) / ratio;
            ldmk_mat[i * 2 + 1] = (ldmk7_data[i * 2 + 1] - bbox_data[1]) / ratio;
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
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API) 
#ifdef USE_RKNNAPI
        trackfaceinfo.score = res["Softmax_Softmax_103/out0_2"]->cpu_data()[1];
        const float* glass_data = res["Softmax_Softmax_76/out0_3"]->cpu_data();
        const float* mask_data = res["Softmax_Softmax_79/out0_4"]->cpu_data();
        const float* landmark_data = res["MatMul_MatMul_124/out0_1"]->cpu_data();
        const float* bbox_data = res["MatMul_MatMul_113/out0_0"]->cpu_data();
#elif defined(USE_RKNN2API)
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
#if defined(USE_RKNN2API)
        const float* landmark_data = res["215"]->cpu_data();
#endif
#endif
        const float* bbox_data = res["output"]->cpu_data();
#endif
#endif

#if defined(USE_BMNN)
        trackfaceinfo.score = res["188_Softmax"]->cpu_data()[1];
        const float* glass_data = res["157_Softmax"]->cpu_data();
        const float* mask_data = res["161_Softmax"]->cpu_data();
        const float* landmark_data = res["215_MatMul_f32"]->cpu_data();
        const float* bbox_data = res["output_MatMul_f32"]->cpu_data();
#endif

        int x1 = bbox_data[0] * width / 10 + offset_x;
        int y1 = bbox_data[1] * height / 10 + offset_y;
        int x2 = bbox_data[2] * width / 10 + width + offset_x;
        int y2 = bbox_data[3] * height / 10 + height + offset_y;
        std::cout<< "res count: " << res["215"]->count() << ";" << std::endl;
        // int x1 = offset_x;
        // int y1 = offset_y;
        // int x2 = width ;
        // int y2 =  height ;

        trackfaceinfo.rect.x = x1;
        trackfaceinfo.rect.w = x2 - x1 + 1;
        trackfaceinfo.rect.y = y1;
        trackfaceinfo.rect.h = y2 - y1 + 1;
        trackfaceinfo.glass_index = std::max_element(glass_data, glass_data + 3) - glass_data;
        trackfaceinfo.mask_index = std::max_element(mask_data, mask_data + 2) - mask_data;
        for (size_t i = 0; i < 2; i++)//四个眼角
        {
            trackfaceinfo.pts.x[i] = (landmark_data[4 * i] + landmark_data[4 * i + 2]) * width / 80 + offset_x;
            trackfaceinfo.pts.y[i] = (landmark_data[4 * i + 1] + landmark_data[4 * i + 3]) * height / 80 + offset_y;
        }
        
       

        for (size_t i = 2; i < 5; i++)
        {
            trackfaceinfo.pts.x[i] = landmark_data[2 * (i - 2) + 8] * width / 40 + offset_x;
            trackfaceinfo.pts.y[i] = landmark_data[2 * (i - 2) + 9] * height / 40 + offset_y;
        }

        for (size_t i = 0; i < 2; i++)
        {
            cv::circle(face, cv::Point(int( trackfaceinfo.pts.x[i]-offset_x ), int(trackfaceinfo.pts.y[i]-offset_y ) ), 1, CV_RGB(125, 255, 0), 1);
        }
        cv::imwrite("face.jpg", face);

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

    void facedetector_base::tracking_landmark(cv::Mat& face, face_info_internal& trackfaceinfo, int offset_x, int offset_y, bool is_landmark)
    {
        int width = face.cols;
        int height = face.rows;

        cv::resize(face, face, cv::Size(128, 128));
        // auto res = tracker_->forward(face);
        auto res = pipeline_->forward(face);
            std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> nodes;
            for (auto& node : res) {
                nodes.emplace_back(node.second);
            }
            // std::sort(nodes.begin(), nodes.end(),
            //     [](std::shared_ptr<glasssix::memory::tensor<float>>& A, std::shared_ptr<glasssix::memory::tensor<float>>& B) {return A->count() < B->count(); });
            float* landmark_data = nodes[0]->mutable_cpu_data();
            // const size_t land_sz = nodes[0]->count();
            yolo_wrapper::Softmax(nodes[0]->mutable_cpu_data(), 2);

            // land_info_internal landmark;
            // landmark.score = nodes[0]->mutable_cpu_data()[1];

			// for (size_t i = 0; i < land_sz / 2; i++) {
            //     // when use cv::resize no pad, mul width & height
			// 	landmark.pts.push_back(exposing::make_param_pair(land[2 * i] * width, land[2 * i + 1] * height));
            // }
// #if defined(USE_RKNNAPI) || defined(USE_RKNN2API) 
// #ifdef USE_RKNNAPI
//         trackfaceinfo.score = res["Softmax_Softmax_103/out0_2"]->cpu_data()[1];
//         const float* glass_data = res["Softmax_Softmax_76/out0_3"]->cpu_data();
//         const float* mask_data = res["Softmax_Softmax_79/out0_4"]->cpu_data();
//         const float* landmark_data = res["MatMul_MatMul_124/out0_1"]->cpu_data();
//         const float* bbox_data = res["MatMul_MatMul_113/out0_0"]->cpu_data();
// #elif defined(USE_RKNN2API)
//         trackfaceinfo.score = res["188"]->cpu_data()[1];
//         const float* glass_data = res["157"]->cpu_data();
//         const float* mask_data = res["161"]->cpu_data();
// #if defined(USE_RKNN2API) && defined(BUILD_RV1106) 
//         std::array<std::string, 7> intermediate_out{ "185","199","212","114","118","122","126" };
//         float concat_data[208] = { 0.f };
//         float* ptr = concat_data;
//         for (size_t i = 0; i < intermediate_out.size(); i++)
//         {
//             std::memcpy(ptr, res[intermediate_out[i]]->cpu_data(), res[intermediate_out[i]]->count() * sizeof(float));
//             ptr += res[intermediate_out[i]]->count();
//         }

//         float landmark_data[14] = { 0.f };
//         excalibur::juliusblas::cblas_sgemv_AnoTrans(14, 208, 1.f, matmul_weight_.data(), 208, concat_data, 1, 0.f, landmark_data, 1);
//         //for (size_t i = 0; i < 14; i++)
//         //{   
//         //    float lmrk = 0.f;
//         //    for (size_t j = 0; j < 208; j++)
//         //    {
//         //        lmrk += (matmul_weight_[i*208+j]*concat_data[j]);
//         //    }
//         //}
// #else
// #if defined(USE_RKNN2API)
//         const float* landmark_data = res["215"]->cpu_data();
// #endif
// #endif
//         const float* bbox_data = res["output"]->cpu_data();
// #endif
// #endif

// #if defined(USE_BMNN)
//         trackfaceinfo.score = res["188_Softmax"]->cpu_data()[1];
//         const float* glass_data = res["157_Softmax"]->cpu_data();
//         const float* mask_data = res["161_Softmax"]->cpu_data();
//         const float* landmark_data = res["215_MatMul_f32"]->cpu_data();
//         const float* bbox_data = res["output_MatMul_f32"]->cpu_data();
// #endif

        // int x1 = bbox_data[0] * width / 10 + offset_x;
        // int y1 = bbox_data[1] * height / 10 + offset_y;
        // int x2 = bbox_data[2] * width / 10 + width + offset_x;
        // int y2 = bbox_data[3] * height / 10 + height + offset_y;
        // std::cout<< "res count: " << res["215"]->count() << ";" << std::endl;
        int x1 = offset_x;
        int y1 = offset_y;
        int x2 = width + offset_x;
        int y2 =  height + offset_y;
        // float * bbox_data{x1,y1,x2,y2};//这种写法不允许
        float * bbox_data;
        bbox_data[0] = float(x1);
        bbox_data[1] = float(x2);
        bbox_data[2] = float(y1);
        bbox_data[3] = float(y2);

        trackfaceinfo.rect.x = x1;
        trackfaceinfo.rect.w = x2 - x1 + 1;
        trackfaceinfo.rect.y = y1;
        trackfaceinfo.rect.h = y2 - y1 + 1;
        // trackfaceinfo.glass_index = std::max_element(glass_data, glass_data + 3) - glass_data;
        // trackfaceinfo.mask_index = std::max_element(mask_data, mask_data + 2) - mask_data;
        for (size_t i = 0; i < 2; i++)//四个眼角
        {
            trackfaceinfo.pts.x[i] = (landmark_data[4 * i] + landmark_data[4 * i + 2]) * width + offset_x;
            trackfaceinfo.pts.y[i] = (landmark_data[4 * i + 1] + landmark_data[4 * i + 3]) * height+ offset_y;
        }
        

        for (size_t i = 2; i < 5; i++)
        {
            trackfaceinfo.pts.x[i] = landmark_data[2 * (i - 2) + 8] * width + offset_x;
            trackfaceinfo.pts.y[i] = landmark_data[2 * (i - 2) + 9] * height + offset_y;
        }
#if 1 //五个关键点 trackfaceinfo.pts 只能接受5个点
        trackfaceinfo.pts.x[0] = landmark_data[192]* width + offset_x;//96
        trackfaceinfo.pts.y[0] = landmark_data[193] * height+ offset_y;
        trackfaceinfo.pts.x[1] = landmark_data[194]* width + offset_x;//97
        trackfaceinfo.pts.y[1] = landmark_data[195] * height+ offset_y;

        trackfaceinfo.pts.x[2] = landmark_data[108]* width + offset_x;//54
        trackfaceinfo.pts.y[2] = landmark_data[109] * height+ offset_y;
        trackfaceinfo.pts.x[3] = landmark_data[152]* width + offset_x;//76
        trackfaceinfo.pts.y[3] = landmark_data[153] * height+ offset_y;
        trackfaceinfo.pts.x[4] = landmark_data[164]* width + offset_x;//82
        trackfaceinfo.pts.y[4] = landmark_data[165] * height+ offset_y;
#else //七个关键点(trackfaceinfo.pts 无法接受7个点)
        trackfaceinfo.pts.x[0] = landmark_data[120]* width + offset_x;//60
        trackfaceinfo.pts.y[0] = landmark_data[121] * height+ offset_y;
        trackfaceinfo.pts.x[1] = landmark_data[128]* width + offset_x;//64
        trackfaceinfo.pts.y[1] = landmark_data[129] * height+ offset_y;
        trackfaceinfo.pts.x[2] = landmark_data[136]* width + offset_x;//68
        trackfaceinfo.pts.y[2] = landmark_data[137] * height+ offset_y;
        trackfaceinfo.pts.x[3] = landmark_data[144]* width + offset_x;//72
        trackfaceinfo.pts.y[3] = landmark_data[145] * height+ offset_y;

        trackfaceinfo.pts.x[4] = landmark_data[108]* width + offset_x;//54
        trackfaceinfo.pts.y[4] = landmark_data[109] * height+ offset_y;
        trackfaceinfo.pts.x[5] = landmark_data[152]* width + offset_x;//76
        trackfaceinfo.pts.y[5] = landmark_data[153] * height+ offset_y;
        trackfaceinfo.pts.x[6] = landmark_data[164]* width + offset_x;//82
        trackfaceinfo.pts.y[6] = landmark_data[165] * height+ offset_y;
#endif
        for (size_t i = 0; i < 2; i++)
        {
            cv::circle(face, cv::Point(int( trackfaceinfo.pts.x[i]-offset_x ), int(trackfaceinfo.pts.y[i]-offset_y ) ), 1, CV_RGB(125, 255, 0), 1);
        }
        // cv::imwrite("face.jpg", face);
        //目前14个数据
        std::vector<float> landmark_data_vector{
landmark_data[120],landmark_data[121],landmark_data[128],landmark_data[129],landmark_data[136],landmark_data[137],landmark_data[144],landmark_data[145],landmark_data[108],landmark_data[109],landmark_data[152],landmark_data[153],landmark_data[164],landmark_data[165]
        };
        float* landmark_data_seven = landmark_data_vector.data();
        
        float yaw, pitch, roll;
        estimate_head_pose(landmark_data_seven, bbox_data, yaw, pitch, roll);
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
    face_info facedetector_base::single_trace(face_info face, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
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
        mat_safty_cut(cache1_, face_in_prev_frame, track_box);

        cv::Mat cache_temp(height, width, CV_8UC3, bitmap.data());

        int min_edge = std::min(track_box.height, track_box.width);
        float scale = min_edge / 20.0f;
        if (scale < 1.0)
            scale = 1.0;

        tracking_corrfilter(cache_temp, face_in_prev_frame, track_box, scale);
        cv::Mat faceROI_in_frame;
        mat_safty_cut(cache_temp, faceROI_in_frame, track_box);
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
