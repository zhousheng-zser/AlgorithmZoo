#include "face_alignment_impl.hpp"
#include "face_alignment_internal.hpp"
#include <json.h>

namespace glasssix::romancia
{
	face_alignment_impl::face_alignment_impl()
	{
	}

	face_alignment_impl::~face_alignment_impl()
	{
	}
	void face_alignment_impl::init(const exposing::param_string& str_params)
	{
		Json::Reader reader(Json::Features::strictMode());
		Json::Value root;
		if (!reader.parse(exposing::to_narrow_string(str_params), root))
			throw Json::Exception("parse json failed");

		std::string models_directory = root["models_directory"].asString();
		int device = root.get("device", Json::Int(-1)).asInt();

		impl_ = std::make_unique<face_alignment_internal>(exposing::to_param_string(models_directory + "/blur_detection_best.racy"), device);
	}

	exposing::param_string face_alignment_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_string face_alignment_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map)
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

		int channels = data_shape[1];
		int height = data_shape[2];
		int width = data_shape[3];

		if (data_shape[0] != 1)
			throw exposing::abi_invalid_argument("data_shape[0] != 1");

		int command = root["command"].asInt();
		switch (command)
		{
		case 0:
			{
				auto faces = exposing::make_param_vector<longinus::face_info, 1>();
				for (auto& i : root["facerect_list"])
				{
					auto face = exposing::make_exported_interface<longinus::face_info>();
					face.set_x(i["x"].asFloat());
					face.set_y(i["y"].asFloat());
					face.set_height(i["height"].asFloat());
					face.set_width(i["width"].asFloat());
					faces.push_back(face);
				}

				auto res = impl_->blur_detect(faces, input_data, channels, height, width, order);

				for (size_t i = 0; i < res.size(); i++)
				{
					value["clarity"].append(Json::Value(res[i]));
				}
			}
			break;
		case 1:
			{
				float angle = root["angle"].asFloat();
				auto res = impl_->rotate(angle, input_data, channels, height, width, order);
				if (output_data.size() != input_data.size())
					throw exposing::abi_invalid_argument("output_data.size() != input_data.size()");

				res.copy_to(0, output_data);
			}
			break;
		default:
			break;
		}
		value["command"] = root["command"];

		return exposing::to_param_string(writer.write(value));
	}
}
