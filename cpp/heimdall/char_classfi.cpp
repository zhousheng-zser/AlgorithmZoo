#include "char_classfi.hpp"
#include <Primitives/tensor_conversions.hpp>
namespace glasssix
{
	namespace heimdall
	{
		const static char cool_roll_label_index[] = { '0', '1','2','3','4','5','6','7','8','9','A','C','I' };
		const static char heavy_rail_label_index[] = { '0', '1','2','3','4','5','6','7','8','9','A','B','C','G','P','X','E' };
		char_classfi::char_classfi(label_type lt)
		{
			switch (lt)
			{
			case glasssix::heimdall::label_type::COOL_ROLL:
				label_index_ = cool_roll_label_index;
				break;
			case glasssix::heimdall::label_type::HEAVY_RAIL:
				label_index_ = heavy_rail_label_index;
				break;
			default:
				break;
			}
		}
		std::pair<char, float> char_classfi::detect(cv::Mat& img, excalibur::pipeline<float>& classfi_instance)
		{
			cv::Mat pre_img = pre_handel_img(img);
			auto input_img = std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{1, 64, 48, 3}, -1, memory::NHWC);
			std::copy(pre_img.data, pre_img.data + pre_img.step[0] * pre_img.rows, input_img->mutable_cpu_data());
			input_img->convert_order();
			auto result = classfi_instance.forward(input_img | memory::tensor_convert_to<float>);
			std::vector<float> detections(result["output"]->cpu_data(), result["output"]->cpu_data() + result["output"]->count());
			auto biggest_index = std::distance(detections.begin(), std::max_element(detections.begin(), detections.end()));
			return { label_index_[biggest_index], detections[biggest_index] };
		}
		cv::Mat char_classfi::pre_handel_img(cv::Mat& img) {
			int H = img.rows;
			int W = img.cols;
			float ratio_w = (float)W / 48.0;
			float ratio_h = (float)H / 64.0;
			cv::Mat resize_img;
			if (ratio_w == ratio_h)
			{
				cv::resize(img, resize_img, cv::Size2i{ 48, 64 });
			}
			else if (ratio_w > ratio_h) {
				int new_x = 48;
				int new_y = (int)(H / ratio_w);
				int pad1 = (int)((64 - new_y) / 2);
				int pad2 = 64 - new_y - pad1;
				cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
				cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
			}
			else {
				int new_y = 64;
				int new_x = (int)(W / ratio_h);
				int pad1 = (int)((48 - new_x) / 2);
				int pad2 = 48 - new_x - pad1;
				cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
				cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
			}
			return resize_img;
		}
	}
}
