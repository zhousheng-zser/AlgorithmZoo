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

		exposing::param_vector<exposing::param_vector<std::uint8_t>> align128(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			const exposing::param_vector<longinus::face_info>& faces, std::int32_t order)
		{
#ifdef __arm__
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}
			if (order != 1)
				throw exposing::abi_invalid_argument("Not supported order");

			cv::Mat img(height, width, CV_8UC3, bitmap.data());

			cv::Mat ROI, rotated_ROI, final_mat, final_mat_gray;
			auto res = exposing::make_param_vector<std::uint8_t, 2>();

			cv::Mat gray;
			cv::cvtColor(img, gray, CV_BGR2GRAY);
			for (size_t i = 0; i < faces.size(); i++)
			{
				cv::Rect MarginRect(faces[i].x() - faces[i].width() * 0.2f,
					faces[i].y() - faces[i].height() * 0.2f,
					faces[i].width() * 1.4f,
					faces[i].height() * 1.4f);

				safty_cut(gray, ROI, MarginRect);

				cv::Point2f ldmk5[5];
				auto pts = faces[i].pts();
				for (size_t j = 0; j < pts.size(); j++)
				{
					ldmk5[j] = cv::Point2f(pts[j].key() - MarginRect.x, pts[j].value() - MarginRect.y);
				}
				cv::Point2f center_eye((ldmk5[0].x + ldmk5[1].x) / 2, (ldmk5[0].y + ldmk5[1].y) / 2);
				cv::Point2f center_mouth((ldmk5[3].x + ldmk5[4].x) / 2, (ldmk5[3].y + ldmk5[4].y) / 2);
				cv::Point2f center((center_eye.x + center_mouth.x) / 2, (center_eye.y + center_mouth.y) / 2);
				double tan = (center_eye.x - center_mouth.x) / (center_eye.y - center_mouth.y);
				double arctan = atan(tan) * 180 / 3.1415926;

				cv::Mat rot_mat = cv::getRotationMatrix2D(center, -1 * arctan, 1.0);
				cv::warpAffine(ROI, rotated_ROI, rot_mat, ROI.size(), cv::INTER_CUBIC, cv::BORDER_CONSTANT, cv::Scalar::all(0));

				double distance = std::sqrt((center_eye.x - center_mouth.x) * (center_eye.x - center_mouth.x) + (center_eye.y - center_mouth.y) * (center_eye.y - center_mouth.y));

				if (distance < std::numeric_limits<double>::epsilon())
				{
					throw exposing::abi_invalid_argument("Illegal distance. Error landmarks.");
				}

				double cos = (center_mouth.y - center_eye.y) / distance;
				double sin = (center_mouth.x - center_eye.x) / distance;
				cv::Point2f new_center_eye(center_eye.x + (float)(sin * distance / 2), (float)(center_eye.y - (1 - cos) * distance / 2));
				cv::Point2f new_center_mouth(center_mouth.x - (float)(sin * distance / 2), (float)(center_mouth.y + (1 - cos) * distance / 2));
				cv::Rect2f final_rect(new_center_eye.x - distance,
					new_center_eye.y - distance / 2,
					distance * 2, distance * 2);
				safty_cut(rotated_ROI, final_mat, final_rect);
				cv::equalizeHist(final_mat, final_mat);

				cv::resize(final_mat, final_mat, cv::Size(128, 128));

				auto temp_vec = exposing::make_param_vector<std::uint8_t>();
				temp_vec.resize(128 * 128 * 3);
				for (size_t j = 0; j < 3; j++)
				{
					temp_vec.copy_from({ final_mat.data , 128 * 128 }, 3 * 128 * 128);
				}

				res.push_back(temp_vec);
			}

			return res;
#else
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}
			init_cache(bitmap, channels, height, width, order);

			std::shared_ptr<memory::tensor<uint8_t>> ROI, rotated_ROI, final_mat, final_mat_gray, color_img, resized_color_img;
			std::vector<std::shared_ptr<memory::tensor<uint8_t>>> src_vector;
			auto res = exposing::make_param_vector<std::uint8_t, 2>();

			std::shared_ptr<memory::tensor<uint8_t>> gray;
			excalibur::rgb2gray_cpu(cache_, gray);
			for (size_t i = 0; i < faces.size(); i++)
			{
				src_vector.clear();
				rectangle<int> MarginRect = rectangle<int>(faces[i].x() - faces[i].width() * 0.2f,
					faces[i].y() - faces[i].height() * 0.2f,
					faces[i].height() * 1.4f,
					faces[i].width() * 1.4f);

				excalibur::safty_cut_cpu(gray, ROI, &MarginRect);

				point<float> ldmk5[5];
				auto pts = faces[i].pts();
				for (size_t j = 0; j < pts.size(); j++)
				{
					ldmk5[j] = point<float>(pts[j].key() - MarginRect.x, pts[j].value() - MarginRect.y);
				}
				point<float> center_eye = point<float>((ldmk5[0].x + ldmk5[1].x) / 2, (ldmk5[0].y + ldmk5[1].y) / 2);
				point<float> center_mouth = point<float>((ldmk5[3].x + ldmk5[4].x) / 2, (ldmk5[3].y + ldmk5[4].y) / 2);
				point<float> center = point<float>((center_eye.x + center_mouth.x) / 2, (center_eye.y + center_mouth.y) / 2);
				double tan = (center_eye.x - center_mouth.x) / (center_eye.y - center_mouth.y);
				double arctan = atan(tan) * 180 / 3.1415926;

				excalibur::rotate_with_points_cpu(ROI, rotated_ROI, center, -1 * arctan);

				double distance = std::sqrt((center_eye.x - center_mouth.x) * (center_eye.x - center_mouth.x) + (center_eye.y - center_mouth.y) * (center_eye.y - center_mouth.y));

				if (distance < std::numeric_limits<double>::epsilon())
				{
					throw exposing::abi_invalid_argument("Illegal distance. Error landmarks.");
				}

				double cos = (center_mouth.y - center_eye.y) / distance;
				double sin = (center_mouth.x - center_eye.x) / distance;
				point<float> new_center_eye = point<float>(center_eye.x + (float)(sin * distance / 2), (float)(center_eye.y - (1 - cos) * distance / 2));
				point<float> new_center_mouth = point<float>(center_mouth.x - (float)(sin * distance / 2), (float)(center_mouth.y + (1 - cos) * distance / 2));
				rectangle<float> final_rect = rectangle<float>(new_center_eye.x - distance,
					new_center_eye.y - distance / 2,
					distance * 2, distance * 2);
				excalibur::safty_cut_cpu(rotated_ROI, final_mat, &final_rect);
				excalibur::equalize_hist_cpu(final_mat, final_mat);

				for (size_t k = 0; k < 3; k++)
				{
					src_vector.push_back(final_mat);
				}

				excalibur::merge_channel_cpu(src_vector, color_img);
				excalibur::resize_cpu(color_img, resized_color_img, 128, 128);

				auto temp_vec = exposing::make_param_vector<std::uint8_t>();
				temp_vec.resize(static_cast<size_t>(resized_color_img->count()));
				temp_vec.copy_from({ resized_color_img->cpu_data() , static_cast<size_t>(resized_color_img->count()) }, 0);

				res.push_back(temp_vec);
			}

			return res;
#endif
		}

		exposing::param_vector<exposing::param_vector<std::uint8_t>> align(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			const exposing::param_vector<longinus::face_info>& faces, std::int32_t order)
		{
#ifdef __arm__
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}
			if (order != 1)
				throw exposing::abi_invalid_argument("Not supported order");

			cv::Mat img(height, width, CV_8UC3, bitmap.data());

			cv::Mat ROI, rotated_ROI, final_mat, final_mat_gray, resized_color_img;
			auto res = exposing::make_param_vector<std::uint8_t, 2>();

			for (size_t i = 0; i < faces.size(); i++)
			{
				cv::Rect MarginRect(faces[i].x() - faces[i].width() * 0.0f,
					faces[i].y() - faces[i].height() * 0.0f,
					faces[i].width() * 1.0f,
					faces[i].height() * 1.0f);

				safty_cut(img, ROI, MarginRect);
				float min_edge = std::min(MarginRect.width, MarginRect.height);
				float scale = 160.f / min_edge;
				if (scale < 1.0f)
					cv::resize(ROI, ROI, cv::Size(ROI.cols * scale, ROI.rows * scale));
				else
					scale = 1.0f;

				cv::Point2f ldmk5[5];
				auto pts = faces[i].pts();
				for (size_t j = 0; j < pts.size(); j++)
				{
					ldmk5[j] = cv::Point2f(pts[j].key() - MarginRect.x, pts[j].value() - MarginRect.y);
				}
				cv::Point2f center_eye((ldmk5[0].x + ldmk5[1].x) / 2, (ldmk5[0].y + ldmk5[1].y) / 2);
				cv::Point2f center_mouth((ldmk5[3].x + ldmk5[4].x) / 2, (ldmk5[3].y + ldmk5[4].y) / 2);
				double tan = (center_eye.x - center_mouth.x) / (center_eye.y - center_mouth.y);
				double arctan = atan(tan) * 180 / 3.1415926;

				cv::Point2f center((center_eye.x + center_mouth.x) * scale / 2, (center_eye.y + center_mouth.y) * scale / 2);
				cv::Mat rot_mat = cv::getRotationMatrix2D(center, -1 * arctan, 1.0);
				cv::warpAffine(ROI, rotated_ROI, rot_mat, ROI.size(), cv::INTER_CUBIC, cv::BORDER_CONSTANT, cv::Scalar::all(0));

				double distance = std::sqrt((center_eye.x - center_mouth.x) * (center_eye.x - center_mouth.x) + (center_eye.y - center_mouth.y) * (center_eye.y - center_mouth.y));

				if (distance < std::numeric_limits<double>::epsilon())
				{
					throw exposing::abi_invalid_argument("Illegal distance. Error landmarks.");
				}

				double cos = (center_mouth.y - center_eye.y) / distance;
				double sin = (center_mouth.x - center_eye.x) / distance;
				cv::Point2f new_center_eye(center_eye.x + (float)(sin * distance / 2), (float)(center_eye.y - (1 - cos) * distance / 2));
				cv::Point2f new_center_mouth(center_mouth.x - (float)(sin * distance / 2), (float)(center_mouth.y + (1 - cos) * distance / 2));
				cv::Rect2f final_rect((new_center_eye.x - distance * 1.25f) * scale,
					(new_center_eye.y - distance * 0.75f) * scale,
					distance * 2.5f * scale, distance * 2.5f * scale);
				safty_cut(rotated_ROI, final_mat, final_rect);

				cv::resize(final_mat, resized_color_img, cv::Size(128, 128));

				std::vector<cv::Mat> img_channel_vec;
				cv::split(resized_color_img, img_channel_vec);

				auto temp_vec = exposing::make_param_vector<std::uint8_t>();
				temp_vec.resize(static_cast<size_t>(3 * 128 * 128));
				for (size_t j = 0; j < 3; j++)
					temp_vec.copy_from({ img_channel_vec[j].data, 128 * 128 }, j * 128 * 128);

				res.push_back(temp_vec);
			}

			return res;
#else
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}
			init_cache(bitmap, channels, height, width, order);

			std::shared_ptr<memory::tensor<uint8_t>> ROI, rotated_ROI, final_mat, final_mat_gray, resized_color_img;
			auto res = exposing::make_param_vector<std::uint8_t, 2>();

			for (size_t i = 0; i < faces.size(); i++)
			{
				rectangle<int> MarginRect = rectangle<int>(faces[i].x() - faces[i].width() * 0.0f,
					faces[i].y() - faces[i].height() * 0.0f,
					faces[i].height() * 1.0f,
					faces[i].width() * 1.0f);

				excalibur::safty_cut_cpu(cache_, ROI, &MarginRect);

				float min_edge = std::min(MarginRect.w, MarginRect.h);
				float scale = 160.f / min_edge;
				if (scale < 1.0f)
					excalibur::resize_cpu(ROI, ROI, std::ceil(ROI->height() * scale), std::ceil(ROI->width() * scale));
				else
					scale = 1.0f;

				point<float> ldmk5[5];
				auto pts = faces[i].pts();
				for (size_t j = 0; j < pts.size(); j++)
				{
					ldmk5[j] = point<float>(pts[j].key() - MarginRect.x, pts[j].value() - MarginRect.y);
				}
				point<float> center_eye = point<float>((ldmk5[0].x + ldmk5[1].x) / 2, (ldmk5[0].y + ldmk5[1].y) / 2);
				point<float> center_mouth = point<float>((ldmk5[3].x + ldmk5[4].x) / 2, (ldmk5[3].y + ldmk5[4].y) / 2);
				point<float> center = point<float>((center_eye.x + center_mouth.x) * scale / 2, (center_eye.y + center_mouth.y) * scale / 2);
				double tan = (center_eye.x - center_mouth.x) / (center_eye.y - center_mouth.y);
				double arctan = atan(tan) * 180 / 3.1415926;

				excalibur::rotate_with_points_cpu(ROI, rotated_ROI, center, -1 * arctan);

				double distance = std::sqrt((center_eye.x - center_mouth.x) * (center_eye.x - center_mouth.x) + (center_eye.y - center_mouth.y) * (center_eye.y - center_mouth.y));

				if (distance < std::numeric_limits<double>::epsilon())
				{
					throw exposing::abi_invalid_argument("Illegal distance. Error landmarks.");
				}

				double cos = (center_mouth.y - center_eye.y) / distance;
				double sin = (center_mouth.x - center_eye.x) / distance;
				point<float> new_center_eye = point<float>(center_eye.x + (float)(sin * distance / 2), (float)(center_eye.y - (1 - cos) * distance / 2));
				point<float> new_center_mouth = point<float>(center_mouth.x - (float)(sin * distance / 2), (float)(center_mouth.y + (1 - cos) * distance / 2));
				rectangle<float> final_rect = rectangle<float>((new_center_eye.x - distance * 1.25f) * scale,
					(new_center_eye.y - distance * 0.75f) * scale,
					distance * 2.5f * scale, distance * 2.5f * scale);
				excalibur::safty_cut_cpu(rotated_ROI, final_mat, &final_rect);

				excalibur::resize_cpu(final_mat, resized_color_img, 128, 128);

				auto temp_vec = exposing::make_param_vector<std::uint8_t>();
				temp_vec.resize(static_cast<size_t>(resized_color_img->count()));
				temp_vec.copy_from({ resized_color_img->cpu_data(), static_cast<size_t>(resized_color_img->count()) }, 0);

				res.push_back(temp_vec);
			}

			return res;
#endif
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

	exposing::param_vector<exposing::param_vector<std::uint8_t>> face_alignment_internal::align128(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
		const exposing::param_vector<longinus::face_info>& faces, std::int32_t order) const
	{
		return impl_->align128(bitmap, channels, height, width, faces, order);
	}

	exposing::param_vector<exposing::param_vector<std::uint8_t>> face_alignment_internal::align(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
		const exposing::param_vector<longinus::face_info>& faces, std::int32_t order) const
	{
		return impl_->align(bitmap, channels, height, width, faces, order);
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
