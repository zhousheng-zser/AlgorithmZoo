#include "anti_spoofing_impl.hpp"
#include "anti_spoofing_internal.hpp"
#include  <json.h>

namespace glasssix::damocles
{
	anti_spoofing_impl::anti_spoofing_impl()
	{
	}

	anti_spoofing_impl::~anti_spoofing_impl()
	{
	}

	void anti_spoofing_impl::init(const exposing::param_string& str_params)
	{
		Json::Reader reader(Json::Features::strictMode());
		Json::Value root;
		if (!reader.parse(exposing::to_narrow_string(str_params), root))
			throw Json::Exception("parse json failed");
		std::string models_directory = root["models_directory"].asString();
		int model_type = root.get("model_type", Json::Int(1)).asInt();
		int device = root.get("device", Json::Int(-1)).asInt();

		impl_ = std::make_unique<anti_spoofing_internal>(exposing::to_narrow_string(models_directory), model_type, device);
	}


	exposing::param_string anti_spoofing_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_string anti_spoofing_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map)
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"longinus internal object not initialized");

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

		int command = root["command"].asInt();
		switch (command)
		{
		case 0://anti_spoofing
			{
				auto faces = exposing::make_param_vector<longinus::face_info, 1>();
				for (const auto& i : root["facerect_list"])
				{
					auto face = exposing::make_exported_interface<longinus::face_info>();
					face.set_x(i["x"].asFloat());
					face.set_y(i["y"].asFloat());
					face.set_height(i["height"].asFloat());
					face.set_width(i["width"].asFloat());
					faces.push_back(face);
				}
				auto res = impl_->spoofing_detect(faces, input_data, data_shape[1], data_shape[2], data_shape[3], order);
				Json::Value res_array = Json::Value(Json::arrayValue);
				for (size_t i = 0; i < res.size(); i++)
				{
					Json::Value prob_array = Json::Value(Json::arrayValue);
					for (size_t j = 0; j < res[i].size(); j++)
						prob_array.append(res[i][j]);
					res_array.append(prob_array);
				}
				value["spoofing_result"] = res_array;
			}
			break;
		case 1://presentation_attack_detect
			{
				int action_cmd = root["action_cmd"].asInt();
				auto face = exposing::make_exported_interface<longinus::face_info>();
				face.set_x(root["facerect"]["x"].asFloat());
				face.set_y(root["facerect"]["y"].asFloat());
				face.set_height(root["facerect"]["height"].asFloat());
				face.set_width(root["facerect"]["width"].asFloat());

				bool ret = impl_->presentation_attack_detect(action_cmd, face, input_data, data_shape[1], data_shape[2], data_shape[3], order);
				value["presentation_attack_result"] = Json::Value(ret);
			}
			break;
		default:
			break;
		}
		value["command"] = root["command"];
		return exposing::to_param_string(writer.write(value));
	}

	exposing::param_vector<exposing::param_vector<float>> anti_spoofing_impl::spoofing_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"damocles internal object not initialized");

		auto native_result = impl_->spoofing_detect(faces, bitmap, channels, height, width, order);
		auto result = exposing::make_param_vector<float, 2>();
		for (const auto& item : native_result)
		{
			result.push_back(exposing::make_param_vector<float>(item));
		}

		return result;
	}
	bool anti_spoofing_impl::presentation_attack_detect(int action_cmd, const longinus::face_info& face, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"damocles internal object not initialized");

		return impl_->presentation_attack_detect(action_cmd, face, bitmap, channels, height, width, order);
	}
}
