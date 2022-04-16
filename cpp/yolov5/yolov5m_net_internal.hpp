#ifndef __YOLOV5M_NET_INTERNAL_HPP__
#define __YOLOV5M_NET_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include "result_info.hpp"

namespace glasssix::yolov5
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

    class yolov5m_net_internal
    {
    public:
        class impl;

        yolov5m_net_internal(std::string_view yolov5m_racy_path, int device);
        yolov5m_net_internal(std::string_view yolov5m_phai, std::string_view yolov5m_racy_path, int device);
        yolov5m_net_internal(const std::vector<std::string>& yolov5m_phai, std::string_view yolov5m_racy_path, int device);

        yolov5m_net_internal();
        virtual ~yolov5m_net_internal();

        yolov5m_net_internal(const yolov5m_net_internal&) = delete;
        yolov5m_net_internal& operator=(const yolov5m_net_internal&) = delete;
        

        static std::string version();

        yolov5::result_info detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const;

    private:
        std::unique_ptr<impl> impl_;
    };
}
#endif