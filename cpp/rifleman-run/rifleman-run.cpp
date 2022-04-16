// rifleman-run.cpp: 目标的源文件。

#include <opencv2/opencv.hpp>
#include <abi/consumer.hpp>

#include "rifleman-run.hpp"
#include "rifleman/deepMarMobileNet_net.hpp"

int main()
{
	int a = 0;
	try
	{
		bool isOk = glasssix::exposing::get_component_loader().add_module_by_name("rifleman");

		auto deepMarMobileNet_object = glasssix::exposing::make_exported_interface<glasssix::rifleman::deepMarMobileNet_net>(
			u8"D:/Desktop/onnxs/DeepMAR_MobileNet.phai",
			u8"D:/Desktop/onnxs/DeepMAR_MobileNet.racy",
			0);

		cv::Mat img = cv::imread("D:/Desktop/000030.jpg");
		cv::cvtColor(img, img, cv::COLOR_BGR2RGB);

		assert(img.empty() == false);

		glasssix::exposing::param_span<std::uint8_t> img_span(img.data, img.step[0] * img.rows);

		auto rets = deepMarMobileNet_object.detect(img_span, img.channels(), img.rows, img.cols, 1);

		for (auto per_ret : rets)
		{
			std::cout << "------------- per ret ---------------" << std::endl;
			
			for (auto record : per_ret) {
				std::cout << record.key() << ":" << record.value() << std::endl;
			}
		}

		cv::waitKey(0);
		cv::destroyAllWindows();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return 0;
}