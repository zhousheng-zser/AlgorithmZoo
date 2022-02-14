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
		enum class label_type
		{
			COOL_ROLL,
			HEAVY_RAIL
		};
		class char_classfi
		{
		public:
			char_classfi(label_type lt);
			std::pair<char, float> detect(cv::Mat& img, excalibur::pipeline<float>& classfi_instance);
		private:
			cv::Mat pre_handel_img(cv::Mat& img);
			const char* label_index_;
		};
	}
}
