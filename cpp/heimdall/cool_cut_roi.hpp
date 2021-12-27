#pragma once
#include<vector>
#include<opencv2/opencv.hpp>

namespace glasssix
{
	namespace heimdall
	{
		struct cordinate_roi {
			std::vector<std::vector<cv::Point2f>> cordinate;
			std::vector<cv::Mat> rois;
			std::vector<float> max_R;
		};
		class cut_reg_roi {
		public:
			cut_reg_roi(int threshold);
			cordinate_roi cut_roi_gather(std::vector<std::vector<cv::Point2f>> pre_rois, cv::Mat& img);
			bool  triangleCircle(const cv::Point2f& p1, const cv::Point2f& p2, const cv::Point2f& p3, cv::Point2f& center, double& radius);
		private:
			bool isThreePointsOnOneLine(const cv::Point2f& p1, const cv::Point2f& p2, const cv::Point2f& p3);
			void out_inner_turn(std::vector<cv::Point2f>& singel_point);
			float getAngelOfTwoVector(cv::Point2f& pt1, cv::Point2f& pt2, cv::Point2f& c);
			std::vector<std::vector<cv::Point2f>> get_coordinate_order(std::vector<std::vector<cv::Point2f>>& cordi, cv::Mat& img, const int& threshold);
			std::pair<cv::Point2f, float> get_start_end(cv::Point2f& center, float& R_iner, float& R_out, cv::Point2f& point_inner, cv::Point2f& point_out, bool is_start);
			float get_radian(cv::Point2f& center, cv::Point2f& point_cor);
			cv::Mat cut_roi_region(cv::Mat& img, cv::Point2f& center, cv::Point2f& start_point, cv::Point2f& end_point, float& start_theta, float& end_theta, float& R_out, float& R_inner);
			std::pair<std::vector<cv::Mat>, std::vector<float>> handel_img(cv::Mat& img, std::vector<std::vector<cv::Point2f>>& bbox);
			int threshold_;
		};
	}
}
