#include "detect_code_impl.hpp"
#include "detect_code_internal.hpp"
#include <map>
#include "json.h"

namespace glasssix::fighting
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
        int batch  = root.get("batch", Json::Int(-1)).asInt();

		impl_ = std::make_unique<detect_code_internal>(exposing::to_narrow_string(models_directory), device,batch);
	}

    // exposing::param_vector<fighting::box_info> detect_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t height, std::int32_t width, std::int32_t roi_x, std::int32_t roi_y, std::int32_t roi_width, std::int32_t roi_height, const exposing::param_hash_map<exposing::param_string,float>& param_map_abi)
    // {
    //     std::map<std::string,float> param_map_std;
    //     for (auto it : param_map_abi) {
    //         param_map_std.insert(std::make_pair(it.key(), it.value()));
    //     }

    //     if (!impl_)
    //         throw exposing::abi_invalid_operation(u8"fighting detect_code internal object not initialized"); 

    //     return impl_->detect(bitmap, height, width, roi_x, roi_y, roi_width, roi_height, param_map_std);
    // }

    exposing::param_string detect_code_impl::version() const
    {
        return exposing::to_param_string(impl_->version());
    }

    exposing::param_string detect_code_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map  ) 
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"fight internal object not initialized");

		Json::Reader reader(Json::Features::strictMode());
		Json::FastWriter writer;
		Json::Value root, value;
		if (!reader.parse(exposing::to_narrow_string(exposing::unbox<exposing::param_string>(input_params_map.get_value("params"))), root))
			throw Json::Exception("parse json failed");

		// std::cout << root.toStyledString() << std::endl;
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

		auto result = impl_->detect(input_data, height, width, roi_x, roi_y, roi_width, roi_height, dparam_map);

		Json::Value jarray_box;
        Json::Value jarray_fight_detected(Json::arrayValue);
        Json::Value jarray_normal_detected(Json::arrayValue);

        for (int i = 0; i < result.size(); i++)
        {
            int category = Json::Int(result[i].category());
            if (category == 1)
            {
                jarray_box["x1"] = Json::Int(result[i].x1());
                jarray_box["y1"] = Json::Int(result[i].y1());
                jarray_box["x2"] = Json::Int(result[i].x2());
                jarray_box["y2"] = Json::Int(result[i].y2());
                jarray_box["score"] = Json::Value(result[i].score());
                jarray_box["category"] = Json::Value(result[i].category());
                jarray_fight_detected.append(jarray_box);
            }
            else
            {
                jarray_box["x1"] = Json::Int(result[i].x1());
                jarray_box["y1"] = Json::Int(result[i].y1());
                jarray_box["x2"] = Json::Int(result[i].x2());
                jarray_box["y2"] = Json::Int(result[i].y2());
                jarray_box["score"] = Json::Value(result[i].score());
                jarray_box["category"] = Json::Value(result[i].category());
                jarray_normal_detected.append(jarray_box);
            }
        }

        Json::Value jarray_info;

        jarray_info["fight_list"] = jarray_fight_detected;
        jarray_info["normal_list"] = jarray_normal_detected;

        value["detect_info"] = jarray_info;
		value["status"]["message"] = Json::Value("OK");
		value["status"]["code"] = Json::Value(0);
		return exposing::to_param_string(writer.write(value));
	}

}
