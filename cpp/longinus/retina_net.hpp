#pragma once
#include "face_info.hpp"
#include "facedetector_base.hpp"

#include <memory>
#include <vector>
#include <map>
#include <abi/consumer.hpp>

#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>

namespace glasssix::longinus
{
    class retina_net : public facedetector_base
    {
    public:
        retina_net(std::string_view models_directory, int model_type, float nms_threshold = 0.4, int device = -1);
        retina_net() = delete;
        retina_net(const retina_net&) = delete;
        retina_net&operator=(const retina_net&) = delete;
        virtual ~retina_net();

        // Batch process have some advantage in inference but can't speed up preprocess and postprocess
        // TODO: implement
        //std::vector<std::vector<face_info>> detectBatchImages(std::vector<cv::Mat> imgs, float threshold = 0.5);
        //Test in GTX1060:
        // | model | speed | input size | preprocess time | inference | postprocess time |
        //	| :------ : | : ---- : | : -------- : | : ------------ - : | : ------ - : | : -------------- : |
        //	|  caffe | ????ms | 1920x1080 | ????ms | 61ms | ????ms      |
        //	|  caffe | ????ms | 1280��720 | ????ms | 44ms | ????ms      |
        //	|  caffe | 17.3ms | 640��480 | 3.9ms | 13.4ms | 1.0ms |
        virtual exposing::param_vector<face_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int min_size = 16, float threshold = 0.5, int order = 0, bool do_attributing = false);

        virtual std::string version() const;

    private:

        std::shared_ptr<PrePostProcessGenPipeline> retina_;
        int model_type_;
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
    };
}
