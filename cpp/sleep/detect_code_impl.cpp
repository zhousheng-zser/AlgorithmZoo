#include "detect_code_impl.hpp"
#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include <map>
#include <utility>
#include "json.h"
#include <iostream>
namespace glasssix::sleep
{
	detect_code_impl::detect_code_impl() = default;

	detect_code_impl::~detect_code_impl() = default;

	void detect_code_impl::init(const exposing::param_string& str_params)
	{
		Json::Reader reader(Json::Features::strictMode());
		Json::Value root;
		if (!reader.parse(exposing::to_narrow_string(str_params), root))
			throw Json::Exception("parse json failed");
		std::string models_directory = root["models_directory"].asString();
		int device = root.get("device", Json::Int(-1)).asInt();

		impl_ = std::make_unique<detect_code_internal>(exposing::to_narrow_string(models_directory), device);
	}

	exposing::param_string detect_code_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_string detect_code_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map  ) 
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"pedestrian internal object not initialized");

		Json::Reader reader(Json::Features::strictMode());
		Json::FastWriter writer;
		Json::Value root, value;
		if (!reader.parse(exposing::to_narrow_string(exposing::unbox<exposing::param_string>(input_params_map.get_value("params"))), root))
			throw Json::Exception("parse json failed");

		std::cout << root.toStyledString() << std::endl;
		Json::Value params = root.get("dyparams", Json::Value());

		std::map<std::string, float> dparam_map;

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

		auto result = impl_->detect(input_data, channels, height, width, roi_x, roi_y, roi_width, roi_height, dparam_map);

		Json::Value jarray_work_detected(Json::arrayValue);
		Json::Value jarray_lying_detected(Json::arrayValue);
		Json::Value jarray_box;
		for (int i = 0; i < result.size(); i++)
		{
			int category = Json::Int(result[i].category());
			jarray_box["x1"] = Json::Int(result[i].x1());
			jarray_box["y1"] = Json::Int(result[i].y1());
			jarray_box["x2"] = Json::Int(result[i].x2());
			jarray_box["y2"] = Json::Int(result[i].y2());
			jarray_box["score"] = Json::Value(result[i].confidence());
			
			if( category)
				jarray_work_detected.append(jarray_box);
			else
				jarray_lying_detected.append(jarray_box);
		}

		value["detect_info"]["work_list"] = jarray_work_detected;
		value["detect_info"]["lying_list"] = jarray_lying_detected;
		return exposing::to_param_string(writer.write(value));
	}

}
