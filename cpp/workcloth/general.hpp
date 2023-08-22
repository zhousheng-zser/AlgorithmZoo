#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <numeric>
#include <unordered_map>
#include <algorithm>

namespace glasssix::workcloth
{
	using pair_ranges_count = std::pair<std::vector<int>, std::vector<int>>;
	pair_ranges_count bincount(const std::vector<float>& vec);

	std::array<pair_ranges_count, 3> bgr_ranges_count(cv::Mat crop, int clr_int);

	/// <summary>
	/// 
	/// </summary>
	/// <param name="person"></param>
	/// <param name="crotch"></param>
	/// <param name="clr_int"></param>
	/// <param name="peak_thres"></param>
	/// <param name="peak_distance"></param>
	/// <returns>cloth_strange, bgr</returns>
	std::pair<bool, std::array<int, 3>> extract_rgb(cv::Mat person, cv::Rect crotch, int clr_int = 25, int peak_thres = 8, int peak_distance = 3);


}
