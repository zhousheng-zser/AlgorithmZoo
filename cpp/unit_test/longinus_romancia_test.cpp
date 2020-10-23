#include "CppUnitTest.h"
#include "../longinus/retina_net.hpp"
#include "../romancia/face_alignment.hpp"
#include "../gaius/feature_extractor.hpp"
#include "../cassius/feature_extractor.hpp"
#include "../common/include/Primitives/tensor.hpp"

#include <random>
#include <algorithm>
#include <filesystem>

#include <abi/consumer.hpp>
#include <profiler.hpp>
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
		longinus_romancia_test() : retina_{ exposing::make_exported_interface<longinus::retina_net>(u8"models/longinus.racy",u8"models/banshee.racy", 0.4,-1) },
			face_alignment_{exposing::make_exported_interface<romancia::face_alignment>(u8"models/antispoofing80x80", -1)}
		{
		}

		TEST_METHOD(detect_alignment_test)
		{
			cv::Mat img = cv::imread("C:/Users/Glasssix-ZYF/Desktop/WeChat Image_20201010144107.jpg");
			cv::resize(img, img, cv::Size(320, 240));

			std::shared_ptr<memory::tensor<std::uint8_t>> tensor_img;
			mat2tensor_cpu(img, tensor_img);
			exposing::param_span<std::uint8_t> img_span(tensor_img->mutable_cpu_data(), tensor_img->channels() * tensor_img->height() * tensor_img->width());
			auto detect_result = retina_.detect(img_span, tensor_img->channels(), tensor_img->height(), tensor_img->width(), 16, 0.5, 0, false);

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


			auto align_result = face_alignment_.get(img_span, img.channels(), img.rows, img.cols, detect_result, 0);

			Logger::WriteMessage((std::string("algin result num: ") + std::to_string(align_result.size())).c_str());
		}

		TEST_METHOD(tracker_test)
		{
			cv::VideoCapture cap(0);
			bool need_detect = true;
			longinus::face_info tracker_face{ nullptr };
			glasssix::timer t;
			for (size_t i = 0; i < 10000; i++)
			{
				cv::Mat img;
				cap >> img;

				//cv::resize(img, img, cv::Size(320, 240));

				exposing::param_span<std::uint8_t> img_span(img.data, img.rows *img.cols * img.channels());

				exposing::param_vector<longinus::face_info> detect_result;
				if (need_detect)
				{
					detect_result = retina_.detect(img_span, img.channels(), img.rows, img.cols, 16, 0.5, 1, false);
					if (detect_result.size())
					{
						need_detect = false;
						float area = 0.0f;
						for (const auto& x : detect_result)
						{
							if (x.width() * x.height() > area)
							{
								tracker_face = x;
								area = x.width() * x.height();
							}

							cv::rectangle(img, cv::Rect(x.x(), x.y(), x.width(), x.height()), cv::Scalar(0, 0, 255));
							std::string face_str = "detect: {x: " + std::to_string(x.x()) + ", y: " + std::to_string(x.y()) + ", height: " + std::to_string(x.height()) + ", width: " + std::to_string(x.width());

							face_str += ", pts:";
							auto pts = x.pts();
							for (const auto& y : pts)
							{
								cv::circle(img, cv::Point(y.key(), y.value()), 2, cv::Scalar(0, 255, 255));
								face_str += " " + std::to_string(y.key()) + "," + std::to_string(y.value());
							}
							face_str += " }";

							Logger::WriteMessage(face_str.c_str());
						}
					}
				}
				else
				{
					t.start();
					try
					{
						tracker_face = retina_.single_trace(tracker_face, img_span, img.channels(), img.rows, img.cols, 1);
					}
					catch (const exposing::abi_error& err)
					{
						std::cout << err.what_to_narrow() << std::endl;
					}
					t.stop();
					Logger::WriteMessage(std::to_string(t.get_elapsed_milli_seconds()).c_str());
					if (tracker_face.confidence() > 0.1)
					{
						need_detect = false;
						cv::rectangle(img, cv::Rect(tracker_face.x(), tracker_face.y(), tracker_face.width(), tracker_face.height()), cv::Scalar(0, 0, 255));
						cv::putText(img, "yaw: " + std::to_string(tracker_face.yaw()), cv::Point(0, 20), 2, 1.0, cv::Scalar(0, 0, 255));
						cv::putText(img, "pitch: " + std::to_string(tracker_face.pitch()), cv::Point(0, 50), 2, 1.0, cv::Scalar(0, 0, 255));
						cv::putText(img, "roll: " + std::to_string(tracker_face.roll()), cv::Point(0, 80), 2, 1.0, cv::Scalar(0, 0, 255));
						std::string face_str = "tracker:  {x: " + std::to_string(tracker_face.x()) + ", y: " + std::to_string(tracker_face.y()) + ", height: " + std::to_string(tracker_face.height()) + ", width: " + std::to_string(tracker_face.width());
						face_str += ", pts:";
						auto pts = tracker_face.pts();
						for (const auto& y : pts)
						{
							cv::circle(img, cv::Point(y.key(), y.value()), 2, cv::Scalar(0, 255, 255));
							face_str += " " + std::to_string(y.key()) + "," + std::to_string(y.value());
						}
						face_str += " }";

						
						Logger::WriteMessage(face_str.c_str());
					}
					else
						need_detect = true;
				}

				cv::imshow("img", img);
				cv::waitKey(25);
			}
		}

		TEST_METHOD(tracker_video_test)
		{
			
			bool need_detect = true;
			longinus::face_info tracker_face{ nullptr };
			for (size_t i = 0; i < 2; i++)
			{
				cv::Mat img = cv::imread("C:\\Users\\Glasssix-ZYF\\Desktop\\111.png");
				//cv::Mat img;
				//cap >> img;

				//cv::resize(img, img, cv::Size(320, 240));

				exposing::param_span<std::uint8_t> img_span(img.data, img.rows * img.cols * img.channels());

				exposing::param_vector<longinus::face_info> detect_result;
				if (need_detect)
				{
					detect_result = retina_.detect(img_span, img.channels(), img.rows, img.cols, 16, 0.5, 1, false);
					if (detect_result.size())
					{
						need_detect = false;
						int area = INT_MIN;
						for (const auto& x : detect_result)
						{
							if (x.width() * x.height() > area)
							{
								tracker_face = x;
								area = x.width() * x.height();
							}

							cv::rectangle(img, cv::Rect(x.x(), x.y(), x.width(), x.height()), cv::Scalar(0, 0, 255));
							std::string face_str = "detect: {x: " + std::to_string(x.x()) + ", y: " + std::to_string(x.y()) + ", height: " + std::to_string(x.height()) + ", width: " + std::to_string(x.width());

							face_str += ", pts:";
							auto pts = x.pts();
							for (const auto& y : pts)
							{
								cv::circle(img, cv::Point(y.key(), y.value()), 2, cv::Scalar(0, 255, 255));
								face_str += " " + std::to_string(y.key()) + "," + std::to_string(y.value());
							}
							face_str += " }";

							Logger::WriteMessage(face_str.c_str());
						}
					}
				}
				else
				{
					try
					{
						tracker_face = retina_.single_trace(tracker_face, img_span, img.channels(), img.rows, img.cols, 1);
					}
					catch (const exposing::abi_error& err)
					{
						std::cout << err.what_to_narrow() << std::endl;
					}
					if (tracker_face.confidence() > 0.1)
					{
						need_detect = false;
						cv::rectangle(img, cv::Rect(tracker_face.x(), tracker_face.y(), tracker_face.width(), tracker_face.height()), cv::Scalar(0, 0, 255));
						cv::putText(img, "yaw: " + std::to_string(tracker_face.yaw()), cv::Point(0, 20), 2, 1.0, cv::Scalar(0, 0, 255));
						cv::putText(img, "pitch: " + std::to_string(tracker_face.pitch()), cv::Point(0, 50), 2, 1.0, cv::Scalar(0, 0, 255));
						cv::putText(img, "roll: " + std::to_string(tracker_face.roll()), cv::Point(0, 80), 2, 1.0, cv::Scalar(0, 0, 255));
						std::string face_str = "tracker:  {x: " + std::to_string(tracker_face.x()) + ", y: " + std::to_string(tracker_face.y()) + ", height: " + std::to_string(tracker_face.height()) + ", width: " + std::to_string(tracker_face.width());
						face_str += ", pts:";
						auto pts = tracker_face.pts();
						for (const auto& y : pts)
						{
							cv::circle(img, cv::Point(y.key(), y.value()), 2, cv::Scalar(0, 255, 255));
							face_str += " " + std::to_string(y.key()) + "," + std::to_string(y.value());
						}
						face_str += " }";


						Logger::WriteMessage(face_str.c_str());

						double value = face_alignment_.antispoofing(tracker_face, img_span, img.channels(), img.rows, img.cols, 1);
						Logger::WriteMessage(std::to_string(value).c_str());
					}
					else
						need_detect = true;
				}

				cv::imshow("img", img);
				cv::waitKey();
			}
		}
	private:
		longinus::retina_net retina_;
		romancia::face_alignment face_alignment_;
	};


	TEST_CLASS(union_test)
	{
	public:
		union_test():retina_{ exposing::make_exported_interface<longinus::retina_net>(u8"models/longinus.racy",u8"models/banshee.racy", 0.4,-1) },
			face_alignment_{ exposing::make_exported_interface<romancia::face_alignment>(u8"models/antispoofing80x80", -1) },
			gaius_{ exposing::make_exported_interface<gaius::feature_extractor>(u8"models/mobile_unicorn.racy", u8"models/mobile_unicorn_mask.racy", -1) },
			cassius_{ exposing::make_exported_interface<cassius::feature_extractor>(u8"models/unicorn.racy", -1) }
		{}

		TEST_METHOD(union_test1)
		{
			cv::Mat img1 = cv::imread("a1.jpg");
			cv::Mat img2 = cv::imread("a2.jpg");

			std::shared_ptr<memory::tensor<std::uint8_t>> tensor_img1, tensor_img2;
			mat2tensor_cpu(img1, tensor_img1);
			mat2tensor_cpu(img2, tensor_img2);
			/*exposing::param_span<std::uint8_t> img_span1(tensor_img1->mutable_cpu_data(), tensor_img1->channels() * tensor_img1->height() * tensor_img1->width());
			exposing::param_span<std::uint8_t> img_span2(tensor_img2->mutable_cpu_data(), tensor_img2->channels() * tensor_img2->height() * tensor_img2->width());
			auto detect_result1 = retina_.detect(img_span1, tensor_img1->channels(), tensor_img1->height(), tensor_img1->width(), 16, 0.5, 0, false);
			auto detect_result2 = retina_.detect(img_span2, tensor_img2->channels(), tensor_img2->height(), tensor_img2->width(), 16, 0.5, 0, false);

			auto align_result1 = face_alignment_.get(img_span1, img1.channels(), img1.rows, img1.cols, detect_result1, 0);
			auto align_result2 = face_alignment_.get(img_span2, img2.channels(), img2.rows, img2.cols, detect_result2, 0);*/

			auto forward_result1 = gaius_.get(exposing::param_span<uint8_t>(const_cast<uint8_t *>(tensor_img1->cpu_data()), 3 * 128 * 128), 1, 0, false);
			auto forward_result2 = gaius_.get(exposing::param_span<uint8_t>(const_cast<uint8_t *>(tensor_img2->cpu_data()), 3 * 128 * 128), 1, 0, false);

			float sum0 = 0.0f;
			float sum1 = 0.0f;
			float sum2 = 0.0f;
			for (int j = 0; j < forward_result1[0].size(); j++)
			{
				sum0 += forward_result1[0][j] * forward_result2[0][j];
				sum1 += forward_result1[0][j] * forward_result1[0][j];
				sum2 += forward_result2[0][j] * forward_result2[0][j];
			}
			Logger::WriteMessage(("gaius cosine: " + std::to_string(sum0 / sqrt(sum1 * sum2))).c_str());

			auto forward_result3 = cassius_.get(exposing::param_span<uint8_t>(const_cast<uint8_t*>(tensor_img1->cpu_data()), 3 * 128 * 128), 1, 0);
			auto forward_result4 = cassius_.get(exposing::param_span<uint8_t>(const_cast<uint8_t*>(tensor_img2->cpu_data()), 3 * 128 * 128), 1, 0);

			std::vector<float> fea1;
			std::vector<float> fea2;
			sum0 = sum1 = sum2 = 0.0f;
			for (int j = 0; j < forward_result3[0].size(); j++)
			{
				sum0 += forward_result3[0][j] * forward_result4[0][j];
				sum1 += forward_result3[0][j] * forward_result3[0][j];
				sum2 += forward_result4[0][j] * forward_result4[0][j];
			}
			Logger::WriteMessage(("cassius cosine: " + std::to_string(sum0 / sqrt(sum1 * sum2))).c_str());
		}
	private:
		longinus::retina_net retina_;
		romancia::face_alignment face_alignment_;
		gaius::feature_extractor gaius_;
		cassius::feature_extractor cassius_;
	};
}
