#ifndef __TUMBLE_DETECT_CODE_INTERNAL_HPP__
#define __TUMBLE_DETECT_CODE_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include "box_info.hpp"

#include <opencv2/opencv.hpp>

namespace glasssix::tumble
{
    struct box_info_internal
    {
        int x1;
        int y1;
        int x2;
        int y2;
        float score;
        int category;
        exposing::param_string version;
    };

    struct yolo_result
    {
        int x1;
        int y1;
        int x2;
        int y2;
        int category;
        float score;
        yolo_result(int x1_,int y1_,int x2_,int y2_,int category_,float score_ ):x1(x1_),y1(y1_),x2(x2_),y2(y2_),category(category_),score(score_)
        {}

        box_info_internal safe_yolo_result(int pic_w,int pic_h)
        {
            box_info_internal temp;
            temp.x1 = std::max(0, x1 );
            temp.x2 = std::min(pic_w-1, x2);
            temp.y1 = std::max(0, y1 );
            temp.y2 = std::min(pic_h-1, y2 );
            temp.score    = score ;
            temp.category = category;
            return temp;
        }

        box_info_internal yolo_result2box()
        {
            box_info_internal temp;
            temp.x1         = x1;
            temp.y1         = y1;
            temp.x2         = x2;
            temp.y2         = y2;
            temp.score      = score ;
            temp.category   = category;
            return temp;
        }
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

        exposing::param_vector<tumble::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map) const;

    private:
        std::unique_ptr<impl> impl_;
    };
}
#endif