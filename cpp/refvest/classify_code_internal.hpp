#ifndef __CLASSIFY_CODE_INTERNAL_HPP__
#define __CLASSIFY_CODE_INTERNAL_HPP__

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstddef>
#include <cstdint>
#include <abi/param_span.hpp>

#include "box_info.hpp"

#include "../posture/box_info.hpp"

#include <opencv2/opencv.hpp>

namespace glasssix::refvest
{
    struct PostureInfo
    {
        std::int32_t xmin;
        std::int32_t ymin;
        std::int32_t xmax;
        std::int32_t ymax;
        float score;
        int category;
        std::vector<cv::Point> Kpoints;
        std::vector<float> Kpoints_score;

        PostureInfo(posture::box_info& b_info) {
            xmin = b_info.x1();
            xmax = b_info.x2();
            ymin = b_info.y1();
            ymax = b_info.y2();
            score = b_info.score();
            category = b_info.category();

            auto key_points = b_info.key_points(); // 3 elems peer group : x, y, score
            for (size_t i = 0; i < (int)key_points.size() / 3; i++) {
                cv::Point key_p;
                key_p.x = key_points[i * 3];
                key_p.y = key_points[i * 3 + 1];
                Kpoints_score.push_back(key_points[i * 3 + 2]);
                Kpoints.push_back(key_p);
            }
        }

        bool if_bodyerr() {
            if (Kpoints_score.size() != Kpoints.size()) return true;
            int err_counter = 0;
            std::array<int, 4> check_idxs{ 5,6,11,12 };
            for (auto idx : check_idxs) {
                if (Kpoints_score[idx] < 0.8) {
                    err_counter++;
                }
            }

            auto upperbody_img_area = get_vest_det_region().area();
            auto people_area = get_rect().area();
            return err_counter > 1 || (upperbody_img_area * 1.f) < (people_area * 1.f / 6);
        }

        cv::Rect get_vest_det_region() {
            std::vector<cv::Point> Kpoints_temp{
                Kpoints[5],
                Kpoints[6],
                Kpoints[7],
                Kpoints[8],
                Kpoints[11],
                Kpoints[12],
            };

            auto minmax_y = std::minmax_element(Kpoints_temp.begin(), Kpoints_temp.end(), [](cv::Point& a, cv::Point& b) {
                return a.y < b.y; });
            int top = minmax_y.first->y;
            int bottom = minmax_y.second->y;

            auto minmax_x = std::minmax_element(Kpoints_temp.begin(), Kpoints_temp.end(), [](cv::Point& a, cv::Point& b) {
                return a.x < b.x; });
            int left = minmax_x.first->x;
            int right = minmax_x.second->x;

            return cv::Rect{
                cv::Point(std::round(left), std::round(top)),
                cv::Point(std::round(right), std::round(bottom)) };
        }

        cv::Rect get_rect() {
            return cv::Rect{
                cv::Point(std::round(xmin), std::round(ymin)),
                cv::Point(std::round(xmax), std::round(ymax)) };
        }
    };

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

    class classify_code_internal
    {
    public:
        class impl;

        /// <summary>
        /// Creates an instance with a specified GPU core or the default CPU.
        /// </summary>
        /// <param name="racy_path">The model path</param>
        /// <param name="device">The device ID; -1 for CPU or a non-negative number for a GPU core</param>
        classify_code_internal(std::string_view model_directory, int device);

        virtual ~classify_code_internal();

        classify_code_internal(const classify_code_internal&) = delete;
        classify_code_internal& operator=(const classify_code_internal&) = delete;

        std::string version();

        exposing::param_vector<refvest::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list, std::map<std::string, float>& param_map) const;

    private:
        std::unique_ptr<impl> impl_;
    };
}
#endif