#pragma once

#include "face_info.hpp"
#include "facedetector_base.hpp"

#include <memory>
#include <vector>
#include <map>
#include <abi/consumer.hpp>

namespace glasssix::longinus
{
    class yolo_net : public facedetector_base
    {
    public:
        yolo_net(std::string_view models_directory, int model_type, float nms_threshold = 0.4, int device = -1);
        yolo_net() = delete;
        yolo_net(const yolo_net&) = delete;
        yolo_net&operator=(const yolo_net&) = delete;
        virtual ~yolo_net();

        virtual exposing::param_vector<face_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int min_size = 16, float threshold = 0.5, int order = 0, bool do_attributing = false);

        virtual std::string version() const;

    private:

        std::shared_ptr<GenPipeline> yolo_face_;
       
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API) 
    #ifdef USE_RKNN2API
            std::shared_ptr<Yolov8_Complement<GenPipeline>> Yolo_instance;
    #else
            std::shared_ptr<Yolov7<GenPipeline,false,true>> Yolo_instance;
    #endif
#endif

#if defined(USE_BMNN)  //only yolov8
            std::shared_ptr<SophonYolov8Wrapper> Yolo_instance;
#endif

        // std::string models_directory;

        int model_type_;
        float nms_threshold_;
    };
}
