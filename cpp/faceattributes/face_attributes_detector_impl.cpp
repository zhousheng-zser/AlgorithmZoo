#include "face_attributes_detector_impl.hpp"
#include "hardcode.hpp"
#include "face_attribute_info_impl.hpp"
#ifdef USE_RKNNAPI
#include "RKNNWrapper/rknn_wrapper.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#elif defined(USE_RKNN2API)
#include "RKNN2Wrapper/rknn2_wrapper.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#else
#include "Excalibur/pipeline.hpp"
#endif

namespace glasssix::face_attributes
{
	class face_attributes_detector_impl::impl
	{
	public:
		impl() = delete;

		impl(const exposing::param_string &models_directory, std::int32_t device) : device_{device}, instance_{get_model_params("attributes_detection_best"), std::string{models_directory} + "/face_attributes.racy", device}
		{
		}

		~impl()
		{
		}

		exposing::param_vector<face_attribute_info> detect(const exposing::param_vector<longinus::face_info> &faces, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order)
		{
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}
			// auto result = exposing::make_param_vector<face_attribute_info>();
			if (order != 1)
				throw exposing::abi_invalid_argument("Not supported order");

				auto results = exposing::make_param_vector<face_attributes::face_attribute_info>();				
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
//#if 0
		
			cv::Mat img(height, width, CV_8UC3, bitmap.data());
			std::uint8_t temp[ 3 * 96 * 96];
			std::uint8_t* ptr = temp;
			for (size_t i = 0; i < faces.size(); i++)
			{
				cv::Mat crop_face;
				cv::Rect2f rect(faces[i].x(), faces[i].y(), faces[i].width(), faces[i].height());
				mat_safty_cut(img, crop_face, rect);
				cv::resize(crop_face, crop_face, cv::Size(96, 96));
				std::copy(crop_face.data, crop_face.data + 3 * 96 * 96, ptr);
				// ptr += 3 * 96 * 96;
				auto network_result = instance_.forward(temp, { static_cast<int>(1), 96, 96, 3 }, RKNN_TENSOR_NHWC);
				std::vector<std::string> out_names = {"gender","age","mask","glass"};
//#ifdef USE_RKNNAPI //rv1109
//			std::vector<std::string> out_names = {"age","gender","mask","glass"};
//#else
//#endif
// #else
			// for(size_t i = 0; i < out_names.size();i++)
			// {
			// 	init_cache(bitmap, channels, height, width, order);
			// 	std::shared_ptr<memory::tensor<uint8_t>> crop_faces(new memory::tensor<uint8_t>(std::vector<int>{static_cast<int>(faces.size()), 3, 96, 96}, -1, memory::NCHW, nullptr));
			// 	uint8_t* ptr = crop_faces->mutable_cpu_data();
			// 	for (size_t i = 0; i < faces.size(); i++)
			// 	{
			// 		rectangle<int> MarginRect = rectangle<int>(faces[i].x() - faces[i].width() * 0.0f,
			// 			faces[i].y() - faces[i].height() * 0.0f,
			// 			faces[i].height() * 1.0f,
			// 			faces[i].width() * 1.0f);

			// 		std::shared_ptr<memory::tensor<uint8_t>> crop_face;
			// 		excalibur::safty_cut_cpu(cache_, crop_face, &MarginRect);
			// 		excalibur::resize_cpu(crop_face, crop_face, 96, 96);
			// 		const uint8_t* crop_data = crop_face->cpu_data();
			// 		std::copy(crop_data, crop_data + 3 * 96 * 96, ptr);
			// 		ptr += 3 * 96 * 96;
			// 	}
			// 	auto network_result = instance_.forward(crop_faces | memory::tensor_convert_to<float>);
			// 	std::vector<std::string> out_names = {"output","output1","output2","output3"};
#endif
				// exposing::param_vector<int> ret;
				std::vector<int> ret;
				// face_attribute_info_internal face_info;
				for(size_t i = 0; i < out_names.size();i++)
				{
					if (auto iter = network_result.find(out_names[i]); iter != network_result.end())
					{
						auto iter_output = iter->second->cpu_data();

						auto index = -1;
						if(out_names[i] == "age")
						{
							auto max = *iter_output;
							index = 0;

							for(auto it2 = 1; it2 < 4; it2++)
							{
								if(iter_output[it2] > max)
								{
									max = iter_output[it2];
									index = it2;
								}
							}
						}
						else
						{
							index = *iter_output > *(iter_output +1) ? 0 : 1;
						}
						ret.push_back(index);//TODO ：有问题，第一次，ret【1】没有值的，会报错,记得改
					}
				}

			
				face_attributes::face_attribute_info_internal face_info{ret[0],ret[1],ret[2],ret[3]};

				results.push_back( glasssix::exposing::make_as_first<face_attributes::face_attribute_info_impl>(face_info));

			return results;
			// std::cout << result.size() << std::endl;
		}

		exposing::param_string version() const
		{
			return "1.0.1";
		}

	private:
		static inline void mat_safty_cut(cv::Mat& img, cv::Mat& dst, cv::Rect roi)
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
        		dst = mat;
    		}

	private:
		int device_;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
		rknnwrapper::rknn_wrapper instance_;
#else
		glasssix::excalibur::pipeline<float> instance_;
#endif
	};

	face_attributes_detector_impl::face_attributes_detector_impl()
	{
	}

	face_attributes_detector_impl::~face_attributes_detector_impl()
	{
	}
	void face_attributes_detector_impl::init(const exposing::param_string &models_directory, std::int32_t device)
	{
		impl_ = std::make_unique<impl>(models_directory, device);
	}

	exposing::param_string face_attributes_detector_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<face_attribute_info> face_attributes_detector_impl::detect(const exposing::param_vector<longinus::face_info> &faces, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"face_attributes_detector_internal object not initialized");

		return impl_->detect(faces, bitmap, channels, height, width, order);
	}
}
