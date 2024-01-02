#pragma once
#include<vector>
#include<opencv2/opencv.hpp>

namespace glasssix::playphone
{

	// letterbox
	static inline cv::Mat letterbox(cv::Mat img, int hope_w = 640, int hope_h = 640)
	{
		int H = img.rows;
		int W = img.cols;
		float ratio_w = (float)W / (float)hope_w;
		float ratio_h = (float)H / (float)hope_h;
		cv::Mat resize_img;
		if (ratio_w == ratio_h)
		{
			cv::resize(img, resize_img, cv::Size2i{ hope_w, hope_h });
		}
		else if (ratio_w > ratio_h)
		{
			int new_x = hope_w;
			int new_y = (int)(H / ratio_w);
			int pad1 = (int)((hope_h - new_y) / 2);
			int pad2 = hope_h - new_y - pad1;
			cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
			cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
		}
		else
		{
			int new_y = hope_h;
			int new_x = (int)(W / ratio_h);
			int pad1 = (int)((hope_w - new_x) / 2);
			int pad2 = hope_w - new_x - pad1;
			cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
			cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
		}
		return resize_img;
	}

	static inline cv::Mat playphone_preprocess(cv::Mat img, int img_size) {

		cv::Mat md_img;
		cv::Mat hsv_image;
		cv::Mat black_mask;
		cv::cvtColor(img, hsv_image, cv::COLOR_BGR2HSV);
		const cv::Scalar lower_black_ = cv::Scalar{ 0, 0, 0 };
		const cv::Scalar upper_black_ = cv::Scalar{ 180, 255, 60 };
		cv::inRange(hsv_image, lower_black_, upper_black_, black_mask);

		for (int row = 0; row < black_mask.rows; ++row)
		{
			for (int col = 0; col < black_mask.cols; ++col)
			{
				auto& hsv = hsv_image.at<cv::Vec3b>(row, col);
				if (black_mask.at<uchar>(row, col) > 0 || black_mask.at<uchar>(row, col) > 0)
				{
					hsv = { 0, 0, 65 };
				}
			}
		}
		cv::cvtColor(hsv_image, md_img, cv::COLOR_HSV2RGB);// is RGB!
		cv::Mat letter_img = letterbox(md_img, img_size, img_size);
		return letter_img;
	}

}
