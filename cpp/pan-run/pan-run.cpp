// longinus-run.h: 目标的头文件。
#pragma once

#include <abi/consumer.hpp>
#include <opencv2/opencv.hpp>

#include "pan/yolov5Deepsort_net.hpp"

#include "utils.hpp"


int main()
{
	int a = 0;
	try
	{
		bool isOk = glasssix::exposing::get_component_loader().add_module_by_name("pan");

		auto yolov5deepsort_object = glasssix::exposing::make_exported_interface<glasssix::pan::yolov5Deepsort_net>(
			u8"D:/Desktop/onnxs/m_0214.phai",
			u8"D:/Desktop/onnxs/m_0214.racy",
			u8"D:/Desktop/onnxs/deepsort.phai",
			u8"D:/Desktop/onnxs/deepsort.racy",
			0);

		cv::Mat img = cv::imread("D:/Desktop/111.png");

		glasssix::exposing::param_span<std::uint8_t> img_span(img.data, img.step[0] * img.rows);

		auto ret = yolov5deepsort_object.detect(img_span, img.channels(), img.rows, img.cols, 1);

		
		auto size = ret.conf().size();
		
		std::vector<Detection> detections;
		for (int i = 0; i < size; ++i) {
			auto coordinate = ret.coordinates()[i];
			auto conf = ret.conf()[i];
			auto cls = ret.cls()[i];

			detections.push_back(Detection{ cv::Rect(coordinate[0], coordinate[1], coordinate[2]-coordinate[0], coordinate[3]-coordinate[1]), conf, cls.data() });
		}

		utils::visualizeDetection(img, detections);

		cv::imshow("img", img);
		cv::waitKey(0);
		cv::destroyAllWindows();
		
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return 0;
}