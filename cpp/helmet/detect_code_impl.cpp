#include "detect_code_impl.hpp"
#include "detect_code_internal.hpp"
#include <map>
#include <utility>
#include "json.h"

namespace glasssix::helmet
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
		impl_ = std::make_unique<detect_code_internal>(str_params);
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
		auto head_info_list = root["head_info_list"];
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
		auto result = impl_->detect(std::move(input_data), channels, height, width, roi_x, roi_y, roi_width, roi_height, heads, dynamic_param_map);

		Json::Value jarray_box;
		Json::Value jarray_helmet_detected(Json::arrayValue);			
		Json::Value jarray_hat_detected(Json::arrayValue);
		Json::Value jarray_head_detected(Json::arrayValue);

		for (int i = 0; i < result.size(); i++)
		{
			int category = Json::Int(result[i].category());

			if (category == 0)
			{
				jarray_box["x1"] = Json::Int(result[i].x1());
				jarray_box["y1"] = Json::Int(result[i].y1());
				jarray_box["x2"] = Json::Int(result[i].x2());
				jarray_box["y2"] = Json::Int(result[i].y2());
				jarray_box["score"]= Json::Value(result[i].score());
				jarray_helmet_detected.append(jarray_box);
			}
			else if (category == 1)
			{
				jarray_box["x1"] = Json::Int(result[i].x1());
				jarray_box["y1"] = Json::Int(result[i].y1());
				jarray_box["x2"] = Json::Int(result[i].x2());
				jarray_box["y2"] = Json::Int(result[i].y2());
				jarray_box["score"]= Json::Value(result[i].score());
				jarray_hat_detected.append(jarray_box);
			}
			else if (category == 2)
			{
				jarray_box["x1"] = Json::Int(result[i].x1());
				jarray_box["y1"] = Json::Int(result[i].y1());
				jarray_box["x2"] = Json::Int(result[i].x2());
				jarray_box["y2"] = Json::Int(result[i].y2());
				jarray_box["score"]= Json::Value(result[i].score());
				jarray_head_detected.append(jarray_box);
			}
		}

		Json::Value jarray_info;

		jarray_info["with_helmet_list"] = jarray_helmet_detected;
		jarray_info["with_hat_list"] = jarray_hat_detected;
		jarray_info["head_list"] = jarray_head_detected;


		value["detect_info"] = jarray_info;

		return exposing::to_param_string(writer.write(value));
	}

	exposing::param_vector<helmet::box_info> detect_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y,
         int roi_width, int roi_height, exposing::param_vector<head::box_info> head_info_list, const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"helmet internal object not initialized");

		std::map<std::string, float> param_map;

		for (auto it : param_map_abi) {
			param_map.insert(std::make_pair(it.key(), it.value()));
		}

		return impl_->detect(std::move(bitmap), channels, height, width, roi_x, roi_y, roi_width, roi_height, head_info_list, param_map);
	}

}
