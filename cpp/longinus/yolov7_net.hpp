#pragma once
#include "face_info.hpp"
#include "facedetector_base.hpp"

#include <memory>
#include <vector>
#include <map>
#include <abi/consumer.hpp>

namespace glasssix::longinus
{
    class yolov7_net : public facedetector_base
    {
    public:
        yolov7_net(std::string_view models_directory, int model_type, float nms_threshold = 0.4, int device = -1);
        yolov7_net() = delete;
        yolov7_net(const yolov7_net&) = delete;
        yolov7_net&operator=(const yolov7_net&) = delete;
        virtual ~yolov7_net();

        virtual exposing::param_vector<face_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int min_size = 16, float threshold = 0.5, int order = 0, bool do_attributing = false);

        virtual std::string version() const;

    private:
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
        std::unique_ptr<rknnwrapper::rknn_wrapper> yolov7_;
#else
        std::unique_ptr<glasssix::excalibur::pipeline<float>> yolov7_;
#endif
        int model_type_;
        float nms_threshold_;
    };
}
