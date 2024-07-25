#include "detect_code_impl.hpp"
#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include <map>
#include <utility>
#include "../posture/box_info.hpp"
#include "json.h"
#include<iostream>
namespace glasssix::smoke
{
	detect_code_impl::detect_code_impl() = default;

	detect_code_impl::~detect_code_impl() = default;

	void detect_code_impl::init(const exposing::param_string & str_params)
	{
		Json::Reader reader(Json::Features::strictMode());
		Json::Value root;
		if (!reader.parse(exposing::to_narrow_string(str_params), root))
			throw Json::Exception("parse json failed");
		std::string model_directory = root["models_directory"].asString();
		int device = root.get("device", Json::Int(-1)).asInt();
		impl_ = std::make_unique<detect_code_internal>(exposing::to_narrow_string(model_directory), device);
	}

	exposing::param_string detect_code_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_string detect_code_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map) {
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"selene internal object not initialized");

		Json::Reader reader(Json::Features::strictMode());
		Json::FastWriter writer;
		Json::Value root, value;
		if (!reader.parse(exposing::to_narrow_string(exposing::unbox<exposing::param_string>(input_params_map.get_value("params"))), root))
			throw Json::Exception("parse json failed");
		Json::Value params = root.get("dyparams", Json::Value());
		//从json获取人头数据,并赋值给C++对应结构体
		auto posture_info_list = Json::Value(Json::arrayValue);;
		posture_info_list = params["info_list"];
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

		std::map<std::string, float> dynamic_param_map;

		params.removeMember("info_list");
		for (auto& param_name : params.getMemberNames()) {
			dynamic_param_map.try_emplace(param_name, params[param_name].asFloat());
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
		auto result = impl_->detect(std::move(input_data), channels, height, width, roi_x, roi_y, roi_width, roi_height, postures, dynamic_param_map);

		Json::Value jarray_box;
		Json::Value jarray_smoke_detected(Json::arrayValue);			
		Json::Value jarray_normal_detected(Json::arrayValue);

		for (int i = 0; i < result.size(); i++)
		{
			int category = Json::Int(result[i].category());
			Json::Value jarray_key_points = Json::Value(Json::arrayValue);
			if (category == 1)
			{
				jarray_box["x1"] = Json::Int(result[i].x1());
				jarray_box["y1"] = Json::Int(result[i].y1());
				jarray_box["x2"] = Json::Int(result[i].x2());
				jarray_box["y2"] = Json::Int(result[i].y2());
				jarray_box["score"] = Json::Value(result[i].confidence());

				auto key_points = result[i].key_points();
				for (size_t i = 0; i < (int)key_points.size() / 3; i++) {
					Json::Value KPoint;
					KPoint["x"] = Json::Int(key_points[i * 3]);
					KPoint["y"] = Json::Int(key_points[i * 3 + 1]);
					KPoint["point_score"] = Json::Value(key_points[i * 3 + 2]);
					jarray_key_points.append(KPoint);
				}
				jarray_box["key_points"] = jarray_key_points;
				jarray_normal_detected.append(jarray_box);
			}
			else if (category == 0)
			{
				jarray_box["x1"] = Json::Int(result[i].x1());
				jarray_box["y1"] = Json::Int(result[i].y1());
				jarray_box["x2"] = Json::Int(result[i].x2());
				jarray_box["y2"] = Json::Int(result[i].y2());
				jarray_box["score"] = Json::Value(result[i].confidence());
				auto key_points = result[i].key_points();
				for (size_t i = 0; i < (int)key_points.size() / 3; i++) {
					Json::Value KPoint;
					KPoint["x"] = Json::Int(key_points[i * 3]);
					KPoint["y"] = Json::Int(key_points[i * 3 + 1]);
					KPoint["point_score"] = Json::Value(key_points[i * 3 + 2]);
					jarray_key_points.append(KPoint);
				}
				jarray_box["key_points"] = jarray_key_points;

				jarray_smoke_detected.append(jarray_box);
			}
		}
		Json::Value jarray_info;

		jarray_info["norm_list"] = jarray_normal_detected;
		jarray_info["smoke_list"] = jarray_smoke_detected;
		value["detect_info"] = jarray_info;

		return exposing::to_param_string(writer.write(value));
	}

}

