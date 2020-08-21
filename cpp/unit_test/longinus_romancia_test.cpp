#include "CppUnitTest.h"
#include "../longinus/retina_net.hpp"
#include "../romancia/face_alignment.hpp"
#include "../common/include/Primitives/tensor.hpp"

#include <random>
#include <algorithm>
#include <filesystem>

#include <abi/consumer.hpp>

#include <opencv2/opencv.hpp>

using namespace glasssix;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace unittest
{
	namespace
	{
		template <typename Dtype>
		static void mat2tensor_cpu(const cv::Mat& srcu, std::shared_ptr<memory::tensor<Dtype>>& dst, memory::orderType order = memory::NCHW, bool bgr2rgb = false)
		{
			if (srcu.data == NULL)
			{
				LOG(ERROR) << "No data.";
				return;
			}

			int channels = srcu.channels();
			int width = srcu.cols;
			int height = srcu.rows;
			/*int type_id = src.type() % 8;
			auto type_name = std::string(typeid(Dtype).name());*/

			if (order == memory::NCHW)
			{
				dst.reset(new memory::tensor<Dtype>(std::vector<int>{1, channels, height, width}, -1, memory::NCHW));
				Dtype* dst_data = dst->mutable_cpu_data();
				int dst_offset = width * height;
				int* c_dst_offset = new int[channels];

				for (int c = 0; c < channels; c++)
				{
					if (bgr2rgb)
					{
						c_dst_offset[c] = (channels - 1 - c) * dst_offset;
					}
					else
					{
						c_dst_offset[c] = c * dst_offset;
					}
				}

				for (int c = 0; c < channels; c++)
				{
					for (int h = 0; h < height; h++)
					{
						const Dtype* src_data = srcu.ptr<Dtype>(h);
						int dst_sub_offset = h * width;

						for (int w = 0; w < width; w++)
						{
							dst_data[c_dst_offset[c] + dst_sub_offset + w] =
								src_data[w * channels + c];
						}
					}
				}

				delete[] c_dst_offset;
			}
			else if (order == memory::NHWC)
			{
				dst.reset(new memory::tensor<Dtype>(std::vector<int>{1, height, width, channels}, -1, memory::NHWC));
				Dtype* dst_data = dst->mutable_cpu_data();
				memcpy(dst_data, srcu.data, height * width * channels * sizeof(Dtype));
			}
			else
			{
				NOT_IMPLEMENTED;
			}
		}
	}
	TEST_CLASS(longinus_romancia_test)
	{
	public:
		longinus_romancia_test() : retina_{ exposing::make_exported_interface<longinus::retina_net>(u8"E:/Research/Source/Repos/Excalibur/models/retina.phai", u8"E:/Research/Source/Repos/Excalibur/models/retina.racy",0.4,-1) },
			face_alignment_{exposing::make_exported_interface<romancia::face_alignment>(-1)}
		{
		}

		TEST_METHOD(detect_alignment_test)
		{
			cv::Mat img = cv::imread("C:/Users/Glasssix-ZYF/Desktop/test.jpg");
			cv::resize(img, img, cv::Size(320, 240));
			cv::Mat gray;
			cv::cvtColor(img, gray, CV_BGR2GRAY);

			std::shared_ptr<memory::tensor<std::uint8_t>> tensor_img;
			mat2tensor_cpu(img, tensor_img);
			exposing::param_span<std::uint8_t> img_span(tensor_img->mutable_cpu_data(), tensor_img->channels() * tensor_img->height() * tensor_img->width());
			auto detect_result = retina_.get(img_span, tensor_img->channels(), tensor_img->height(), tensor_img->width(), 16, 0.5, 0);

			for (const auto& x : detect_result)
			{
				
				std::string face_str = "{x: " + std::to_string(x.x()) + ", y: " + std::to_string(x.y()) + ", height: " + std::to_string(x.height()) + ", width: " + std::to_string(x.width());
				
				face_str += ", pts:";
				auto pts = x.pts();
				for (const auto& y : pts)
				{
					face_str += " " + std::to_string(y.key()) + "," + std::to_string(y.value());
				}
				face_str += " }";

				Logger::WriteMessage(face_str.c_str());
			}


			exposing::param_span<std::uint8_t> gray_span(gray.data, gray.rows * gray.cols);
			auto align_result = face_alignment_.get(gray_span, gray.rows, gray.cols, detect_result);

			Logger::WriteMessage((std::string("algin result num: ") + std::to_string(align_result.size())).c_str());
		}
	private:
		longinus::retina_net retina_;
		romancia::face_alignment face_alignment_;
	};
}
