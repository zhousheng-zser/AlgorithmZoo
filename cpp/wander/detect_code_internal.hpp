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
#include "../pedestrian/box_info.hpp"
#include <opencv2/opencv.hpp>
namespace glasssix::wander
{
    struct PedestrianInfo
    {
        PedestrianInfo(pedestrian::box_info& b_info) {
            x1 = b_info.x1();
            x2 = b_info.x2();
            y1 = b_info.y1();
            y2 = b_info.y2();
            score = b_info.score();
            //category = b_info.category();   

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
		//int category;//徘徊没用到这个值  
    };


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

        std::string remove_person_by_index(int device_id,int id);
   
        exposing::param_vector<wander::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, 
       int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, double>& param_map, const std::vector<PedestrianInfo>& pedestrain_info) const;
        //成员函数
    private:
        std::unique_ptr<impl> impl_;
    };

}
#endif