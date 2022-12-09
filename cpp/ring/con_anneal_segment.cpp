#include "con_anneal_segment.hpp"
#include "savitzky_golay_fliter.hpp"
#include <algorithm>
namespace glasssix
{
namespace ring
{
std::vector<cv::Point2f> findFoot(const cv::Mat& img)
{
	cv::Mat gray_mat;
	cv::Mat canny_mat;
	int lowThreshold = 50;
	int maxThreshold = 100;
	int kernel_size = 3;
	cvtColor(img, gray_mat, CV_BGR2GRAY);
	cv::Canny(gray_mat, canny_mat, lowThreshold, maxThreshold, kernel_size, true);

	std::vector<cv::Point2f> foot_points;
	int W = canny_mat.cols;
	int H = canny_mat.rows;
	int bias = (W + H) / 4; //bias for expanding search region. value : 0 ~ (W + H) / 4
	foot_points.push_back(top_left_corner_point(canny_mat, 0, 0, bias));
	foot_points.push_back(top_right_corner_point(canny_mat, W - 1, 0, bias));
	foot_points.push_back(bottom_right_corner_point(canny_mat, W - 1, H - 1, bias));
	foot_points.push_back(bottom_left_corner_point(canny_mat, 0, H - 1, bias));

	return foot_points;
}

bool redirectRect(cv::Mat& img)
{
	bool img_validity = true;
	cv::Mat plate_gray;
	cv::Mat plate_binary_img;
	cv::cvtColor(img, plate_gray, CV_BGR2GRAY);
	cv::adaptiveThreshold(plate_gray, plate_binary_img, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 333, -6);
	plate_binary_img = cv::Mat(plate_binary_img, cv::Rect(25, 25, 590, 590));

	cv::Mat histogram_vertical;
	cv::Mat histogram_horizon;
	cv::reduce(plate_binary_img, histogram_vertical, 1, cv::REDUCE_SUM, CV_32SC1);
	cv::reduce(plate_binary_img, histogram_horizon, 0, cv::REDUCE_SUM, CV_32SC1);
	auto waves_vertical = find_waves_by_width_amplitude(histogram_vertical, 146000, 200000, 60, 100);
	auto waves_horizon = find_waves_by_width_amplitude(histogram_horizon, 146000, 200000, 60, 100);

	if (waves_vertical.first != -1 && waves_horizon.first == -1)
	{
		if (std::min(waves_vertical.first, waves_vertical.second) > 320)
		{
			cv::rotate(img, img, cv::ROTATE_180);
		}
	}
	else if (waves_vertical.first == -1 && waves_horizon.first != -1)
	{
		if (std::max(waves_horizon.first, waves_horizon.second) < 320)
		{
			cv::rotate(img, img, cv::ROTATE_90_CLOCKWISE);
		}
		else if (std::min(waves_horizon.first, waves_horizon.second) > 320)
		{
			cv::rotate(img, img, cv::ROTATE_90_COUNTERCLOCKWISE);
		}
	}
	else
	{
		img_validity = false;
	}

	return img_validity;
}

cv::Mat charBoxDet(const cv::Mat& img, int center_x = 280, int center_y = 360, int crop_h = 94, int crop_w = 421)
{
	cv::Mat out;
	cv::Mat input = img.clone();
	int roi_x = center_x - crop_w / 2;
	int roi_y = center_y - crop_h / 2;

	out = cv::Mat(input, cv::Rect(roi_x, roi_y, crop_w, crop_h));
	return out;
}

std::vector<std::pair<int, int>> find_segment_img(cv::Mat img)
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

	return cut_index(min_indexs, ver_list);
}

std::vector<std::pair<int, int>> cut_index(std::vector<int>& min_indexs, const std::vector<float>& ver_list)
{
	std::vector<std::pair<int, int>> cord_list;
	std::vector<int> del_list;

	for (int i = 0; i < min_indexs.size() - 1; ++i)
	{
		if (ver_list[min_indexs[i]] > 15)
		{
			del_list.push_back(i);
		}
	}
	for (int i = 0; i < del_list.size(); ++i) {
		min_indexs.erase(min_indexs.begin() + (del_list[i] - i));
	}

	int start = 0;
	while (start < min_indexs.size() - 1)
	{
		for (int end = start + 1; end < min_indexs.size(); ++end)
		{
			if (min_indexs[end] - min_indexs[start] > 20)
			{
				cord_list.push_back({ min_indexs[start], min_indexs[end] });
				start = end;
				break;
			}
			else if (end == min_indexs.size() - 1)
			{
				if (min_indexs[end] - min_indexs[start] > 5)
				{
					cord_list.push_back({ min_indexs[start], min_indexs[end] });
				}
				++start;
			}
		}
	}

	return cord_list;
}


cv::Point top_left_corner_point(cv::Mat& mat, int x, int y, int biasline)
{
	cv::Point point{ 0, 0 };
	int i = 1, j = 0;
	while (i < mat.rows / 2 + biasline && j < mat.cols / 2 + biasline)
	{
		j = 0;
		while (i >= 0)
		{
			if (mat.at<uchar>(y + i, x + j) == 255)
			{
				point.x = x + j;
				point.y = y + i;
				return point;
			}
			else
			{
				--i;
				++j;
			}
		}

		i = 0;

		while (j >= 0)
		{
			if (mat.at<uchar>(y + i, x + j) == 255)
			{
				point.x = x + j;
				point.y = y + i;
				return point;
			}
			else
			{
				++i;
				--j;
			}
		}
	}
	return point;
}

cv::Point top_right_corner_point(cv::Mat& mat, int x, int y, int biasline)
{
	cv::Point point{ 0, 0 };
	int i = 1, j = 0;
	while (i < mat.rows / 2 + biasline && j >(-mat.cols) / 2 - biasline)
	{
		j = 0;
		while (i >= 0)
		{
			if (mat.at<uchar>(y + i, x + j) == 255)
			{
				point.x = x + j;
				point.y = y + i;
				return point;
			}
			else
			{
				--i;
				--j;
			}
		}

		i = 0;

		while (j <= 0)
		{
			if (mat.at<uchar>(y + i, x + j) == 255)
			{
				point.x = x + j;
				point.y = y + i;
				return point;
			}
			else
			{
				++i;
				++j;
			}
		}
	}
	return point;
}

cv::Point bottom_left_corner_point(cv::Mat& mat, int x, int y, int biasline)
{
	cv::Point point{ 0, 0 };
	int i = -1, j = 0;
	while (i > -mat.rows / 2 - biasline && j < mat.cols / 2 + biasline)
	{
		j = 0;
		while (i <= 0)
		{
			if (mat.at<uchar>(y + i, x + j) == 255)
			{
				point.x = x + j;
				point.y = y + i;
				return point;
			}
			else
			{
				++i;
				++j;
			}
		}

		i = 0;

		while (j >= 0)
		{
			if (mat.at<uchar>(y + i, x + j) == 255)
			{
				point.x = x + j;
				point.y = y + i;
				return point;
			}
			else
			{
				--i;
				--j;
			}
		}
	}
	return point;
}

cv::Point bottom_right_corner_point(cv::Mat& mat, int x, int y, int biasline)
{
	cv::Point point{ 0, 0 };
	int i = -1, j = 0;
	while (i > -mat.rows / 2 - biasline && j > -mat.cols / 2 - biasline)
	{
		j = 0;
		while (i <= 0)
		{
			if (mat.at<uchar>(y + i, x + j) == 255)
			{
				point.x = x + j;
				point.y = y + i;
				return point;
			}
			else
			{
				++i;
				--j;
			}
		}

		i = 0;

		while (j <= 0)
		{
			if (mat.at<uchar>(y + i, x + j) == 255)
			{
				point.x = x + j;
				point.y = y + i;
				return point;
			}
			else
			{
				--i;
				++j;
			}
		}
	}
	return point;
}

template<class T>
void visual_point(const cv::Mat& img, T pointArr)
{
	cv::Mat viusalmat = img.clone();
	for (auto& point : pointArr)
		cv::circle(viusalmat, cv::Point(point.x, point.y), 3, cv::Scalar(255, 0, 0), 3);
	cv::imshow("visual_point", viusalmat); cv::waitKey(0);
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