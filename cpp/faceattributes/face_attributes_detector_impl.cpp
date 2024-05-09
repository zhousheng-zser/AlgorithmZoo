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
#include "Excalibur/operation_safty_cut.hpp"
#include "Excalibur/operation_resize.hpp"
#include "Primitives/tensor_conversions.hpp"
#endif

#include <json.h>

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
		impl() = delete;

		impl(const exposing::param_string &models_directory, std::int32_t device) : device_{device}, instance_{get_model_params("attributes_detection_best"), std::string{models_directory} + "/face_attributes.racy", device}
		{
		}

		~impl()
		{
		}

		
		void  Softmax(float* data, int num )
		{             
			double L2_Sum=0.f;
			for(size_t i=0; i<num; i++) 
			{
				data[i]= ( exp(data[i] ) );
				L2_Sum +=  data[i];
			}
			for(size_t i=0; i<num; i++) 
				data[i] =  data[i] / L2_Sum ;
		}

		exposing::param_vector<face_attribute_info> detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order)
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
			std::vector<cv::Rect> rect_tests;
			cv::Rect test_rect1(96, 71, 163 - 96, 139 - 71);
			cv::Rect test_rect2(406, 52, 476 - 406, 123 - 52);
			cv::Rect test_rect3(435, 2, 499 - 435, 65 - 2);
			rect_tests.push_back(test_rect1);
			rect_tests.push_back(test_rect2);
			rect_tests.push_back(test_rect3);
			cv::Mat img(height, width, CV_8UC3, bitmap.data());

			// std::vector<cv::Mat>  faces;
		
			std::vector<std::uint8_t> temp(faces.size() * forward_input_bytes);
		uchar * data =img.ptr<uchar>();
			for (size_t i = 0; i < faces.size(); i++)
			{
				cv::Mat crop_face;
				cv::Rect2f rect(faces[i].x(), faces[i].y(), faces[i].width(), faces[i].height());
				refine(rect, height, width);
				// cv::Mat crop_face = img(cv::Range(rect.y, rect.y + rect.height), cv::Range(rect.x, rect.x + rect.width));
				#if 1
				mat_safty_cut(img, crop_face, rect);
				#else
				mat_safty_cut(img, crop_face, rect_tests[i]);
				#endif
uchar * data2 =crop_face.ptr<uchar>();

// crop_face= cv::imread("dsds.jpg");
				cv::resize(crop_face, crop_face, cv::Size(forward_input_width, forward_input_height));
				// cv::imwrite("dsdds.jpg",crop_face);
				auto  network_results = instance_.forward(crop_face.data, { 1, crop_face.rows, crop_face.cols,crop_face.channels() }, RKNN_TENSOR_NHWC);

					
			const std::vector<std::string> out_namesss = { "gender", "age", "mask", "glass" };
			auto gender_datas = network_results[out_namesss[0]]->mutable_cpu_data();
			auto age_datas = network_results[out_namesss[1]]->mutable_cpu_data();
			auto mask_datas = network_results[out_namesss[2]]->mutable_cpu_data();
			auto glass_datas = network_results[out_namesss[3]]->mutable_cpu_data();

			// Softmax(gender_datas,2);

				if (crop_face.isContinuous())
					std::copy(crop_face.data, crop_face.data + forward_input_bytes, temp.data() + i * forward_input_bytes);
				else
				{
					for (size_t j = 0; j < crop_face.rows; j++)
						std::copy(crop_face.data, crop_face.data + j * forward_input_channels * forward_input_width, temp.data() + i * forward_input_bytes + j * forward_input_channels * forward_input_width);
				}
			}
			auto network_result = instance_.forward(temp.data(), { static_cast<int>(faces.size()), forward_input_height, forward_input_width, forward_input_channels }, RKNN_TENSOR_NHWC);
			
			const std::vector<std::string> out_namess = { "gender", "age", "mask", "glass" };
			// auto gender_datas = network_result[out_namess[0]]->cpu_data();
			// auto age_datas = network_result[out_namess[1]]->cpu_data();
			// auto mask_datas = network_result[out_namess[2]]->cpu_data();
			// auto glass_datas = network_result[out_namess[3]]->cpu_data();

#if defined(USE_RKNNAPI)
			//std::vector<std::string> out_names = { "gender", "age", "mask", "glass" };
#else
			std::vector<std::string> out_names = { "gender", "age", "mask", "glass" };
#endif
#else
			init_cache(bitmap, channels, height, width, order);
			std::shared_ptr<memory::tensor<uint8_t>> crop_faces(new memory::tensor<uint8_t>(std::vector<int>{static_cast<int>(faces.size()), forward_input_channels, forward_input_height, forward_input_width}, -1, memory::NCHW, nullptr));
			uint8_t* ptr = crop_faces->mutable_cpu_data();
			for (size_t i = 0; i < faces.size(); i++)
			{
				std::shared_ptr<memory::tensor<uint8_t>> crop_face;
				excalibur::rectangle<float> rect(faces[i].x(), faces[i].y(), faces[i].height(), faces[i].width());
				refine(rect, height, width);
				excalibur::safty_cut_cpu(cache_, crop_face, &rect);
				excalibur::resize_cpu(crop_face, crop_face, forward_input_height, forward_input_width);
				std::copy(crop_face->cpu_data(), crop_face->cpu_data() + forward_input_bytes, ptr);
				ptr += forward_input_bytes;
			}

			auto network_result = instance_.forward(cache_ | memory::tensor_convert_to<float>);
			const std::vector<std::string> out_names = { "gender", "age", "mask", "glass" };
#endif

			auto gender_data = network_result[out_names[0]]->cpu_data();
			auto age_data = network_result[out_names[1]]->cpu_data();
			auto mask_data = network_result[out_names[2]]->cpu_data();
			auto glass_data = network_result[out_names[3]]->cpu_data();
			#if 0
			for (size_t i = 0; i < faces.size(); i++)
			{
				std::cout << "gender : " << *(gender_data + i * 2) << " " << *(gender_data + 1 + i * 2) << std::endl;
				std::cout << "age : " << *(age_data + i * 4) << " " << *(age_data + 1 + i * 4) << " " << *(age_data + 2 + i * 4) << " " << *(age_data + 3 + i * 4) << std::endl;
				std::cout << "mask : " << *(mask_data + i * 2) << " " << *(mask_data + 1 + i * 2) << std::endl;
				std::cout << "glass : " << *(glass_data + i * 2) << " " << *(glass_data + 1 + i * 2) << std::endl;
			}
			#else
			#endif
			for (size_t i = 0; i < faces.size(); i++)
			{
				int gender_index = std::max_element(gender_data + i * 2, gender_data + (i + 1) * 2) - gender_data - i * 2;
				int age_index = std::max_element(age_data + i * 4, age_data + (i + 1) * 4) - age_data - i * 4;
				int mask_index = std::max_element(mask_data + i * 2, mask_data + (i + 1) * 2) - mask_data - i * 2;
				int glass_index = std::max_element(glass_data + i * 2, glass_data + (i + 1) * 2) - glass_data - i * 2;
				// std::cout << "index : " << gender_index << " " << age_index << " " << mask_index << " " <<glass_index << std::endl;
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
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
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
			// std::cout << "bounder : "<< rect.x << " " << rect.y << " " << rect.x + rect.width << " " << rect.y + rect.height << std::endl;
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
#else
		static inline void refine(excalibur::rectangle<float>& face, int height, int width)
		{
			float bbw = 0, bbh = 0, maxSide = 0, minSide = 0;
			float h = 0, w = 0;
			float x1 = 0, y1 = 0, x2 = 0, y2 = 0;
			bbw = face.w - 1;
			bbh = face.h - 1;
			x1 = face.x;
			y1 = face.y;

			maxSide = ((bbh > bbw) ? bbh : bbw) * 1.02f + 10;
			x1 = x1 + bbw * 0.5 - maxSide * 0.5;
			y1 = y1 + bbh * 0.5 - maxSide * 0.5;
			face.w = round(maxSide + 1);
			face.h = round(maxSide + 1);
			face.x = round(x1);
			face.y = round(y1);

			//boundary check
			if (face.x < 0)
				face.x = 0;
			if (face.y < 0)
				face.y = 0;
			if (face.x + face.w - 1 > width)
				face.w = width - face.x;
			if (face.y + face.h - 1 > height)
				face.h = height - face.y;

			//minSide = (face.height > face.width) ? face.width : face.height;
			//face.height = minSide;
			//face.width = minSide;
		}

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
#endif

	private:
		int device_;
#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
		rknnwrapper::rknn_wrapper instance_;
#else
		excalibur::pipeline<float> instance_;
		std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
#endif
	};

	face_attributes_detector_impl::face_attributes_detector_impl()
	{
	}

	face_attributes_detector_impl::~face_attributes_detector_impl()
	{
	}
	void face_attributes_detector_impl::init(const exposing::param_string& str_params)
	{
		Json::Reader reader(Json::Features::strictMode());
		Json::Value root;
		if (!reader.parse(exposing::to_narrow_string(str_params), root))
			throw Json::Exception("parse json failed");
		std::string models_directory = root["models_directory"].asString();
		int device = root.get("device", Json::Int(-1)).asInt();

		impl_ = std::make_unique<impl>(exposing::to_param_string(models_directory), device);
	}

	exposing::param_string face_attributes_detector_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_string face_attributes_detector_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map)
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"romancia internal object not initialized");

		Json::Reader reader(Json::Features::strictMode());
		Json::FastWriter writer;
		Json::Value root, value;
		if (!reader.parse(exposing::to_narrow_string(exposing::unbox<exposing::param_string>(input_params_map.get_value("params"))), root))
			throw Json::Exception("parse json failed");

		auto input_data = exposing::unbox<exposing::param_span<std::uint8_t>>(input_params_map.get_value("input_data"));
		auto output_data = exposing::unbox<exposing::param_span<std::uint8_t>>(input_params_map.get_value("output_data"));
		int order = exposing::unbox<int>(input_params_map.get_value("order"));
		auto data_shape = input_params_map.get_value("data_shape").as<exposing::param_vector<int>>();

		if (data_shape[0] != 1)
			throw exposing::abi_invalid_argument("data_shape[0] != 1");

		auto faces = exposing::make_param_vector<longinus::face_info, 1>();
		for (const auto& i : root["facerect_list"])
		{
			auto face = exposing::make_exported_interface<longinus::face_info>();
			face.set_x(i["x"].asFloat());
			face.set_y(i["y"].asFloat());
			face.set_height(i["height"].asFloat());
			face.set_width(i["width"].asFloat());
			face.set_ori_x(i["ori_x"].asFloat());
			face.set_ori_y(i["ori_y"].asFloat());
			face.set_ori_height(i["ori_height"].asFloat());
			face.set_ori_width(i["ori_width"].asFloat());
			faces.push_back(face);
		}
		auto res = impl_->detect(faces, input_data, data_shape[1], data_shape[2], data_shape[3], order);

		Json::Value res_array = Json::Value(Json::arrayValue);
		for (const auto& i : res)
		{
			Json::Value attr;
			attr["age"] = Json::Int(i.age());
			attr["gender"] = Json::Int(i.gender());
			attr["mask"] = Json::Int(i.mask());
			attr["glass"] = Json::Int(i.glass());
			res_array.append(attr);
		}

		value["face_attributes"] = res_array;
		return exposing::to_param_string(writer.write(value));
	}

	exposing::param_vector<face_attribute_info> face_attributes_detector_impl::detect(const exposing::param_vector<longinus::face_info> &faces, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"face_attributes_detector_internal object not initialized");

		return impl_->detect(faces, bitmap, channels, height, width, order);
	}
}
