#include "detect_code_impl.hpp"
#include "detect_code_internal.hpp"
#include <map>
#include <utility>
#include "json.h"

namespace glasssix::crowd
{
	detect_code_impl::detect_code_impl() = default;

	detect_code_impl::~detect_code_impl() = default;


	void detect_code_impl::init(const exposing::param_string& str_params)
	{
		Json::Reader reader(Json::Features::strictMode());
		Json::Value root;
		if (!reader.parse(exposing::to_narrow_string(str_params), root))
		{
			throw Json::Exception("parse json failed");
		}
		std::string model_directory = root["models_directory"].asString();
		int32_t device = root.get("device", Json::Int(-1)).asInt();
		impl_ = std::make_unique<detect_code_internal>(exposing::to_narrow_string(model_directory), device);
	} 
	exposing::param_string detect_code_impl::execute(exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map)
	{
		if (!impl_)
		{
			throw exposing::abi_invalid_operation(u8"selene internal object not initialized");
		}
		Json::Reader reader(Json::Features::strictMode());
		Json::FastWriter writer;
		Json::Value root, value;
		if (!reader.parse(exposing::to_narrow_string(exposing::unbox<exposing::param_string>(input_params_map.get_value("params"))), root))
			throw Json::Exception("parse json failed");
		Json::Value params = root.get("dyparams", Json::Value());
		std::map<std::string, float> dynamic_params_map;
		for (auto& param_name : params.getMemberNames())
		{
			dynamic_params_map.try_emplace(param_name, params[param_name].asFloat());
		}

		auto input_data = exposing::unbox<exposing::param_span<std::uint8_t>>(input_params_map.get_value("input_data"));
		auto output_data = exposing::unbox<exposing::param_span<std::uint8_t>>(input_params_map.get_value("output_data"));
		auto data_shape = input_params_map.get_value("data_shape").as<exposing::param_vector<int>>();

		int num = data_shape[0];
		int channels = data_shape[1];
		int height = data_shape[2];
		int width = data_shape[3];

		int roi_x = params.get("roi_x", Json::Int(0)).asInt();
		int roi_y = params.get("roi_y", Json::Int(0)).asInt();
		int roi_height = params.get("roi_height", Json::Int(height)).asInt();
		int roi_width = params.get("roi_width", Json::Int(width)).asInt();
		int min_cluster_size = params["min_cluster_size"].asInt();

		auto result = impl_->detect(std::move(input_data), channels, height, width, roi_x, roi_y, roi_width, roi_height, min_cluster_size, dynamic_params_map);

		Json::Value jarray_box;
		Json::Value jarray_head_detected(Json::arrayValue);
		Json::Value jarray_cluster_detected(Json::arrayValue);

		std::unordered_map<int, Json::Value>temp;
		for (int i = 0; i < result.size(); i++)
		{
			// int category = Json::Int(result[i].category());

			{
				auto x1_ = Json::Int(result[i].x1());
				auto y1_ = Json::Int(result[i].y1());
				auto x2_ = Json::Int(result[i].x2());
				auto y2_ = Json::Int(result[i].y2());
				auto category_ = Json::Int(result[i].category());
				jarray_box["x1"] = x1_;
				jarray_box["y1"] = y1_;
				jarray_box["x2"] = x2_;
				jarray_box["y2"] = y2_;
				jarray_box["category"] = category_;
				jarray_head_detected.append(jarray_box);

				if (temp[category_].isNull()) {
					temp[category_] = jarray_box;
				}
				else {
					temp[category_]["x1"] = std::min(temp[category_]["x1"].asInt(), x1_);
					temp[category_]["y1"] = std::min(temp[category_]["y1"].asInt(), y1_);
					temp[category_]["x2"] = std::max(temp[category_]["x2"].asInt(), x2_);
					temp[category_]["y2"] = std::max(temp[category_]["y2"].asInt(), y2_);
				}
			}
		}
		for (auto val : temp) {
			jarray_cluster_detected.append(val.second);
		}

		Json::Value jarray_info;

		jarray_info["head_list"] = jarray_head_detected;
		jarray_info["cluster_list"] = jarray_cluster_detected;

		value["detect_info"] = jarray_info;
		return exposing::to_param_string(writer.write(value));
	}

	exposing::param_string detect_code_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<crowd::box_info> detect_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y,
         int roi_width, int roi_height, int min_cluster_size, const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"crowd internal object not initialized");

		std::map<std::string, float> param_map;

		for (auto it : param_map_abi) {
			param_map.insert(std::make_pair(it.key(), it.value()));
		}

		return impl_->detect(std::move(bitmap), channels, height, width, roi_x, roi_y, roi_width, roi_height, min_cluster_size, param_map);
	}

}
