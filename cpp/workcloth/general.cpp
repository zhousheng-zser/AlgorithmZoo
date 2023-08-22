#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <numeric>
#include <unordered_map>
#include <algorithm>
#include "general.hpp"
#include "find_peaks.hpp"

namespace glasssix::workcloth
{

	pair_ranges_count bincount(const std::vector<float>& vec) {
		std::vector<int> rst(vec.size(), 0);
		std::vector<int> count(int(*std::max_element(vec.begin(), vec.end())) + 1, 0);
		//std::array<int, 10> count{0};
		for (int i = 0; i < vec.size(); i++) {
			rst[i] = int(vec[i]);
			count[rst[i]]++;
		}
		return { rst, count };
	}


	template<typename T>
	void print_V(std::vector<float> vec, std::string str, int H, int W, int checkVal) {
		bool check = checkVal > 0;
		int checkcount = 0;

		std::cout << str << std::endl;
		std::array<int, 9> bitcount{ 0 };
		for (int i = 0; i < H; i++) {
			for (int j = 0; j < W; j++) {
				int loc = T(vec[i * W + j]);
				if(check)
					if (loc == checkVal) {
						std::cout << loc << "  ";
						checkcount++;
					}
					else
						std::cout << " " << "  ";
				else
					std::cout << loc << "  ";

				bitcount[loc]++;
			}
			std::cout << std::endl;
		}

		std::cout << std::endl;
		if (check)
			std::cout << "checkcount: " << checkcount << std::endl;
		std::cout << "\tBitcount: ";
		for (auto b : bitcount)
			std::cout << b << ", ";
		std::cout << std::endl << std::endl;
	}

	std::array<pair_ranges_count, 3> bgr_ranges_count(cv::Mat crop, int clr_int) {
		cv::Mat crop_float;
		crop.convertTo(crop_float, CV_32FC3);
		crop_float = crop_float / float(clr_int);
		std::vector<cv::Mat> mv;
		split(crop_float, mv);
		std::vector<float> B = mv[0].reshape(1, 1);
		std::vector<float> G = mv[1].reshape(1, 1);
		std::vector<float> R = mv[2].reshape(1, 1);
#ifdef BUILD_DEBUG_INFO
		//print_V<int>(B, "B", crop.rows, crop.cols);
		//print_V<int>(G, "G", crop.rows, crop.cols);
		//print_V<int>(R, "R", crop.rows, crop.cols);

		auto [b_ranges, b_count] = bincount(B);
		auto [g_ranges, g_count] = bincount(G);
		auto [r_ranges, r_count] = bincount(R);
#endif
		return{ bincount(B) ,bincount(G) ,bincount(R) };
	}

	std::pair<bool, std::array<int, 3>> extract_rgb(cv::Mat person, cv::Rect crotch, int clr_int, int peak_thres, int peak_distance) {
		int area = crotch.area();
		cv::Mat crop = person(crotch).clone();

#ifdef BUILD_DEBUG_INFO
		cv::Mat cck = crop.clone();
		cv::resize(cck, cck, cv::Size{}, 5, 5);
		//cv::imshow("crop", cck); cv::waitKey(0);
#endif
		auto bgr_stc = bgr_ranges_count(crop, clr_int);
		auto [b_ranges, b_count] = bgr_stc[0];
		auto [g_ranges, g_count] = bgr_stc[1];
		auto [r_ranges, r_count] = bgr_stc[2];

#ifdef BUILD_DEBUG_INFO
		//for (int c = 0; c < 3; c++) {
		//	auto peakIndx = findPeaks(bgr_stc[c].second, { area / peak_thres, area }, peak_distance);
		//	std::cout << c <<" peakIndx: ";
		//	for (auto b : peakIndx)
		//		std::cout << b << ", ";
		//	std::cout << std::endl << std::endl;
		//}
#endif

		auto b_peaks = findPeaks(b_count, { area / peak_thres, area }, peak_distance);
		auto g_peaks = findPeaks(g_count, { area / peak_thres, area }, peak_distance);
		auto r_peaks = findPeaks(r_count, { area / peak_thres, area }, peak_distance);

		auto bgr_more = bgr_ranges_count(crop, 10);

		auto [b_ranges_m, b_count_m] = bgr_more[0];
		auto [g_ranges_m, g_count_m] = bgr_more[1];
		auto [r_ranges_m, r_count_m] = bgr_more[2];
		auto bgr_bind = [](std::vector<int> _count, int mul)->int {
			auto biggest = std::max_element(_count.begin(), _count.end());
			return std::distance(_count.begin(), biggest) * mul;
		};

		bool _strange = false;
		std::array<int, 3> O_BGR;
		if (b_peaks.size() <= 1 && g_peaks.size() <= 1 && r_peaks.size() <= 1) {
			O_BGR = { bgr_bind(b_count_m,10),bgr_bind(g_count_m,10),bgr_bind(r_count_m,10) };
		}
		else {
			_strange = true;
			//O_BGR = { -10,-10,-10 };
			O_BGR = { bgr_bind(b_count_m,10),bgr_bind(g_count_m,10),bgr_bind(r_count_m,10) };
		}
		return { _strange, O_BGR }; //_strange == false Equal wearing == true
	}

}
