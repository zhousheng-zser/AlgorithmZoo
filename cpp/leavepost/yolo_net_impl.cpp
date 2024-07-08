#include "yolo_net_impl.hpp"
#include "yolo_net_internal.hpp"
#include "box_info_impl.hpp"
#include "json.h"

namespace glasssix::leavepost
{
	yolo_net_impl::yolo_net_impl()
	{
	}

	yolo_net_impl::~yolo_net_impl()
	{
	}

	void yolo_net_impl::init(const exposing::param_string & str_params)
	{
		impl_ = std::make_unique<yolo_net_internal>(str_params);
	}
	exposing::param_string yolo_net_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_string yolo_net_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map) {
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"selene internal object not initialized");

		Json::Reader reader(Json::Features::strictMode());
		Json::FastWriter writer;
		Json::Value root, value;
		if (!reader.parse(exposing::to_narrow_string(exposing::unbox<exposing::param_string>(input_params_map.get_value("params"))), root))
			throw Json::Exception("parse json failed");
		Json::Value params = root.get("dyparams", Json::Value());

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
		auto result = impl_->detect(std::move(input_data), channels, height, width, roi_x, roi_y, roi_width, roi_height, dynamic_param_map);

		Json::Value jarray_box;
		Json::Value jarray_info;
		Json::Value jarray_work = Json::Value(Json::arrayValue);

		for (auto obj : result)
		{
			int category = Json::Int(obj.label());
			if (category == 0 || category == 1)
			{
				jarray_box["x1"] = Json::Int(obj.x());
				jarray_box["y1"] = Json::Int(obj.y());
				jarray_box["x2"] = Json::Int(obj.x() + obj.width());
				jarray_box["y2"] = Json::Int(obj.height() + obj.y());
				jarray_box["score"] = Json::Value(obj.confidence());
				// jarray_box["label"] = Json::Value(obj.label());
				jarray_work.append(jarray_box);
			}


		}
		jarray_info["hat_list"] = jarray_work;

		value["detect_info"] = jarray_info;

		return exposing::to_param_string(writer.write(value));
	}

	exposing::param_vector<box_info> yolo_net_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y,
         int roi_width, int roi_height,const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"leavepost internal object not initialized");

		std::map<std::string, float> param_map;

		for (auto it : param_map_abi) {
			param_map.insert(std::make_pair(it.key(), it.value()));
		}

		return impl_->detect(std::move(bitmap), channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map);
	}
}
