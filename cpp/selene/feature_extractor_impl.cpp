#include "feature_extractor_impl.hpp"
#include "feature_extractor_internal.hpp"
#include "../longinus/face_info.hpp"
#include "json.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

namespace glasssix::selene
{
	static inline void safty_cut(cv::Mat& img, cv::Mat& dst, cv::Rect roi)
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

	static inline std::vector<std::uint8_t> align(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
		const exposing::param_vector<longinus::face_info>& faces, std::int32_t order)
	{
		if (bitmap.empty())
		{
			throw exposing::abi_invalid_argument("current frame is empty");
		}
		if (order != 1)
			throw exposing::abi_invalid_argument("Not supported order");

		cv::Mat img(height, width, CV_8UC3, bitmap.data());

		cv::Mat ROI, rotated_ROI, final_mat, final_mat_gray, resized_color_img;
		std::vector<std::uint8_t> res(faces.size() * 3 * 128 * 128);

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

			for (size_t j = 0; j < 3; j++)
				std::copy(img_channel_vec[j].data, img_channel_vec[j].data + 128 * 128, res.begin() + i * 3 * 128 * 128 + j * 128 * 128);
		}

		return res;
	}

	feature_extractor_impl::feature_extractor_impl()
	{
	}

	feature_extractor_impl::~feature_extractor_impl()
	{
	}

	void feature_extractor_impl::init(const exposing::param_string& str_params)
	{
		Json::Reader reader(Json::Features::strictMode());
		Json::Value root;
		if (!reader.parse(exposing::to_narrow_string(str_params), root))
			throw Json::Exception("parse json failed");
		std::string models_directory = root["models_directory"].asString();
		int model_type = root.get("model_type", Json::Int(3)).asInt();
		int device = root.get("device", Json::Int(-1)).asInt();
		bool use_int8 = root.get("use_int8", Json::Value(false)).asBool();

		impl_ = std::make_unique<feature_extractor_internal>(exposing::to_narrow_string(models_directory), model_type, device, use_int8);
	}

	std::int32_t feature_extractor_impl::get_model_type() const
	{
		return impl_->get_model_type();
	}

	exposing::param_string feature_extractor_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_string feature_extractor_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map)
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"selene internal object not initialized");

		Json::Reader reader(Json::Features::strictMode());
		Json::FastWriter writer;
		Json::Value root, value;
		if (!reader.parse(exposing::to_narrow_string(exposing::unbox<exposing::param_string>(input_params_map.get_value("params"))), root))
			throw Json::Exception("parse json failed");

		auto faces = exposing::make_param_vector<longinus::face_info, 1>();
		for (auto & i : root["facerectwithfaceinfo_lists"])
		{
			auto face = exposing::make_exported_interface<longinus::face_info>();
			face.set_x(i["x"].asFloat());
			face.set_y(i["y"].asFloat());
			face.set_height(i["height"].asFloat());
			face.set_width(i["width"].asFloat());

			auto vec_pts = exposing::make_param_vector<exposing::param_pair<float, float>>();
			for (auto & j : i["landmark"])
			{
				auto pts = exposing::make_param_pair(j["x"].asFloat(), j["y"].asFloat());
				vec_pts.push_back(pts);
			}
			face.set_pts(vec_pts);
			faces.push_back(face);
		}

		auto input_data = exposing::unbox<exposing::param_span<std::uint8_t>>(input_params_map.get_value("input_data"));
		int order = exposing::unbox<int>(input_params_map.get_value("order"));
		auto data_shape = input_params_map.get_value("data_shape").as<exposing::param_vector<int>>();

		if(data_shape[0] != 1)
			throw exposing::abi_invalid_argument("data_shape[0] != 1");

		auto aligned = align(input_data, data_shape[1], data_shape[2], data_shape[3], faces, order);

		auto native_result = impl_->get(exposing::param_span<std::uint8_t>(aligned.data(), aligned.size()), faces.size(), 0);

		auto output_data = exposing::unbox<exposing::param_span<std::uint8_t>>(input_params_map.get_value("output_data"));
		if(output_data.size() < faces.size() * 256)
			throw exposing::abi_invalid_argument("output_data.size() < num * 256");

		for (size_t i = 0; i < native_result.size(); i++)
		{
			std::copy(native_result[i].data() + i * 3 * 128 * 128, native_result[i].data() + (i + 1) * 3 * 128 * 128, output_data.data() + i * 256);
		}

		value["dimension"] = Json::Int(256);
		value["num"] = Json::Int(faces.size());

		return exposing::to_param_string(writer.write(value));
	}
}
