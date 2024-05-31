#include "detect_code_impl.hpp"
#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include <map>
#include <utility>
#include "json.h"
#include <iostream>

namespace glasssix::onphone
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

	exposing::param_string detect_code_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map)
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"onphone internal object not initialized");

		Json::Reader reader(Json::Features::strictMode());
		Json::FastWriter writer;
		Json::Value root, value;
		if (!reader.parse(exposing::to_narrow_string(exposing::unbox<exposing::param_string>(input_params_map.get_value("params"))), root))
			throw Json::Exception("parse json failed");
		Json::Value params = root.get("dyparams", Json::Value());
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

		int command = root["command"].asInt();
		switch (command)
		{
		case 0:
		{
			//从json获取人头数据,并赋值给C++对应结构体
			auto head_info_list = Json::Value(Json::arrayValue);;
			head_info_list = params["head_info_list"];
			auto heads = exposing::make_param_vector<head::box_info>();
			for (auto p : head_info_list)
			{
				auto head = exposing::make_exported_interface<head::box_info>();
				head.set_x1(p["x1"].asInt());
				head.set_y1(p["y1"].asInt());
				head.set_x2(p["x2"].asInt());
				head.set_y2(p["y2"].asInt());
				head.set_score(p["score"].asFloat());
				heads.push_back(head);
			}
			std::map<std::string, float> dynamic_param_map;
			params.removeMember("head_info_list");
			for (auto& param_name : params.getMemberNames()) {
				dynamic_param_map.try_emplace(param_name, params[param_name].asFloat());
			}
			auto result = impl_->detect(std::move(input_data), channels, height, width, roi_x, roi_y, roi_width, roi_height, heads, dynamic_param_map);
			Json::Value jarray_normal_detected(Json::arrayValue);
			Json::Value jarray_onphone_detected(Json::arrayValue);
			for (int i = 0; i < result.size(); i++)
			{
				float category = result[i].category();
				Json::Value jarray_box;
				jarray_box["x1"] = Json::Int(result[i].x1());
				jarray_box["y1"] = Json::Int(result[i].y1());
				jarray_box["x2"] = Json::Int(result[i].x2());
				jarray_box["y2"] = Json::Int(result[i].y2());
				jarray_box["head_score"] = Json::Value(result[i].confidence());
				if (category <= 0.5f)
				{
					jarray_normal_detected.append(jarray_box);
				}
				else if (category > 0.5f)
				{
					jarray_box["phone_list"] = Json::Value(Json::arrayValue);

					Json::Value phone_info;
					auto phone_loacl_list = result[i].phonelocal_list();
					auto phone_score_list = result[i].phonescore_list();

					if (phone_loacl_list.size() == 4 * phone_score_list.size())
					{
						for (size_t i = 0; i < phone_score_list.size(); i++)
						{
							phone_info["x1"] = Json::Int(phone_loacl_list[i * 4 + 0]);
							phone_info["y1"] = Json::Int(phone_loacl_list[i * 4 + 1]);
							phone_info["x2"] = Json::Int(phone_loacl_list[i * 4 + 2]);
							phone_info["y2"] = Json::Int(phone_loacl_list[i * 4 + 3]);
							phone_info["phone_score"] = Json::Value(phone_score_list[i]);
						}
						jarray_box["phone_list"].append(phone_info);
					}
					jarray_onphone_detected.append(jarray_box);
				}
			}
			value["detect_info"]["norm_list"] = jarray_normal_detected;
			value["detect_info"]["onphone_list"] = jarray_onphone_detected;
			value["command"] = root["command"];
			break;
		}
		case 1:
		{
			//从json获取姿态数据,并赋值给C++对应结构体
			auto posture_info_list = Json::Value(Json::arrayValue);;
			posture_info_list = params["posture_info_list"];
			auto postures = exposing::make_param_vector<posture::box_info>();
			for (auto p : posture_info_list)
			{
				auto posture = exposing::make_exported_interface<posture::box_info>();
				auto key_points = exposing::make_param_vector<float>();
				auto pts = p["key_points"];
				for (auto j : pts)
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
			params.removeMember("posture_info_list");
			for (auto& param_name : params.getMemberNames()) {
				dynamic_param_map.try_emplace(param_name, params[param_name].asFloat());
			}
			auto result = impl_->exdetect(std::move(input_data), channels, height, width, roi_x, roi_y, roi_width, roi_height, postures, dynamic_param_map);

			Json::Value jarray_normal_detected(Json::arrayValue);
			Json::Value jarray_onphone_detected(Json::arrayValue);

			for (int i = 0; i < result.size(); i++)
			{
				float category = result[i].category();
				Json::Value jarray_box;

				jarray_box["x1"] = Json::Int(result[i].x1());
				jarray_box["y1"] = Json::Int(result[i].y1());
				jarray_box["x2"] = Json::Int(result[i].x2());
				jarray_box["y2"] = Json::Int(result[i].y2());
				jarray_box["head_score"] = Json::Value(result[i].confidence());
				if (category <= 0.5f)
				{
					jarray_normal_detected.append(jarray_box);
				}
				else if (category > 0.5f)
				{
					jarray_box["phone_list"] = Json::Value(Json::arrayValue);

					Json::Value phone_info;
					auto phone_loacl_list = result[i].phonelocal_list();
					auto phone_score_list = result[i].phonescore_list();

					if (phone_loacl_list.size() == 4 * phone_score_list.size())
					{
						for (size_t i = 0; i < phone_score_list.size(); i++)
						{
							phone_info["x1"] = Json::Int(phone_loacl_list[i * 4 + 0]);
							phone_info["y1"] = Json::Int(phone_loacl_list[i * 4 + 1]);
							phone_info["x2"] = Json::Int(phone_loacl_list[i * 4 + 2]);
							phone_info["y2"] = Json::Int(phone_loacl_list[i * 4 + 3]);
							phone_info["phone_score"] = Json::Value(phone_score_list[i]);
						}
						jarray_box["phone_list"].append(phone_info);
					}

					jarray_onphone_detected.append(jarray_box);
				}
			}

			value["detect_info"]["norm_list"] = jarray_normal_detected;
			value["detect_info"]["onphone_list"] = jarray_onphone_detected;
			value["command"] = root["command"];
			break;
		}
		default:
			break;
		}
		return exposing::to_param_string(writer.write(value));
	}

	//exposing::param_vector<onphone::box_info> detect_code_impl::exdetect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height,
	//	exposing::param_vector<posture::box_info> posture_info_list,
	//	const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const
	//{
	//	if (!impl_)
	//		throw exposing::abi_invalid_operation(u8"onphone internal object not initialized");

	//	std::map<std::string, float> param_map;

	//	for (auto it : param_map_abi) {
	//		param_map.insert(std::make_pair(it.key(), it.value()));
	//	}

	//	return impl_->exdetect(std::move(bitmap), channels, height, width, roi_x, roi_y, roi_width, roi_height, posture_info_list, param_map);
	//}

}
