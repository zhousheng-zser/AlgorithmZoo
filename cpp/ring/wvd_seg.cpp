
#include "wvd_seg.hpp"
#include <vector>
#include <iostream>
//#include <io.h>

namespace glasssix
{
namespace ring
{

cv::Mat angular_rotation(cv::Mat& img, float angle1) {
	cv::Mat result_img;
	int w = img.cols;
	int h = img.rows;
	float w1 = 0.5 * h / std::tan(std::abs(angle1));
	w1 = angle1 >= 0 ? w1 : -1 * w1;
	std::array<cv::Point2f, 4> src_points{ cv::Point2f(0,0),cv::Point2f(w, 0),cv::Point2f(w, h),cv::Point2f(0, h) };
	std::array<cv::Point2f, 4> dst_points{ cv::Point2f(w1, 0),cv::Point2f(w + w1, 0),cv::Point2f(w - w1, h),cv::Point2f(-w1, h) };
	cv::Mat TransMat = cv::getPerspectiveTransform(src_points, dst_points);
	cv::warpPerspective(img, result_img, TransMat, cv::Size{ w, h }, cv::INTER_LINEAR);
	return result_img;
}

std::vector<int> statistic_projection_y(cv::Mat binary_img)
{
	std::vector<int> ver_list;
	int width = binary_img.cols;
	ver_list.reserve(width);
	for (int i = 0; i < width; i++) {
		auto yline = binary_img.col(i);
		ver_list.push_back(cv::countNonZero(yline));
	}
	return ver_list;
}

float slant_text_angle(cv::Mat binary_image, int angle_base, int angle_range, int angle_step) {
	float best_angle = 0.f;
	float best_denisty = 0.f;
	for (int angle = angle_base - angle_range; angle < angle_base + angle_range; angle += angle_step) {
		float ang = 3.1415926 * (90 - (float)angle) / 180;
		int w = binary_image.cols;
		int h = binary_image.rows;
		auto dst = angular_rotation(binary_image, ang);

		std::vector<int> ver_list = statistic_projection_y(dst);
		std::vector<std::complex<float>> sn(ver_list.size(), { 0,0 });
		for (int i = 0; i < ver_list.size(); i++) {
			sn[i] = ver_list[i];
		}

		WVD wvd(sn);
		auto tfr = wvd.compute(false);
		float MatrixSum = 0;
		for (auto fline : tfr) {
			for (auto val : fline)
				MatrixSum += val.real();
		}
		if (MatrixSum > best_denisty) {
			best_denisty = MatrixSum;
			best_angle = angle;
			////YHC
			//std::cout << "* BEST_TXT_ANGLE_ " << angle << "_SUM_" << MatrixSum / 1000000 << std::endl;
			//cv::imshow("* BEST_TXT_ANGLE" + std::to_string(angle) + "_SUM_" + std::to_string(MatrixSum / 1000000), dst); cv::waitKey(0);

		}
		//YHC
		//else {
		//	std::cout << "  slan_txt_angle_ " << angle << "_SUM_" << MatrixSum / 1000000 << std::endl;
		//	cv::imshow("slan_txt_angle_" + std::to_string(angle) + "_SUM_" + std::to_string(MatrixSum / 1000000), dst); cv::waitKey(0);
		//}
	}
	return best_angle;
}

cv::Mat slant_text_correction(cv::Mat& image) {
	cv::Mat img_gray;
	cv::Mat binary;
	cv::cvtColor(image, img_gray, cv::COLOR_BGR2GRAY);
	cv::adaptiveThreshold(img_gray, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 71, 2);
#ifdef BUILD_DEBUG_INFO
	//cv::imshow("binary", binary); cv::waitKey(0);
#endif
	auto best_angle = slant_text_angle(binary, 1, 45, 10);
	auto best_angle1 = slant_text_angle(binary, best_angle, 10, 1);

	float last_angle = 3.1415926 * (90 - best_angle1) / 180;
	auto corrected_image = angular_rotation(image, last_angle);
	return corrected_image;
}

} // ring
} // glasssix
