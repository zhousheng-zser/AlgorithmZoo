#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include <opencv2/core/core.hpp>

#include "box_info.hpp"
#include "../pedestrian/box_info.hpp"

namespace glasssix::pump_pumptop_person
{
    struct PedestrianInfo
    {
        PedestrianInfo(pedestrian::box_info& b_info) {
            x1 = b_info.x1();
            x2 = b_info.x2();
            y1 = b_info.y1();
            y2 = b_info.y2();
            score = b_info.score();
        }

        cv::Rect get_rect() {
            return cv::Rect{
                cv::Point(std::round(x1), std::round(y1)),
                cv::Point(std::round(x2), std::round(y2)) };
        }

        std::int32_t x1;
        std::int32_t y1;
        std::int32_t x2;
        std::int32_t y2;
        float score;
    };


    class detect_code_internal
    {
    public:
        class impl;

        detect_code_internal(const detect_code_internal &) = delete;

        detect_code_internal &operator=(const detect_code_internal &) = delete;

        detect_code_internal(std::string_view model_directory, int device);

        virtual ~detect_code_internal();

        exposing::param_vector<pump_pumptop_person::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int height, int width, const std::vector<PedestrianInfo>& pedestrian_info_list, std::map<std::string,float>& param_map_std);

        std::string version();

    private:
        std::unique_ptr<impl> impl_;
    };
}
