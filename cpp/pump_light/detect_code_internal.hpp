#ifndef __DETECT_CODE_INTERNAL_HPP__
#define __DETECT_CODE_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>

#include <abi/param_span.hpp>
#include <opencv2/opencv.hpp>
#include "box_info.hpp"


#ifdef BUILD_DEBUG_INFO
#include <opencv2/highgui/highgui.hpp>



#define GetShowRatio(visual_img) std::min(float(1920.f / visual_img.cols), float(1080.f / visual_img.rows)) * 0.75
#define ShowResize(visual_img, showRatio) cv::resize(visual_img, visual_img, cv::Size(), showRatio, showRatio);
#endif // BUILD_DEBUG_INFO

namespace glasssix::pump_light
{
    struct box_info_internal
    {
        float score;
        bool light_status;
        exposing::param_string version;
    };

    class detect_code_internal
    {
    public:
        class impl;

        detect_code_internal(std::string_view model_directory, int device, int model_type);

        virtual ~detect_code_internal();

        detect_code_internal(const detect_code_internal&) = delete;
        detect_code_internal& operator=(const detect_code_internal&) = delete;

        std::string version();

        pump_light::box_info detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, std::map<std::string, float>& param_map) const;

    private:
        std::unique_ptr<impl> impl_;
    };

}
#endif