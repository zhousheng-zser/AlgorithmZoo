#include "svm.h"
#include "face_alignment_internal.hpp"

#include <cmath>
#include <climits>

#include <abi/param_span.hpp>
#include <Primitives/tensor_conversions.hpp>
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

		impl(/*const exposing::param_string& mask_detector_model_path, */const exposing::param_string& antispoofing_model_path, std::int32_t device) : device_{ device }
		{

			{
				std::scoped_lock<std::mutex> lck(svm_mut);
				antispoofer_ = svm_load_model(antispoofing_model_path.data());
			}
			
			if(antispoofer_ == nullptr)
				LOG(FATAL) << "Incorrect param file.";

			/*{
				std::scoped_lock<std::mutex> lck(svm_mut);
				mask_detector_ = svm_load_model(mask_detector_model_path.data());
			}

			if(mask_detector_ == nullptr)
				LOG(FATAL) << "Incorrect param file.";*/

		}

		~impl() 
		{
			svm_free_and_destroy_model(&antispoofer_);
			//svm_free_and_destroy_model(&mask_detector_);
		}

		exposing::param_vector<exposing::param_vector<std::uint8_t>> align(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			const exposing::param_vector<longinus::face_info>& faces, std::int32_t order)
		{
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

				auto* ptr = resized_color_img->cpu_data();
				for (size_t k = 0; k < resized_color_img->count(); k++)
				{
					temp_vec.push_back(ptr[k]);
				}

				res.push_back(temp_vec);
			}

			return res;
		}

		exposing::param_vector<exposing::param_vector<std::uint8_t>> align256(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			const exposing::param_vector<longinus::face_info>& faces, std::int32_t order)
		{
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}
			init_cache(bitmap, channels, height, width, order);

			std::shared_ptr<memory::tensor<uint8_t>> ROI, rotated_ROI, final_mat, final_mat_gray, resized_color_img;
			std::vector<std::shared_ptr<memory::tensor<uint8_t>>> src_vector;
			auto res = exposing::make_param_vector<std::uint8_t, 2>();

			for (size_t i = 0; i < faces.size(); i++)
			{
				src_vector.clear();
				rectangle<int> MarginRect = rectangle<int>(faces[i].x() - faces[i].width() * 0.0f,
					faces[i].y() - faces[i].height() * 0.0f,
					faces[i].height() * 1.0f,
					faces[i].width() * 1.0f);

				excalibur::safty_cut_cpu(cache_, ROI, &MarginRect);

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
				rectangle<float> final_rect = rectangle<float>(new_center_eye.x - distance * 1.25f,
					new_center_eye.y - distance * 0.75f,
					distance * 2.5f, distance * 2.5f);
				excalibur::safty_cut_cpu(rotated_ROI, final_mat, &final_rect);

				excalibur::resize_cpu(final_mat, resized_color_img, 128, 128);

				auto temp_vec = exposing::make_param_vector<std::uint8_t>();

				auto* ptr = resized_color_img->cpu_data();
				for (size_t k = 0; k < resized_color_img->count(); k++)
				{
					temp_vec.push_back(ptr[k]);
				}

				res.push_back(temp_vec);
			}

			return res;
		}

		exposing::param_vector<double> blur_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order)
		{
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}

			cv::Mat img;
			if (channels == 1)
			{
				img = cv::Mat(height, width, CV_8UC1, bitmap.data());
			}
			if (channels == 3 && order == memory::NHWC)
			{
				img = cv::Mat(height, width, CV_8UC3, bitmap.data());
			}
			else
				throw exposing::abi_invalid_argument("Not supported channels or order");

			cv::Mat src, srcBlur, gray1, gray2, gray3, dstImage;
			auto result = exposing::make_param_vector<double>();
			for (const auto &i : faces)
			{
				safty_cut(img, src, cv::Rect(i.x(), i.y(), i.width(), i.height()));
				GaussianBlur(src, srcBlur, cv::Size(3, 3), 0, 0, cv::BORDER_DEFAULT); //��˹�˲�
				cv::convertScaleAbs(srcBlur, src); //ʹ�����Ա任ת����������Ԫ�س�8λ�޷������� ��һ��Ϊ0-255
				if (src.channels() != 1)
				{
					cv::cvtColor(src, gray1, CV_BGR2GRAY);
				}
				else
				{
					gray1 = src.clone();
				}

				cv::Mat tmp_m1, tmp_sd1;    //�����洢��ֵ�ͷ���  
				double m1 = 0, sd1 = 0;
				//ʹ��3x3��Laplacian���Ӿ����˲�  
				cv::Laplacian(gray1, gray2, CV_16S, 3, 1, 0, cv::BORDER_DEFAULT);
				////�鵽0~255  
				cv::convertScaleAbs(gray2, gray3);

				//�����ֵ�ͷ���  
				cv::meanStdDev(gray3, tmp_m1, tmp_sd1);
				m1 = tmp_m1.at<double>(0, 0);     //��ֵ  
				sd1 = tmp_sd1.at<double>(0, 0);       //��׼��  

				result.push_back(sd1 * sd1); //����
			}
			
			return result;
		}

		exposing::param_vector<bool> antispoofing(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
		{
			cv::Mat img;
			if (channels == 3 && order == memory::NHWC)
			{
				img = cv::Mat(height, width, CV_8UC3, const_cast<uchar*>(bitmap.data()));
			}
			else
				throw exposing::abi_invalid_argument("Not supported channels or order");
			
			cv::Mat src;
			auto result = exposing::make_param_vector<bool>();
			for (const auto& i : faces)
			{
				safty_cut(img, src, cv::Rect(i.x(), i.y(), i.width(), i.height()));
				cv::Mat featureMat = pretreatment(src);
				//std::cout << featureMat << std::endl;

				svm_node* features = new svm_node[featureMat.rows + 1];
				for (int i = 0; i < featureMat.rows; i++)
				{
					features[i].index = i + 1;
					features[i].value = featureMat.at<float>(i, 0);
				}
				features[featureMat.rows].index = -1;
				features[featureMat.rows].value = 0;
				double predict = svm_predict(antispoofer_, features);
				delete[] features;

				if (predict == 1.0 && !BlackWhiteDetect(src))
					predict = 0.0;

				result.push_back(predict == 1.0);
			}

			return result;
		}

		//bool mask_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order = 0)
		//{
		//	cv::Mat src;
		//	if (channels == 3 && order == memory::NHWC)
		//	{
		//		cv::Mat img(height, width, CV_8UC3, const_cast<uchar*>(bitmap.data()));
		//		safty_cut(img, src, cv::Rect(face.x(), face.y(), face.width(), face.height()));
		//	}
		//	else
		//		throw exposing::abi_invalid_argument("Not supported channels or order");

		//	auto landmark = face.pts();
		//	cv::Point face_start(face.x(), face.y());
		//	std::vector<cv::Point> vec;
		//	vec.emplace_back(landmark[2].key() - face_start.x, landmark[2].value() - face_start.y);
		//	vec.emplace_back(landmark[3].key() - face_start.x, landmark[3].value() - face_start.y);
		//	vec.emplace_back(landmark[4].key() - face_start.x, landmark[4].value() - face_start.y);

		//	std::sort(vec.begin(), vec.end(), [](const cv::Point& first, const cv::Point& second) {return first.y < second.y; });
		//	int topy = vec[2].y;
		//	int bottomy = vec[0].y;
		//	std::sort(vec.begin(), vec.end(), [](const cv::Point& first, const cv::Point& second) {return first.x < second.x; });
		//	int rightx = vec[2].x;
		//	int leftx = vec[0].x;

		//	int w = rightx - leftx;
		//	int h = topy - bottomy;

		//	w = 1.3 * w;
		//	int pad = int(0.15 * w);
		//	leftx = leftx - pad;
		//	rightx = rightx + pad;
		//	int max_edge = std::max(w, h);
		//	int min_edge = std::min(w, h);
		//	int padding = (max_edge - min_edge) / 2;
		//	if (h > w)
		//	{
		//		leftx = leftx - padding;
		//		rightx = leftx + max_edge;
		//	}
		//	else if (h < w)
		//	{
		//		bottomy = bottomy - int(0.5 * padding);
		//		topy = bottomy + max_edge;
		//	}

		//	cv::Mat crop;

		//	safty_cut(src, crop, cv::Rect(leftx, bottomy, rightx - leftx + 1, topy - bottomy + 1));
		//	cv::resize(crop, crop, cv::Size(30, 30));

		//	cv::HOGDescriptor hog(cv::Size(30, 30), cv::Size(30, 30), cv::Size(30, 30), cv::Size(5, 5), 9);
		//	std::vector<float> descriptors;//HOG����������
		//	int DescriptorDim = 0;//HOG�����ӵ�ά��

		//	cv::Mat gray;
		//	cv::cvtColor(crop, gray, CV_BGR2GRAY);
		//	hog.compute(gray, descriptors);

		//	DescriptorDim = descriptors.size();
		//	svm_node* node = new svm_node[DescriptorDim + 1];
		//	node[DescriptorDim].index = -1;
		//	for (size_t i = 0; i < DescriptorDim; i++)
		//	{
		//		node[i].value = descriptors[i];
		//		node[i].index = i+1;
		//	}

		//	double value = svm_predict(mask_detector_, node);
		//	delete[] node;

		//	return (value == 1.0);
		//}

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

		inline void calcLBP(cv::Mat& src, cv::Mat& dst)
		{
			dst.create(src.rows - 2, src.cols - 2, CV_8UC1);
			dst.setTo(0);
			for (int i = 1; i < src.rows - 1; i++)
			{
				for (int j = 1; j < src.cols - 1; j++)
				{
					uchar center = src.at<uchar>(i, j);
					unsigned char lbpCode = 0;
					lbpCode |= (src.at<uchar>(i - 1, j - 1) > center) << 7;
					lbpCode |= (src.at<uchar>(i - 1, j) > center) << 6;
					lbpCode |= (src.at<uchar>(i - 1, j + 1) > center) << 5;
					lbpCode |= (src.at<uchar>(i, j + 1) > center) << 4;
					lbpCode |= (src.at<uchar>(i + 1, j + 1) > center) << 3;
					lbpCode |= (src.at<uchar>(i + 1, j) > center) << 2;
					lbpCode |= (src.at<uchar>(i + 1, j - 1) > center) << 1;
					lbpCode |= (src.at<uchar>(i, j - 1) > center) << 0;
					dst.at<uchar>(i - 1, j - 1) = lbpCode;
				}
			}
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

		//�ж�ͼ���Ƿ��Ǻڰ�ͼƬ
		bool BlackWhiteDetect(cv::Mat img)
		{
			cv::Mat resizeImg;
			cv::resize(img, resizeImg, cv::Size(48, 48));
			cv::cvtColor(resizeImg, resizeImg, CV_BGR2RGB);
			std::vector<cv::Mat> RGB_split;
			cv::split(resizeImg, RGB_split);
			cv::Mat diffRG, diffGB, diffRB;
			//��������ͨ��Ԫ�ز�ֵ�ľ���ֵ
			cv::absdiff(RGB_split[0], RGB_split[1], diffRG);
			cv::absdiff(RGB_split[1], RGB_split[2], diffGB);
			cv::absdiff(RGB_split[0], RGB_split[2], diffRB);

			cv::Mat meanRG, meanGB, meanRB, tempStdev;
			cv::meanStdDev(diffRG, meanRG, tempStdev);
			cv::meanStdDev(diffGB, meanGB, tempStdev);
			cv::meanStdDev(diffRB, meanRB, tempStdev);

			double mRG, mGB, mRB;
			mRG = meanRG.at<double>(0, 0);
			mGB = meanGB.at<double>(0, 0);
			mRB = meanRB.at<double>(0, 0);
			if ((mRG < 30 && mGB < 30) && mRB < 30)
			{
				return false;
			}

			return true;
		}

		cv::Mat fft2(cv::Mat src)
		{
			cv::Mat Fourier;
			cv::Mat planes[] = { cv::Mat_<float>(src), cv::Mat::zeros(src.size(),CV_32F) };
			cv::merge(planes, 2, Fourier);
			cv::dft(Fourier, Fourier);
			return Fourier;
		}

		void fftshift(cv::Mat& f)
		{
			int x = (int)floor(f.cols / 2.0);
			int y = (int)floor(f.rows / 2.0);
			int width = f.cols;
			int height = f.rows;
			std::vector<cv::Mat> planes;
			cv::split(f, planes);

			for (size_t i = 0; i < planes.size(); i++)
			{
				cv::Mat tmp0, tmp1, tmp2, tmp3;
				cv::Mat q0(planes[i], cv::Rect(0, 0, width, height - y));
				cv::Mat q1(planes[i], cv::Rect(0, height - y, width, y));
				q0.copyTo(tmp0);
				q1.copyTo(tmp1);
				tmp0.copyTo(planes[i](cv::Rect(0, y, width, height - y)));
				tmp1.copyTo(planes[i](cv::Rect(0, 0, width, y)));

				cv::Mat q2(planes[i], cv::Rect(0, 0, width - x, height));
				cv::Mat q3(planes[i], cv::Rect(width - x, 0, x, height));
				q2.copyTo(tmp2);
				q3.copyTo(tmp3);
				tmp2.copyTo(planes[i](cv::Rect(x, 0, width - x, height)));
				tmp3.copyTo(planes[i](cv::Rect(0, 0, x, height)));
			}
			cv::merge(planes, f);
		}

		cv::Mat fft2d(cv::Mat img)
		{
			cv::Mat gray;
			cv::cvtColor(img, gray, CV_BGR2GRAY);
			std::vector<cv::Mat> channels;
			cv::split(gray, channels);
			cv::Mat f = fft2(channels[0]);
			fftshift(f);
			std::vector<cv::Mat> planes;
			cv::split(f, planes);
			cv::magnitude(planes[0], planes[0], planes[0]);
			cv::magnitude(planes[1], planes[1], planes[1]);
			cv::Mat mag = planes[0] + planes[1];
			mag += cv::Scalar::all(1);
			cv::log(mag, mag);
			cv::Mat show;
			mag.convertTo(show, CV_8U);
			cv::equalizeHist(show, show);
			return show;
		}

		cv::Mat pretreatment(cv::Mat img)
		{
			cv::Mat resizeImg;
			cv::resize(img, resizeImg, cv::Size(80, 80));
			cv::Mat fftImg = fft2d(resizeImg);
			//std::cout << fftImg << std::endl;
			cv::Mat HSV, YCrCb;
			cv::cvtColor(img, HSV, CV_BGR2HSV);
			cv::cvtColor(img, YCrCb, CV_BGR2YCrCb);
			std::vector<cv::Mat> HSV_split, YCrCb_split;
			cv::split(HSV, HSV_split);
			cv::split(YCrCb, YCrCb_split);

			//����LBP����
			cv::Mat fft_lbp, H_lbp, S_lbp, V_lbp, Y_lbp, Cr_lbp, Cb_lbp;
			calcLBP(fftImg, fft_lbp);
			calcLBP(HSV_split[0], H_lbp);
			calcLBP(HSV_split[1], S_lbp);
			calcLBP(HSV_split[2], V_lbp);
			calcLBP(YCrCb_split[0], Y_lbp);
			calcLBP(YCrCb_split[1], Cr_lbp);
			calcLBP(YCrCb_split[2], Cb_lbp);

			//����ֱ��ͼͳ��
			const int histSize = 256;
			float range[] = { 0, 256 };
			const float* ranges[] = { range };
			const int channels = 0;

			cv::Mat fft_hist, H_hist, S_hist, V_hist, Y_hist, Cr_hist, Cb_hist;
			cv::calcHist(&fft_lbp, 1, &channels, cv::Mat(), fft_hist, 1, &histSize, &ranges[0], true, false);
			cv::calcHist(&H_lbp, 1, &channels, cv::Mat(), H_hist, 1, &histSize, &ranges[0], true, false);
			cv::calcHist(&S_lbp, 1, &channels, cv::Mat(), S_hist, 1, &histSize, &ranges[0], true, false);
			cv::calcHist(&V_lbp, 1, &channels, cv::Mat(), V_hist, 1, &histSize, &ranges[0], true, false);
			cv::calcHist(&Y_lbp, 1, &channels, cv::Mat(), Y_hist, 1, &histSize, &ranges[0], true, false);
			cv::calcHist(&Cb_lbp, 1, &channels, cv::Mat(), Cb_hist, 1, &histSize, &ranges[0], true, false);
			cv::calcHist(&Cr_lbp, 1, &channels, cv::Mat(), Cr_hist, 1, &histSize, &ranges[0], true, false);

			cv::Mat concatMat;
			cv::vconcat(fft_hist, H_hist, concatMat);
			cv::vconcat(concatMat, S_hist, concatMat);
			cv::vconcat(concatMat, V_hist, concatMat);
			cv::vconcat(concatMat, Y_hist, concatMat);
			cv::vconcat(concatMat, Cb_hist, concatMat);
			cv::vconcat(concatMat, Cr_hist, concatMat);

			return concatMat;
		}
		inline static std::mutex svm_mut;
		struct svm_model* antispoofer_;
		//struct svm_model* mask_detector_;

		int device_;
		std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
	};


	face_alignment_internal::face_alignment_internal(/*const exposing::param_string& mask_detector_model_path, */const exposing::param_string& antispoofing_model_path, int device) : impl_{ std::make_unique<impl>(/*mask_detector_model_path, */antispoofing_model_path, device) }
	{
	}

	face_alignment_internal::~face_alignment_internal()
	{
	}

	exposing::param_vector<exposing::param_vector<std::uint8_t>> face_alignment_internal::align(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
		const exposing::param_vector<longinus::face_info>& faces, std::int32_t order) const
	{
		return impl_->align(bitmap, channels, height, width, faces, order);
	}

	exposing::param_vector<exposing::param_vector<std::uint8_t>> face_alignment_internal::align256(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
		const exposing::param_vector<longinus::face_info>& faces, std::int32_t order) const
	{
		return impl_->align256(bitmap, channels, height, width, faces, order);
	}

	exposing::param_vector<double> face_alignment_internal::blur_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
	{
		return impl_->blur_detect(faces, bitmap, channels, height, width, order);
	}

	exposing::param_vector<bool> face_alignment_internal::antispoofing(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
	{
		return impl_->antispoofing(faces, bitmap, channels, height, width, order);
	}

	//exposing::param_vector<bool> face_alignment_internal::mask_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
	//{
	//	return impl_->mask_detect(faces, bitmap, channels, height, width, order);
	//}

	exposing::param_vector<double> face_alignment_internal::mask_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
	{
		return impl_->mask_detect(faces, bitmap, channels, height, width, order);
	}

	std::string face_alignment_internal::version()
	{
		return impl::version();
	}
}
