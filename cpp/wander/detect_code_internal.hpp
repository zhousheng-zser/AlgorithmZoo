#ifndef __DETECT_CODE_INTERNAL_HPP__
#define __DETECT_CODE_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include "box_info.hpp"

namespace glasssix::wander
{
    struct box_info_internal
    {
        int x1;
        int y1;
        int x2;
        int y2;
        float confidence;
        int id;
        double first_show_time=0.f;
        double last_show_time=0.f;
        float  cosine_similarity=0.f;
    };


    class detect_code_internal 
    {
    public:
        class impl;

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
        detect_code_internal(std::string_view model_directory, int device);

        virtual ~detect_code_internal();

        detect_code_internal(const detect_code_internal&) = delete;
        detect_code_internal& operator=(const detect_code_internal&) = delete;

        std::string version();

        std::string remove_library(int id);
   
        exposing::param_vector<wander::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, double>& param_map) const;
        //成员函数
    private:
        std::unique_ptr<impl> impl_;
    };

}
#endif