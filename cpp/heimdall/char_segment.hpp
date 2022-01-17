#pragma once
#include<opencv2/core.hpp>
#include<opencv2/opencv.hpp>
#include<vector>
#include <string_view>
#include <Excalibur/pipeline.hpp>

namespace glasssix
{
	namespace heimdall
	{
		class char_segment
		{
		public:
			char_segment(float iou_thres, float delet_iou_thres, int stride, bool add_segement);
			char_segment();
			std::vector<float> detect(cv::Mat& img, excalibur::pipeline<float>& segement_instance);
		private:
			cv::Mat pre_handel_img(cv::Mat& img, int& stride);
			float get_bbox_iou(cv::Point2f& point1, cv::Point2f& point2);
			void keep_index_vector(std::vector<cv::Point2f>& trans_bbox, std::vector<int>& retain_index);
			void keep_index_vector_pair(std::vector<std::pair<cv::Point2f, int>>& merge_bbox, std::vector<int>& retain_index);
			std::vector<std::pair<cv::Point2f, int>> get_merge_box(std::vector<cv::Point2f> trans_detections, float& thres);
			std::vector<std::pair<cv::Point2f, int>> delet_error_bbox(std::vector<std::pair<cv::Point2f, int>>& merge_bbox, const float& num_thres);
			std::vector<float> merge_near_box(std::vector<std::pair<cv::Point2f, int>>& box);
			float iou_thres_;
			float delet_iou_thres_;
			int stride_;
			bool add_segement_;
		};
	}
}

