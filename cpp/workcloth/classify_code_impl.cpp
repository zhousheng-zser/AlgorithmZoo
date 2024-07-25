#include "classify_code_impl.hpp"
#include "classify_code_internal.hpp"
#include "box_info_impl.hpp"
#include <map>
#include <utility>
#include "json.h"

namespace glasssix::workcloth
{
	classify_code_impl::classify_code_impl() = default;

	classify_code_impl::~classify_code_impl() = default;

	inline void change_color_hsv_cfg(std::unordered_map<int, std::vector<cv::Scalar>> &color_hsv_cfg , int id, const exposing::param_vector<int>& temp) {
		for (int i = 0; i < temp.size(); i += 3) {
			color_hsv_cfg[id].push_back(cv::Scalar{ double(temp[i]),double(temp[i + 1]) ,double(temp[i + 2]) });
		}
	}

	void classify_code_impl::init(const exposing::param_string& str_params)
	{
		Json::Reader reader(Json::Features::strictMode());
		Json::Value root;
		if (!reader.parse(exposing::to_narrow_string(str_params), root))
			throw Json::Exception("parse json failed");
		std::string models_directory = root["models_directory"].asString();
		int device = root.get("device", Json::Int(-1)).asInt();

		impl_ = std::make_unique<classify_code_internal>(exposing::to_narrow_string(models_directory), device);
	}

	exposing::param_string classify_code_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}
	
	// exposing::param_vector<workcloth::box_info> classify_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, exposing::param_vector<posture::box_info> posture_info_list,
	// 	const exposing::param_hash_map<exposing::param_string, float>& param_map_abi, 
	// 	const exposing::param_hash_map<exposing::param_string, exposing::param_vector<int>>& color_hsv_cfg_abi) const
	// {
	// 	if (!impl_)
	// 		throw exposing::abi_invalid_operation(u8"workcloth internal object not initialized");

	// 	std::map<std::string, float> param_map;

	// 	for (auto it : param_map_abi) {
	// 		param_map.insert(std::make_pair(it.key(), it.value()));
	// 	}
	// 	std::unordered_map<int, std::vector<cv::Scalar>> color_hsv_cfg;
	// 	change_color_hsv_cfg(color_hsv_cfg, 0, color_hsv_cfg_abi.get_value("black"));
	// 	change_color_hsv_cfg(color_hsv_cfg, 1, color_hsv_cfg_abi.get_value("grey"));
	// 	change_color_hsv_cfg(color_hsv_cfg, 2, color_hsv_cfg_abi.get_value("white"));
	// 	change_color_hsv_cfg(color_hsv_cfg, 3, color_hsv_cfg_abi.get_value("red"));
	// 	change_color_hsv_cfg(color_hsv_cfg, 4, color_hsv_cfg_abi.get_value("orange"));
	// 	change_color_hsv_cfg(color_hsv_cfg, 5, color_hsv_cfg_abi.get_value("yellow"));
	// 	change_color_hsv_cfg(color_hsv_cfg, 6, color_hsv_cfg_abi.get_value("green"));
	// 	change_color_hsv_cfg(color_hsv_cfg, 7, color_hsv_cfg_abi.get_value("cyan"));
	// 	change_color_hsv_cfg(color_hsv_cfg, 8, color_hsv_cfg_abi.get_value("blue"));
	// 	change_color_hsv_cfg(color_hsv_cfg, 9, color_hsv_cfg_abi.get_value("purple"));

	// 	return impl_->detect(std::move(bitmap), channels, height, width, roi_x, roi_y, roi_width, roi_height, posture_info_list, param_map, color_hsv_cfg);
	// }


	exposing::param_string classify_code_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map  ) 
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"pedestrian internal object not initialized");

		Json::Reader reader(Json::Features::strictMode());
		Json::FastWriter writer;
		Json::Value root, value;
		if (!reader.parse(exposing::to_narrow_string(exposing::unbox<exposing::param_string>(input_params_map.get_value("params"))), root))
			throw Json::Exception("parse json failed");

		// std::cout << root.toStyledString() << std::endl;
		Json::Value params = root.get("dyparams", Json::Value());

		std::map<std::string, float> dparam_map;

			auto posture_info_list = Json::Value(Json::arrayValue);
		posture_info_list = params["posture_info_list"];

		//std::cout << posture_info_list.toStyledString() << std::endl;
		
		auto postures = exposing::make_param_vector<posture::box_info>();

		for (auto p : posture_info_list)
		{
			auto posture = exposing::make_exported_interface<posture::box_info>();
			auto key_points = exposing::make_param_vector<float>();
			auto pts = p["key_points"];
			for(auto j : pts)
			{
				key_points.push_back(j["x"].asInt());
				key_points.push_back(j["y"].asInt());
				key_points.push_back(j["point_score"].asFloat());
			}
			posture.set_x1(p["location"]["x1"].asInt());
			posture.set_x2(p["location"]["x2"].asInt());
			posture.set_y1(p["location"]["y1"].asInt());
			posture.set_y2(p["location"]["y2"].asInt());
			posture.set_score(p["score"].asFloat());
			posture.set_key_points(key_points);
			postures.push_back(posture);
		}

		Json::Value color_hsv_cfg = params.get("color_hsv_cfg", Json::Value());

		auto color_hsv_cfg_abi = exposing::make_param_hash_map<exposing::param_string, exposing::param_vector<int>>();

		// std::cout<<"color_hsv_cfg: "<<color_hsv_cfg.toStyledString();

		for (auto& param_name : color_hsv_cfg.getMemberNames()) {
			glasssix::exposing::param_vector<int> color_array = glasssix::exposing::make_param_vector<int>();
			auto iarray_data = color_hsv_cfg[param_name.c_str()];
			for (auto j : iarray_data)
				color_array.push_back(j.asInt());
			color_hsv_cfg_abi.add_or_update(param_name.c_str(), color_array);
		}


		std::unordered_map<int, std::vector<cv::Scalar>> color_hsv_cfg_input;
		change_color_hsv_cfg(color_hsv_cfg_input, 0, color_hsv_cfg_abi.get_value("black"));
		change_color_hsv_cfg(color_hsv_cfg_input, 1, color_hsv_cfg_abi.get_value("grey"));
		change_color_hsv_cfg(color_hsv_cfg_input, 2, color_hsv_cfg_abi.get_value("white"));
		change_color_hsv_cfg(color_hsv_cfg_input, 3, color_hsv_cfg_abi.get_value("red"));
		change_color_hsv_cfg(color_hsv_cfg_input, 4, color_hsv_cfg_abi.get_value("orange"));
		change_color_hsv_cfg(color_hsv_cfg_input, 5, color_hsv_cfg_abi.get_value("yellow"));
		change_color_hsv_cfg(color_hsv_cfg_input, 6, color_hsv_cfg_abi.get_value("green"));
		change_color_hsv_cfg(color_hsv_cfg_input, 7, color_hsv_cfg_abi.get_value("cyan"));
		change_color_hsv_cfg(color_hsv_cfg_input, 8, color_hsv_cfg_abi.get_value("blue"));
		change_color_hsv_cfg(color_hsv_cfg_input, 9, color_hsv_cfg_abi.get_value("purple"));


		params.removeMember("posture_info_list");
		params.removeMember("color_hsv_cfg");

		for (auto& param_name : params.getMemberNames()) {
			dparam_map.try_emplace(param_name, params[param_name].asFloat());
		}

	

		auto input_data = exposing::unbox<exposing::param_span<std::uint8_t>>(input_params_map.get_value("input_data"));
		auto output_data = exposing::unbox<exposing::param_span<std::uint8_t>>(input_params_map.get_value("output_data"));
		int order = exposing::unbox<int>(input_params_map.get_value("order"));
		auto data_shape = input_params_map.get_value("data_shape").as<exposing::param_vector<int>>();

		int num = data_shape[0];
		int channels = data_shape[1];
		int height = data_shape[2];
		int width = data_shape[3];

		int roi_x = 0;
		int roi_y = 0;
		int roi_width = width;
		int roi_height = height;

		

		auto result = impl_->detect(input_data, channels, height, width, roi_x, roi_y, roi_width, roi_height, postures, dparam_map,color_hsv_cfg_input);

		Json::Value jarray_box;

		Json::Value jarray_workcloth_detected(Json::arrayValue);

		for (int i = 0; i < result.size(); i++)
		{
			jarray_box["x1"] = Json::Int(result[i].x1());
			jarray_box["y1"] = Json::Int(result[i].y1());
			jarray_box["x2"] = Json::Int(result[i].x2());
			jarray_box["y2"] = Json::Int(result[i].y2());

			jarray_box["is_sleeve"] = result[i].is_sleeve();
			// color ratio : black = 0, grey, white, red, orange, yellow, green, cyan, blue, purple
			auto color_ratios = result[i].color_ratios();
			int color_ratios_size = color_ratios.size();
			if (color_ratios_size != 10 && color_ratios_size != 30 && color_ratios_size != 40)continue;

			Json::Value jarray_color_ratios = Json::Value(Json::arrayValue);
			for (size_t i = 0; i < color_ratios_size; i++)
			{				
				jarray_color_ratios.append(Json::Value(color_ratios[i]));
			}
			jarray_box["color_ratios"] = jarray_color_ratios;
			jarray_workcloth_detected.append(jarray_box);
		}


		value["detect_info"]["cloth_list"] = jarray_workcloth_detected;

		value["status"]["message"] = Json::Value("OK");
		value["status"]["code"] = Json::Value(0);
		return exposing::to_param_string(writer.write(value));
	}

}
