#include "detect_code_impl.hpp"
#include "detect_code_internal.hpp"
#include <map>
#include "json.h"
#include <iostream>

namespace glasssix::pump_pumptop_person
{
    detect_code_impl::detect_code_impl() {}

    detect_code_impl::~detect_code_impl() {}

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

    exposing::param_string detect_code_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map)
    {
        if (!impl_)
            throw exposing::abi_invalid_operation(u8"pump_pumptop_person detect_code internal object not initialized"); 


		Json::Reader reader(Json::Features::strictMode());
		Json::FastWriter writer;
		Json::Value root, value;
		if (!reader.parse(exposing::to_narrow_string(exposing::unbox<exposing::param_string>(input_params_map.get_value("params"))), root))
			throw Json::Exception("parse json failed");
		Json::Value params = root.get("dyparams", Json::Value());
		//从json获取人头数据,并赋值给C++对应结构体
		auto person_list = Json::Value(Json::arrayValue);;
		person_list = params["person_list"];
		std::vector<pump_pumptop_person::PedestrianInfo> persons;

		for (auto p : person_list)
		{
			auto person = exposing::make_exported_interface<pedestrian::box_info>();
			person.set_x1(p["x1"].asInt());
			person.set_x2(p["x2"].asInt());
			person.set_y1(p["y1"].asInt());
			person.set_y2(p["y2"].asInt());
			person.set_score(p["score"].asFloat());
			persons.push_back(pump_pumptop_person::PedestrianInfo(person));
		}
		params.removeMember("person_list");

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

		auto result = impl_->detect(std::move(input_data), height, width, persons, dparam_map);


		Json::Value jarray_box;

		Json::Value jarray_persons_in_pumptop(Json::arrayValue);//在泵内
		Json::Value jarray_persons_out_pumptop(Json::arrayValue);//不在泵内

		for (int i = 0; i < result.size(); i++)
		{
			jarray_box["x1"] = Json::Int(result[i].x1());
			jarray_box["y1"] = Json::Int(result[i].y1());
			jarray_box["x2"] = Json::Int(result[i].x2());
			jarray_box["y2"] = Json::Int(result[i].y2());
			auto category = result[i].category();
			auto score = result[i].score();
			jarray_box["category"] = Json::Int(category);
			jarray_box["score"] = Json::Value(score);

			auto pump_loaction = result[i].pump();
			if (pump_loaction.size() == 8) {
				Json::Value jarray_pump_location_points(Json::arrayValue);

				for (int ptIdx = 0; ptIdx < 4; ptIdx++) {
					Json::Value jarray_point;
					jarray_point["x"] = pump_loaction[2 * ptIdx];
					jarray_point["y"] = pump_loaction[2 * ptIdx + 1];
					jarray_pump_location_points.append(jarray_point);
				}

				jarray_box["pump"] = jarray_pump_location_points;
			}

			if (category == 1) {
				jarray_persons_in_pumptop.append(jarray_box);
			}
			else {
				jarray_persons_out_pumptop.append(jarray_box);
			}
		}

		value["detect_info"]["persons_in_pumptop"] = jarray_persons_in_pumptop;
		value["detect_info"]["persons_out_pumptop"] = jarray_persons_out_pumptop;

		return exposing::to_param_string(writer.write(value));
    }

    exposing::param_string detect_code_impl::version() const
    {
        return exposing::to_param_string(impl_->version());
    }
}
