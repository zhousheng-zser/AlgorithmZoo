#pragma once
#include<opencv2/core.hpp>
#include<opencv2/opencv.hpp>
#include<vector>
#include<utility>
#include <Excalibur/pipeline.hpp>
namespace glasssix
{
	namespace heimdall
	{
		class char_classfi
		{
		public:
			char_classfi();
			std::pair<char, float> detect(cv::Mat& img, excalibur::pipeline<float>& classfi_instance);
		private:
			cv::Mat pre_handel_img(cv::Mat& img);
		};
	}
}
