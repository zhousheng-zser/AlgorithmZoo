#include "detect_code_impl.hpp"
#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"

#include <utility>

namespace glasssix::wander
{
	
	detect_code_impl::detect_code_impl() = default;

	detect_code_impl::~detect_code_impl() = default;


	void detect_code_impl::init(const exposing::param_string& str_params)
	{
		impl_ = std::make_unique<detect_code_internal>(exposing::param_string(str_params));
	}
	exposing::param_string detect_code_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map)
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"selene internal object not initialized");

		Json::Reader reader(Json::Features::strictMode());
		Json::FastWriter writer;
		Json::Value root, value;
		if (!reader.parse(exposing::to_narrow_string(exposing::unbox<exposing::param_string>(input_params_map.get_value("params"))), root))
			throw Json::Exception("parse json failed");
		Json::Value params = root.get("dyparams", Json::Value());

		std::map<std::string, float> dynamic_param_map;

		//从json获取行人数据,并赋值给C++对应结构体
		auto pedestrain_info = Json::Value(Json::arrayValue);
		
		pedestrain_info = params["person_list"];
		auto pedestrians = exposing::make_param_vector<pedestrian::box_info>();
		for (auto p : pedestrain_info)
		{
			auto head = exposing::make_exported_interface<pedestrian::box_info>();
			head.set_x1(p["x1"].asInt());
			head.set_y1(p["y1"].asInt());
			head.set_x2(p["x2"].asInt());
			head.set_y2(p["y2"].asInt());
			head.set_score(p["score"].asFloat());
			pedestrians.push_back(head);
		}
		params.removeMember("person_list");
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

		//转换成 impl_ 要的参数
		std::map<std::string, double> param_map;
		std::vector<PedestrianInfo> pedestrain_info_;
		for (auto it : pedestrians)
			pedestrain_info_.emplace_back(PedestrianInfo{ it });

		for (auto it : dynamic_param_map) {
			param_map.insert(std::make_pair(it.first, it.second));
		}
		auto result = impl_->detect(std::move(input_data), channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map, pedestrain_info_);

		Json::Value jarray_box;

		Json::Value jarray_detected(Json::arrayValue);
		for (int i = 0; i < result.size(); i++)
		{

			jarray_box["x1"] = Json::Int(result[i].x1());
			jarray_box["y1"] = Json::Int(result[i].y1());
			jarray_box["x2"] = Json::Int(result[i].x2());
			jarray_box["y2"] = Json::Int(result[i].y2());
			jarray_box["id"] = Json::Int(result[i].id());
			jarray_box["score"] = Json::Value(result[i].confidence());
			jarray_box["first_show_time"] = Json::Value(result[i].first_show_time());
			jarray_box["last_show_time"] = Json::Value(result[i].last_show_time());
			jarray_box["cosine_similarity"] = Json::Value(result[i].cosine_similarity());
			jarray_detected.append(jarray_box);

		}

		Json::Value jarray_info;

		jarray_info["person_info"] = jarray_detected;

		value["detect_info"] = jarray_info;

		return exposing::to_param_string(writer.write(value));
	}

	exposing::param_string detect_code_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_string detect_code_impl::remove_library(std::int32_t device) const
	{
		return exposing::to_param_string(impl_->remove_library(device));
	}

	exposing::param_string detect_code_impl::remove_person_by_index(std::int32_t device_id, std::int32_t id)const 
	{
		return exposing::to_param_string(impl_->remove_person_by_index(device_id,id));
	}

	exposing::param_vector<wander::box_info> detect_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width,int roi_x, int roi_y, int roi_width, int roi_height,
		const exposing::param_hash_map<exposing::param_string, double>& param_map_abi, 
		const exposing::param_vector<pedestrian::box_info>& pedestrain_info_abi) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"wander internal object not initialized");

		std::map<std::string, double> param_map;
		std::vector<PedestrianInfo> pedestrain_info;
		for (auto it : pedestrain_info_abi)
			pedestrain_info.emplace_back(PedestrianInfo{it});

		for (auto it : param_map_abi) {
			param_map.insert(std::make_pair(it.key(), it.value()));
		}

		return impl_->detect(std::move(bitmap), channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map, pedestrain_info);
	}

}
