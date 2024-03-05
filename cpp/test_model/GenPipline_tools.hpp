#pragma once
#ifndef _GENERAL_PIPLINE_TOOLS_HPP_
#define _GENERAL_PIPLINE_TOOLS_HPP_
#include <opencv2/opencv.hpp>

// YOLO Image Detect Suite Definition

namespace GenPiplineTools {

	static cv::Mat letter_image(cv::Mat img, int hope_w, int hope_h, bool& is_horizon_pad, int& pad_val, float& resize_scale, bool if_cvtColor = false)
	{
		cv::Mat resize_img;
		int H = img.rows;
		int W = img.cols;

		// Skip resizing if hope_w or hope_h is invalid or if current dimensions match hope_h/w
		if (hope_w <= 0 || hope_h <= 0 || (H == hope_h && W == hope_w)) {
			resize_img = img;
			pad_val = 0;
		}
		else {
			float ratio_w = (float)W / (float)hope_w;
			float ratio_h = (float)H / (float)hope_h;
			if (ratio_w == ratio_h) {
				cv::resize(img, resize_img, cv::Size2i{ hope_w, hope_h });
				pad_val = 0;

			}
			else if (ratio_w > ratio_h) {
				int new_x = hope_w;
				int new_y = (int)(H / ratio_w);
				int pad1 = (int)((hope_h - new_y) / 2);
				int pad2 = hope_h - new_y - pad1;
				pad_val = (pad1 + pad2) / 2;
				is_horizon_pad = true;
				resize_scale = ratio_w;
				cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
				cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
			}
			else {
				int new_y = hope_h;
				int new_x = (int)(W / ratio_h);
				int pad1 = (int)((hope_w - new_x) / 2);
				int pad2 = hope_w - new_x - pad1;
				pad_val = (pad1 + pad2) / 2;
				is_horizon_pad = false;
				resize_scale = ratio_h;
				cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
				cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
			}
		}

		if (if_cvtColor)
			cv::cvtColor(resize_img, resize_img, cv::COLOR_BGR2RGB);

		return resize_img;
	}

	static cv::Mat letter_image(cv::Mat img, int hope_w, int hope_h, bool if_cvtColor = false)
	{
		bool is_horizon_pad_temp;
		int pad_val_temp;
		float resize_scale;
		return letter_image(img, hope_w, hope_h, is_horizon_pad_temp, pad_val_temp, resize_scale, if_cvtColor);
	}

	static inline cv::Mat safty_cut(cv::Mat& img, cv::Rect roi)
	{
		int width = roi.width;
		int height = roi.height;
		int x = roi.x;
		int y = roi.y;

		cv::Mat mat(height, width, img.type(), cv::Scalar(0));
		int _x = x;
		int _y = y;
		int _width = width;
		int _height = height;
		if (x < 0)
		{
			_x = 0;
			_width = width + x;
		}

		if (_x + _width > img.cols)
			_width = img.cols - _x;

		if (y < 0)
		{
			_y = 0;
			_height = height + y;
		}

		if (_y + _height > img.rows)
			_height = img.rows - _y;

		img(cv::Rect(_x, _y, _width, _height)).copyTo(mat(cv::Rect(_x - x, _y - y, _width, _height)));
		return mat;
	}




}

#endif //!_GENERAL_PIPLINE_TOOLS_HPP_