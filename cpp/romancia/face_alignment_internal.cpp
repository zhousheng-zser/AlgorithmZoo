#include "face_alignment_internal.hpp"
#include "hardcode.hpp"
#ifdef USE_RKNNAPI
//#if 0
#include "../../common/include/RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#endif

#include <cmath>
#include <climits>

#include <abi/param_span.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <Excalibur/pipeline.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_rotate.hpp>
#include <Excalibur/operation_equalize_hist.hpp>
#include <Excalibur/operation_merge_channel.hpp>
#include <Excalibur/operation_resize.hpp>
#include <Excalibur/operation_rgb2gray.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

using glasssix::excalibur::rectangle;
using glasssix::excalibur::point;


namespace glasssix::romancia
{
	class face_alignment_internal::impl
	{
	public:
		impl() = delete;

		impl(const exposing::param_string& blur_racy_path, std::int32_t device) : device_{ device },
			blur_instance_{ get_model_params("blur_detection_best"), std::string{blur_racy_path}, device }
		{

		}

		~impl()
		{
		}

		exposing::param_vector<float> blur_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order)
		{
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}

			auto result = exposing::make_param_vector<float>();

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
//#if 0
			if (order != 1)
				throw exposing::abi_invalid_argument("Not supported order");

			cv::Mat img(height, width, CV_8UC3, bitmap.data());
			std::uint8_t temp[faces.size() * 3 * 128 * 128];
			std::uint8_t* ptr = temp;
			for (size_t i = 0; i < faces.size(); i++)
			{
				cv::Mat crop_face;
				cv::Rect2f rect(faces[i].x(), faces[i].y(), faces[i].width(), faces[i].height());
				safty_cut(img, crop_face, rect);
				cv::resize(crop_face, crop_face, cv::Size(128, 128));
				std::copy(crop_face.data, crop_face.data + 3 * 128 * 128, ptr);
				ptr += 3 * 128 * 128;
			}

			auto network_result = blur_instance_.forward(temp, { static_cast<int>(faces.size()), 128, 128, 3 }, RKNN_TENSOR_NHWC);
#ifdef USE_RKNNAPI
			if (auto iter = network_result.find("Gemm_Gemm_226/out0_0"); iter != network_result.end())
#else
			if (auto iter = network_result.find("output"); iter != network_result.end())
#endif
#else
			init_cache(bitmap, channels, height, width, order);
			std::shared_ptr<memory::tensor<uint8_t>> crop_faces(new memory::tensor<uint8_t>(std::vector<int>{static_cast<int>(faces.size()), 3, 128, 128}, -1, memory::NCHW, nullptr));
			uint8_t* ptr = crop_faces->mutable_cpu_data();
			for (size_t i = 0; i < faces.size(); i++)
			{
				rectangle<int> MarginRect = rectangle<int>(faces[i].x() - faces[i].width() * 0.0f,
					faces[i].y() - faces[i].height() * 0.0f,
					faces[i].height() * 1.0f,
					faces[i].width() * 1.0f);

				std::shared_ptr<memory::tensor<uint8_t>> crop_face;
				excalibur::safty_cut_cpu(cache_, crop_face, &MarginRect);
				excalibur::resize_cpu(crop_face, crop_face, 128, 128);
				const uint8_t* crop_data = crop_face->cpu_data();
				std::copy(crop_data, crop_data + 3 * 128 * 128, ptr);
				ptr += 3 * 128 * 128;
			}
			auto network_result = blur_instance_.forward(crop_faces | memory::tensor_convert_to<float>);
			if (auto iter = network_result.find("output"); iter != network_result.end())
#endif
			{
				auto iter_output = iter->second->cpu_data();
				for (std::size_t i = 0; i < faces.size(); i++)
					result.push_back(iter_output[i]);
			}

			return result;
		}

		exposing::param_vector<double> mask_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
		{
			cv::Mat img;
			if (channels == 3 && order == memory::NHWC)
			{
				img = cv::Mat(height, width, CV_8UC3, const_cast<uchar*>(bitmap.data()));
			}
			else
				throw exposing::abi_invalid_argument("Not supported channels or order");

			cv::Mat src;
			auto result = exposing::make_param_vector<double>();
			for (const auto& i : faces)
			{
				safty_cut(img, src, cv::Rect(i.x(), i.y(), i.width(), i.height()));
				auto landmark = i.pts();
				cv::Point face_start(i.x(), i.y());
				std::vector<cv::Point> vec;
				vec.emplace_back(landmark[2].key() - face_start.x, landmark[2].value() - face_start.y);
				vec.emplace_back(landmark[3].key() - face_start.x, landmark[3].value() - face_start.y);
				vec.emplace_back(landmark[4].key() - face_start.x, landmark[4].value() - face_start.y);

				std::sort(vec.begin(), vec.end(), [](const cv::Point& first, const cv::Point& second) {return first.y < second.y; });
				int topy = vec[2].y;
				int bottomy = vec[0].y;
				std::sort(vec.begin(), vec.end(), [](const cv::Point& first, const cv::Point& second) {return first.x < second.x; });
				int rightx = vec[2].x;
				int leftx = vec[0].x;

				int w = rightx - leftx;
				int h = topy - bottomy;

				w = 1.3 * w;
				int pad = int(0.15 * w);
				leftx = leftx - pad;
				rightx = rightx + pad;
				int max_edge = std::max(w, h);
				int min_edge = std::min(w, h);
				int padding = (max_edge - min_edge) / 2;
				if (h > w)
				{
					leftx = leftx - padding;
					rightx = leftx + max_edge;
				}
				else if (h < w)
				{
					bottomy = bottomy - int(0.5 * padding);
					topy = bottomy + max_edge;
				}

				cv::Mat crop;

				safty_cut(src, crop, cv::Rect(leftx, bottomy, rightx - leftx + 1, topy - bottomy + 1));
				cv::resize(crop, crop, cv::Size(30, 30));

				cv::cvtColor(crop, crop, CV_BGR2HSV);
				std::vector<cv::Mat> splited;
				cv::split(crop, splited);
				cv::Mat mean, stddev;
				cv::meanStdDev(splited[0], mean, stddev);

				double m1 = mean.at<double>(0, 0);     //��ֵ
				//if (m1 < 30)
				//	return 0.0;

				result.push_back(m1);
			}

			return result;
		}

		exposing::param_vector<std::uint8_t> rotate(float angle, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
		{
			cv::Mat src;
			if (channels == 3 && order == memory::NHWC)
			{
				src = cv::Mat(height, width, CV_8UC3, const_cast<uchar*>(bitmap.data()));
			}
			else
				throw exposing::abi_invalid_argument("Not supported channels or order");

			cv::Mat img;

			if (angle == 0.0f)
			{
				img = src;
			}
			else if (angle == 90.0f)
				cv::rotate(src, img, cv::ROTATE_90_COUNTERCLOCKWISE);
			else if (angle == 180.0f)
				cv::rotate(src, img, cv::ROTATE_180);
			else if (angle == 270.0f)
				cv::rotate(src, img, cv::ROTATE_90_CLOCKWISE);
			else
				throw exposing::abi_invalid_argument("Not supported angle");


			auto result = exposing::make_param_vector<std::uint8_t>();
			result.resize(static_cast<size_t>(src.channels() * src.cols * src.rows));
			result.copy_from({ img.data , static_cast<size_t>(src.channels() * src.cols * src.rows) }, 0);

			return result;
		}

		static std::string version()
		{
			return "1.0.0";
		}
	private:
		void init_cache(exposing::param_span<std::uint8_t>& bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order)
		{
			if (cache_ == nullptr || cache_->channels() != channels || cache_->height() != height || cache_->width() != width || cache_->order() != order)
			{
				std::vector<int> shape;
				if (order == memory::NCHW)
					shape = { static_cast<int>(1), channels, height, width };
				else if (order == memory::NHWC)
					shape = { static_cast<int>(1), height, width, channels };
				else
					NOT_IMPLEMENTED;

				cache_ = std::make_shared<memory::tensor<std::uint8_t>>(shape, -1, (memory::orderType)order/*, &memory::pool_allocator_default<std::uint8_t>::get()*/);
			}

			if (cache_->device() > 0)
			{
#ifdef USE_CUDA
				cudaMemcpy(cache_->mutable_gpu_data(), bitmap, channels * height * width, cudaMemcpyHostToDevice);
#else
				NO_GPU;
#endif
			}
			else
				std::copy(bitmap.begin(), bitmap.end(), cache_->mutable_cpu_data());

			if (order == memory::NHWC)
				cache_->convert_order();
		}

		inline void safty_cut(cv::Mat& img, cv::Mat& dst, cv::Rect roi)
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

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
//#if 0
		rknnwrapper::rknn_wrapper blur_instance_;
#else
		glasssix::excalibur::pipeline<float> blur_instance_;
#endif
		int device_;
		std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
	};


	face_alignment_internal::face_alignment_internal(const exposing::param_string& blur_racy_path, int device) : impl_{ std::make_unique<impl>(blur_racy_path, device) }
	{
	}

	face_alignment_internal::~face_alignment_internal()
	{
	}

	exposing::param_vector<float> face_alignment_internal::blur_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
	{
		return impl_->blur_detect(faces, bitmap, channels, height, width, order);
	}

	exposing::param_vector<double> face_alignment_internal::mask_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
	{
		return impl_->mask_detect(faces, bitmap, channels, height, width, order);
	}

	exposing::param_vector<std::uint8_t> face_alignment_internal::rotate(float angle, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
	{
		return impl_->rotate(angle, bitmap, channels, height, width, order);
	}

	std::string face_alignment_internal::version()
	{
		return impl::version();
	}
}
