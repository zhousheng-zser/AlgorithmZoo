#include "con_anneal_segment.hpp"
#include "savitzky_golay_fliter.hpp"
#include <algorithm>
#include <numeric>
#include <math.h>
#include <opencv2/imgproc/types_c.h>
namespace glasssix
{
namespace ring
{
std::vector<std::pair<int, int>> find_segment_img(cv::Mat& img)
{
	cv::Mat plate_gray;
	cv::Mat plate_binary_img;
	cv::cvtColor(img, plate_gray, CV_BGR2GRAY);
	cv::adaptiveThreshold(plate_gray, plate_binary_img, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 45, 6);
	auto ver_list = projection_y<float>(plate_binary_img);

	std::vector<float> savgol_order2;
	const int windowSize = 19;
	savgol(ver_list.begin(), ver_list.end(), std::back_inserter(savgol_order2), SmoothQuadCubic(windowSize));
	std::vector<int> min_indexs = findWaveTroughs(savgol_order2);
	//// visualize wave troughs or peaks
	//polynomial_curve_show(savgol_order2, min_indexs);

	return cut_index(min_indexs, ver_list, 15);
}

std::vector<std::pair<int, int>> cut_index(std::vector<int>& min_indexs, const std::vector<float>& ver_list,int seg_width)
{
	std::vector<std::pair<int, int>> cord_list;
	std::vector<int> del_list;

	for (int i = 0; i < min_indexs.size() - 1; ++i)
	{
		if (ver_list[min_indexs[i]] > 10)
		{
			del_list.push_back(i);
		}
	}
	for (int i = 0; i < del_list.size(); ++i) {
		min_indexs.erase(min_indexs.begin() + (del_list[i] - i));
	}

	std::vector<int> width_list;
	for (auto i = 5; i < min_indexs.size() - 1; ++i) {
		int width = min_indexs[i] - min_indexs[i - 1];
		if (width > 25) {
			width_list.push_back(width);
		}
	}
	float valid_mean_width = width_list.empty() ? 0 : std::accumulate(width_list.begin(), width_list.end(), 0.0) / width_list.size();

	int start = 0;
	while (start < min_indexs.size() - 1)
	{
		for (int end = start + 1; end < min_indexs.size(); ++end)
		{
			if (min_indexs[end] - min_indexs[start] > valid_mean_width - seg_width || end == min_indexs.size() - 1)
			{
				cord_list.push_back({ min_indexs[start], min_indexs[end] });
				start = end;
				break;
			}
		}
	}

	return cord_list;
}

std::pair<int, int> find_waves_by_width_amplitude(const cv::Mat& histogram_mat, int amplitude_low = 50000, int amplitude_high = 70000, int width_low = 50, int width_high = 100)
{
	std::vector<int32_t> histogram;
	const int tail_index = histogram_mat.total() - 1;
	std::vector<std::pair<int, int>> wave_peaks;
	int start_point = -1; //rising point 
	bool is_peak = false;
	histogram.assign((int32_t*)histogram_mat.data, (int32_t*)histogram_mat.data + histogram_mat.total());

	if (histogram[0] >= amplitude_low && histogram[0] <= amplitude_high)
	{
		start_point = 0;
		is_peak = true;
	}

	for (int i = 0; i < histogram.size(); ++i)
	{
		int x = histogram[i];
		if (is_peak && (x <= amplitude_low || x >= amplitude_high))
		{
			if (i - start_point > 2)
			{
				is_peak = false;
				wave_peaks.push_back({ start_point, i });
			}
		}
		else if (!is_peak && x >= amplitude_low && x <= amplitude_high)
		{
			is_peak = true;
			start_point = i;
		}
	}

	if (is_peak && start_point != -1 && tail_index - start_point + 1 > 4)
	{
		wave_peaks.push_back({ start_point, tail_index });
	}

	std::pair<int, int> selected_wave{ -1,-1 };
	for (auto wave_peak : wave_peaks)
	{
		int width = wave_peak.second - wave_peak.first;
		if (width >= width_low && width <= width_high)
		{
			selected_wave = wave_peak;
		}
	}

	return selected_wave;
}

template<typename T>
std::vector<T> projection_y(cv::Mat binary_img)
{
	std::vector<T> ver_list;
	cv::Mat histogram_horizon;
	int max_blank_number = binary_img.rows;
	int char_pixel_thresh = 1;

	cv::reduce(binary_img, histogram_horizon, 0, cv::REDUCE_SUM, CV_32SC1);
	histogram_horizon = histogram_horizon / 255;

	ver_list.assign((int32_t*)histogram_horizon.data, (int32_t*)histogram_horizon.data + histogram_horizon.total());
	for (auto iter = ver_list.begin(); iter != ver_list.end(); iter++)
	{
		if (*iter >= max_blank_number - char_pixel_thresh) {
			*iter = 0;  //inverse, count char pixel num
		}
		else
		{
			*iter = max_blank_number - *iter; //inverse, count char pixel num
		}
	}
	//std::cout << "projection_y:\n" << histogram_horizon << std::endl;

	return ver_list;
}

std::vector<int> findWaveTroughs(const std::vector<float>& wave) {
	std::vector<int> min_indexs;

	std::vector<int> diff_v(wave.size() - 1, 0);
	// First Difference get trend
	for (std::vector<int>::size_type i = 0; i != diff_v.size(); i++)
	{
		if (wave[i + 1] - wave[i] > 0)
			diff_v[i] = 1;
		else if (wave[i + 1] - wave[i] < 0)
			diff_v[i] = -1;
		else
			diff_v[i] = 0;
	}
	// reset trend
	for (int i = diff_v.size() - 1; i >= 0; i--)
	{
		if (diff_v[i] == 0 && i == diff_v.size() - 1)
		{
			diff_v[i] = 1;
		}
		else if (diff_v[i] == 0)
		{
			if (diff_v[i + 1] >= 0)
				diff_v[i] = 1;
			else
				diff_v[i] = -1;
		}
	}

	// Second Difference find peak(-2) or trough(+2)
	const int trough = 2;
	const int peak = -2;
	if (diff_v[1] - diff_v[0] == trough)
	{
		min_indexs.push_back(1);
	}
	for (std::vector<int>::size_type i = 1; i != diff_v.size() - 1; i++)
	{
		if (diff_v[i + 1] - diff_v[i] == trough && !((diff_v[i - 1] * diff_v[i]) < 0 && (diff_v[i] * diff_v[i + 1]) < 0)) // trough(peak) && not shake
			min_indexs.push_back(i + 1);
	}

	return min_indexs;
}

}
}