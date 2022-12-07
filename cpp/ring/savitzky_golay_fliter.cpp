#include "savitzky_golay_fliter.hpp"
#include <algorithm>

namespace glasssix
{
namespace ring
{

void polynomial_curve_show(std::vector<float>& savgol_order2, std::vector<int>& min_indexs) {
	constexpr int show_height = 300;
	constexpr int x_stretch = 2;
	cv::Mat image = cv::Mat::zeros(show_height, savgol_order2.size() * 2, CV_8UC3);
	image.setTo(cv::Scalar(100, 0, 0));
	// input wave
	std::vector<cv::Point> points;
	for (int i = 0; i < savgol_order2.size(); ++i) {
		points.push_back(cv::Point(i * x_stretch, show_height - savgol_order2[i] * 5 - 20));
	}
	// trough
	for (auto it : min_indexs) {
		cv::line(image, cv::Point(it * x_stretch, show_height), cv::Point(it * x_stretch, show_height - 200), cv::Scalar(0, 0, 250), 1);
	}
	cv::polylines(image, points, false, cv::Scalar(0, 255, 0), 1, 8, 0);
	int check_h = show_height - 13 * 5 - 20;
	cv::line(image, cv::Point(10, check_h), cv::Point(savgol_order2.size() * 2 - 10, check_h), cv::Scalar(0, 0, 250), 1);
	cv::imshow("wave find", image);	//cv::waitKey(0);
}

}
}