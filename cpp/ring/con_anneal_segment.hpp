#pragma once
#include<vector>
#include<opencv2/opencv.hpp>

namespace glasssix
{
namespace ring
{
float cartesian_to_radian(const cv::Point2f& center, const cv::Point2f& coord, float R);

std::vector<cv::Point2f> calcu_box_shrink_new(const cv::RotatedRect& rect, const std::vector<cv::Point2f>& box_shrink);

std::vector<cv::Point2f> findFoot(const cv::Mat& img, const std::vector<cv::Point2f>& init_search_points);

bool redirectRect(cv::Mat& img);

cv::Mat charBoxDet(const cv::Mat& img, int center_x, int center_y, int crop_h, int crop_w);

std::vector<std::pair<int, int>> find_segment_img(cv::Mat img);

std::vector<std::pair<int, int>> cut_index(std::vector<int>& min_indexs, const std::vector<float>& ver_lis);

cv::Point top_left_corner_point(cv::Mat& mat, int x, int y, int biasline);
cv::Point top_right_corner_point(cv::Mat& mat, int x, int y, int biasline);
cv::Point bottom_left_corner_point(cv::Mat& mat, int x, int y, int biasline);
cv::Point bottom_right_corner_point(cv::Mat& mat, int x, int y, int biasline);

std::pair<int, int> find_waves_by_width_amplitude(const cv::Mat& histogram_mat, int amplitude_low, int amplitude_high, int width_low, int width_high);

template<typename T>
std::vector<T> projection_y(cv::Mat binary_img);

template<class T>
void visual_point(cv::Mat img, T pointArr);

std::vector<int> findWaveTroughs(const std::vector<float>& wave);

}
}
