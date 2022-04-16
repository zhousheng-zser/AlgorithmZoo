// longinus-run.h: 目标的头文件。
#pragma once

#include <abi/consumer.hpp>
#include <opencv2/opencv.hpp>

#include "longinus/retina_net.hpp"

int main()
{
	int a = 0;
	try
	{
		bool ret = glasssix::exposing::get_component_loader().add_module_by_name("longinus");

		auto longinus_object = glasssix::exposing::make_exported_interface<glasssix::longinus::retina_net>(
			u8"C:/Users/Glasssix-LYL/Desktop/models/longinus.racy",
			u8"C:/Users/Glasssix-LYL/Desktop/models/pfld_land71_simp.racy",
			0.4f, -1);

		cv::Mat img = cv::imread("C:/Users/Glasssix-LYL/Desktop/test.jpg");
		glasssix::exposing::param_span<std::uint8_t> img_span(img.data, img.step[0] * img.rows);
		auto result = longinus_object.detect(img_span, img.channels(), img.rows, img.cols, 16, 0.5f, 1, true);

		for (auto& x : result)
		{
			std::cout << "x: " << x.x() << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return 0;
}