#ifndef __yolov5Deepsort_net_INTERNAL_HPP__
#define __yolov5Deepsort_net_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include "result_info.hpp"

namespace glasssix::pan
{
    struct anchor_box
    {
        float x;
        float y;
        float height;
        float width;
    };

    struct obj_info_internal
    {
        anchor_box rect;
        int label;
        float prob;
    };

    struct track_info_internal
    {
        anchor_box rect;
        int num;
    };

    class yolov5Deepsort_net_internal
    {
    public:
        class impl;

        yolov5Deepsort_net_internal(std::string_view yolov5m_racy_path, std::string_view deepsort_racy_path, int device);
        yolov5Deepsort_net_internal(std::string_view yolov5m_phai, std::string_view yolov5m_racy_path, std::string_view deepsort_phai, std::string_view deepsort_racy_path, int device);
        yolov5Deepsort_net_internal(const std::vector<std::string>& yolov5m_phai, std::string_view yolov5m_racy_path, const std::vector<std::string>& deepsort_phai, std::string_view deepsort_racy_path, int device);

        yolov5Deepsort_net_internal();
        virtual ~yolov5Deepsort_net_internal();

        yolov5Deepsort_net_internal(const yolov5Deepsort_net_internal&) = delete;
        yolov5Deepsort_net_internal& operator=(const yolov5Deepsort_net_internal&) = delete;
        
        static std::string version();

        pan::result_info detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const;

    private:
        std::unique_ptr<impl> impl_;
    };
}
#endif