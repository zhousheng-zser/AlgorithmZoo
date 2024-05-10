#pragma once

#include "face_info.hpp"
#include <vector>

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#endif

#include <GenPipeline/GenPipeline.hpp>
#include <YoloFamily/Yolo_wrapper.hpp>

#ifdef USE_RKNNAPI
#include "RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "RKNN2Wrapper/rknn2_wrapper.hpp"
#if defined(BUILD_RV1106) 
#include <fstream>
#include "Julius/julius_gemv.hpp"
#endif
#else
#include "tensor.hpp"
#include "Excalibur/pipeline.hpp"
#endif

#include <abi/consumer.hpp>

namespace glasssix::longinus
{
    struct anchor_win
    {
        float x_ctr;
        float y_ctr;
        float w;
        float h;
    };

    struct anchor_box
    {
        float x;
        float y;
        float h;
        float w;
    };

    struct face_pts
    {
        float x[5];
        float y[5];
    };

    struct face_info_internal
    {
        float clarity;
        float has_mask;
        int mask_index;
        int glass_index;
        float score;
        anchor_box rect;
        anchor_box ori_rect;
        face_pts pts;
        float headpose[3];
        bool is_alive;
    };

    struct anchor_cfg
    {
    public:
        int STRIDE;
        std::vector<int> SCALES;
        int BASE_SIZE;
        std::vector<float> RATIOS;
        int ALLOWED_BORDER;

        anchor_cfg()
        {
            STRIDE = 0;
            SCALES.clear();
            BASE_SIZE = 0;
            RATIOS.clear();
            ALLOWED_BORDER = 0;
        }
    };

	class facedetector_base
	{
	public:
        facedetector_base() = delete;
        facedetector_base(std::string_view models_directory, int model_type, float nms_threshold = 0.4, int device = -1);
        facedetector_base(const facedetector_base&) = delete;
        facedetector_base& operator=(const facedetector_base&) = delete;
        virtual ~facedetector_base() {}
		virtual exposing::param_vector<face_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int min_size = 16, float threshold = 0.5, int order = 0, bool do_attributing = false) = 0;

		face_info single_trace(face_info face, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order = 0);
		exposing::param_vector<exposing::param_vector<std::uint8_t>> center_scale_align(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, float scale, std::int32_t order = 1);

		virtual std::string version() const = 0;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        static void mat_safty_cut(cv::Mat& img, cv::Mat& dst, cv::Rect roi);
#endif

    protected:
        void refine(face_info_internal& face, const int& height, const int& width, bool square);
        void init_cache(exposing::param_span<std::uint8_t>& bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order, std::shared_ptr<memory::tensor<std::uint8_t>>& cache);

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        void tracking_landmark(cv::Mat& face, face_info_internal& trackfaceinfo, int offset_x, int offset_y);
        cv::Mat cache0_;
        cv::Mat cache1_;
#else
        void tracking_landmark(std::shared_ptr<memory::tensor<std::uint8_t>>& face, face_info_internal& trackfaceinfo, int offset_x, int offset_y);
        std::shared_ptr<memory::tensor<std::uint8_t>> cache0_;
        std::shared_ptr<memory::tensor<std::uint8_t>> cache1_;
#endif

	private:
        int device_;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::unique_ptr<rknnwrapper::rknn_wrapper> tracker_;

#if defined(USE_RKNN2API)
#if defined(BUILD_RV1106) 
        std::vector<float> matmul_weight_;
#endif
#endif
#else
        std::unique_ptr<glasssix::excalibur::pipeline<float>> tracker_;
#endif
	};
}