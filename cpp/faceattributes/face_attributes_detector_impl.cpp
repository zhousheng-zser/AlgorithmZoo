#include "face_attributes_detector_impl.hpp"
#include "face_attribute_info_impl.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#include "Excalibur/pipeline.hpp"
#include "Excalibur/operation_safty_cut.hpp"
#include "Excalibur/operation_resize.hpp"
#include "Primitives/tensor_conversions.hpp"
#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include <GenPipeline/GetPostprocessing.hpp>

namespace glasssix::face_attributes
{
	namespace
	{
		constexpr std::size_t forward_input_width = 96;
		constexpr std::size_t forward_input_height = 96;
		constexpr std::size_t forward_input_channels = 3;
		constexpr std::size_t forward_input_bytes = forward_input_channels * forward_input_width * forward_input_height;
	}

	class face_attributes_detector_impl::impl
	{
	public:
		impl() {};

		impl(std::string_view model_directory, int device) :impl()
		{
			std::string model_dir = exposing::to_narrow_string(model_directory);
			if (*model_dir.rbegin() != '/') model_dir += '/';
		#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "face_attributes.rknn", 0);
		#elif defined(USE_BMNN)
			ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "face_attributes.bmodel", 0);
		#else
			ioprocess_pipeline_ = PrePostProcessGenPipeline::mkSharePipeline(model_dir + "face_attributes.onnx", 0);
		#endif
			ioprocess_pipeline_->manual_possible_normalization(0, 1.f / 255);
			//ioprocess_pipeline_->set_postprocessing(yolov8_GEN<1, 1>);
		}
		~impl()
		{
		}
		exposing::param_vector<face_attribute_info> detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order)
		{
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}
			if (order != 1)
				throw exposing::abi_invalid_argument("Not supported order");

			auto results = exposing::make_param_vector<face_attributes::face_attribute_info>();
			cv::Mat img(cv::Size(width, height),CV_8UC3,const_cast<uint8_t*>(bitmap.data()));
// #if 1 || defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			// cv::Mat img(height, width, CV_8UC3, bitmap.data());
			// std::vector<std::uint8_t> temp(faces.size() * forward_input_bytes);
			for (size_t i = 0; i < faces.size(); i++)
			{
				cv::Mat crop_face;
				cv::Rect2f rect(faces[i].x(), faces[i].y(), faces[i].width(), faces[i].height());
				refine(rect, height, width);
				mat_safty_cut(img, crop_face, rect);
				cv::resize(crop_face, crop_face, cv::Size(forward_input_width, forward_input_height));
				auto network_result = ioprocess_pipeline_->forward(crop_face);
				const std::vector<std::string> out_names = { "gender", "age", "mask", "glass" };
				auto gender_data = network_result[out_names[0]]->cpu_data();
				auto age_data = network_result[out_names[1]]->cpu_data();
				auto mask_data = network_result[out_names[2]]->cpu_data();
				auto glass_data = network_result[out_names[3]]->cpu_data();
				int gender_index = std::max_element(gender_data, gender_data + 2) - gender_data;
				int age_index = std::max_element(age_data, age_data + 4) - age_data;
				int mask_index = std::max_element(mask_data, mask_data + 2) - mask_data;
				int glass_index = std::max_element(glass_data, glass_data + 2) - glass_data;
				face_attributes::face_attribute_info_internal face_info{ gender_index, age_index, mask_index, glass_index };
				results.push_back(glasssix::exposing::make_as_first<face_attributes::face_attribute_info_impl>(face_info));
			}

			return results;
		}

		exposing::param_string version() const
		{
			return "1.0.3";
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
			cv::Rect rect(_x, _y, _width, _height);
        	img(cv::Rect(_x, _y, _width, _height)).copyTo(mat(cv::Rect(_x - x, _y - y, _width, _height)));
        	dst = mat;
    	}

		static inline void refine(cv::Rect2f& face, int height, int width)
		{
			float bbw = 0, bbh = 0, maxSide = 0, minSide = 0;
			float h = 0, w = 0;
			float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
			bbw = face.width - 1;
			bbh = face.height - 1;
			x1 = face.x;
			y1 = face.y;

			maxSide = ((bbh > bbw) ? bbh : bbw) * 1.02f + 10;
			x1 = x1 + bbw * 0.5 - maxSide * 0.5;
			y1 = y1 + bbh * 0.5 - maxSide * 0.5;
			face.width = round(maxSide + 1);
			face.height = round(maxSide + 1);
			face.x = round(x1);
			face.y = round(y1);

			//boundary check
			if (face.x < 0)
				face.x = 0;
			if (face.y < 0)
				face.y = 0;
			if (face.x + face.width - 1 > width)
				face.width = width - face.x;
			if (face.y + face.height - 1 > height)
				face.height = height - face.y;

			//minSide = (face.height > face.width) ? face.width : face.height;
			//face.height = minSide;
			//face.width = minSide;
		}


	private:
		int device_;
		std::shared_ptr<PrePostProcessGenPipeline> ioprocess_pipeline_;
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
