#include "detect_code_impl.hpp"
#include "detect_code_internal.hpp"
#include <map>
#include "json.h"

namespace glasssix::pump_weld
{
    detect_code_impl::detect_code_impl() {}

    detect_code_impl::~detect_code_impl() {}

    void detect_code_impl::init(const exposing::param_string& str_params)
    {
        Json::Reader reader(Json::Features::strictMode());
        Json::Value root;
        if (!reader.parse(exposing::to_narrow_string(str_params), root))
            throw Json::Exception("parse json failed");
        std::string model_directory = root["models_directory"].asString();
        int device = root.get("device", Json::Int(-1)).asInt();
        impl_ = std::make_unique<detect_code_internal>(exposing::to_narrow_string(model_directory), device);
    }

    exposing::param_string detect_code_impl::execute(exposing::param_hash_map < exposing::param_string, exposing::unknown_object> input_params_map)
    {
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"selene internal object not initialized");

		Json::Reader reader(Json::Features::strictMode());
		Json::FastWriter writer;
		Json::Value root, value;
		if (!reader.parse(exposing::to_narrow_string(exposing::unbox<exposing::param_string>(input_params_map.get_value("params"))), root))
			throw Json::Exception("parse json failed");
		Json::Value params = root.get("dyparams", Json::Value());
		int batch = params["batch"].asInt();

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

		auto result = impl_->detect(input_data, batch, height, width, dynamic_param_map);

		Json::Value jarray_persons_weld(Json::arrayValue);

		for (int i = 0; i < result.size(); i++)
		{
			Json::Value jarray_box;
			jarray_box["can_x1"] = Json::Int(result[i].can_x1());
			jarray_box["can_x2"] = Json::Int(result[i].can_x2());
			jarray_box["can_y1"] = Json::Int(result[i].can_y1());
			jarray_box["can_y2"] = Json::Int(result[i].can_y2());

			Json::Value weld_boxes_info;
			auto weld_loacl_list = result[i].weldlocal_list();
			int weld_loacl_list_size = weld_loacl_list.size();
			int weld_loacl_list_group = weld_loacl_list_size / 4;
			for (size_t g = 0; g < weld_loacl_list_group; g++)
			{
				weld_boxes_info["x1"] = Json::Int(weld_loacl_list[g * 4 + 0]);
				weld_boxes_info["y1"] = Json::Int(weld_loacl_list[g * 4 + 1]);
				weld_boxes_info["x2"] = Json::Int(weld_loacl_list[g * 4 + 2]);
				weld_boxes_info["y2"] = Json::Int(weld_loacl_list[g * 4 + 3]);
			}
			jarray_box["weld_list"].append(weld_boxes_info);

			auto category = result[i].category();
			auto score = result[i].score();
			jarray_box["category"] = Json::Int(category);
			//jarray_box["score"] = Json::Int(score);

			jarray_persons_weld.append(jarray_box);
		}

		value["detect_info"]["persons_weld"] = jarray_persons_weld;

		value["status"]["message"] = Json::Value("OK");
		return exposing::to_param_string(writer.write(value));
    }

    exposing::param_vector<pump_weld::box_info> detect_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t batch, std::int32_t height, std::int32_t width, const exposing::param_hash_map<exposing::param_string,float>& param_map_abi)
    {
        std::map<std::string,float> param_map_std;
        for (auto it : param_map_abi) {
            param_map_std.insert(std::make_pair(it.key(), it.value()));
        }

        if (!impl_)
            throw exposing::abi_invalid_operation(u8"pump_weld detect_code internal object not initialized"); 

        return impl_->detect(bitmap, batch, height, width, param_map_std);
    }

    exposing::param_string detect_code_impl::version() const
    {
        return exposing::to_param_string(impl_->version());
    }
}
