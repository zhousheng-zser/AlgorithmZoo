#pragma once
#include<vector>
#include<opencv2/opencv.hpp>

namespace glasssix
{
namespace ring
{
std::vector<std::pair<int, int>> find_segment_img(cv::Mat& img);

std::vector<std::pair<int, int>> cut_index(std::vector<int>& min_indexs, const std::vector<float>& ver_lis);

std::pair<int, int> find_waves_by_width_amplitude(const cv::Mat& histogram_mat, int amplitude_low, int amplitude_high, int width_low, int width_high);

template<typename T>
std::vector<T> projection_y(cv::Mat binary_img);

std::vector<int> findWaveTroughs(const std::vector<float>& wave);

}
}
