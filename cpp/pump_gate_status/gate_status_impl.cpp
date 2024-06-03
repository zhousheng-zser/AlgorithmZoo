#include "gate_status_impl.hpp"
#include "gate_status_internal.hpp"
#include "json.h"
#include <iostream>
namespace glasssix::pump_gate_status
{
    gate_status_impl::gate_status_impl()
    {
    }

    gate_status_impl::~gate_status_impl()
    {
    }

    void gate_status_impl::init(const exposing::param_string& str_params)
	{
		impl_ = std::make_unique<gate_status_internal>();
	}

    exposing::param_string gate_status_impl::version() const
    {
        return exposing::to_param_string(impl_->version());
    }

    exposing::param_string gate_status_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map  ) 
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"gate_status internal object not initialized");

		Json::Reader reader(Json::Features::strictMode());
		Json::FastWriter writer;
		Json::Value root, value;
		if (!reader.parse(exposing::to_narrow_string(exposing::unbox<exposing::param_string>(input_params_map.get_value("params"))), root))
			throw Json::Exception("parse json failed");

		Json::Value params = root.get("dyparams", Json::Value());

		// std::cout<<params.toStyledString()<<"style string \n";

        		int yellow_hsv_lower = root["yellow_hsv_lower"].asInt();
				int yellow_hsv_upper = root["yellow_hsv_upper"].asInt();
				int gray_hsv_lower = root["gray_hsv_lower"].asInt();
				int gray_hsv_upper = root["gray_hsv_upper"].asInt();

        auto door_floor_rois = exposing::make_param_vector<int>();
            
        auto rois = root["rois"];
        door_floor_rois.push_back( rois["door"]["x1"].asInt() );
        door_floor_rois.push_back( rois["door"]["y1"].asInt() );
        door_floor_rois.push_back( rois["door"]["x2"].asInt() );
        door_floor_rois.push_back( rois["door"]["y2"].asInt() );
        door_floor_rois.push_back( rois["floor"]["x1"].asInt() );
        door_floor_rois.push_back( rois["floor"]["y1"].asInt() );
        door_floor_rois.push_back( rois["floor"]["x2"].asInt() );
        door_floor_rois.push_back( rois["floor"]["y2"].asInt() );
        
        std::vector<int> std_rois(door_floor_rois.size());
        for (size_t i = 0; i < door_floor_rois.size(); i++)
            std_rois[i] = door_floor_rois[i];

		std::map<std::string, float> dparam_map;

		for (auto& param_name : params.getMemberNames()) {
			std::cout<<param_name<<"\n";
			dparam_map.try_emplace(param_name, params[param_name].asFloat());
		}
		std::cout<<"dsds\n";

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

		auto result = impl_->detect(input_data, channels, height, width, yellow_hsv_lower, yellow_hsv_upper, gray_hsv_lower,  gray_hsv_upper, std_rois,  dparam_map  );

        if(result)
            value["security_status"] = Json::Value("dangerous");
        else
            value["security_status"] = Json::Value("secure");

        value["status"]["message"] = Json::Value("OK");
		value["status"]["code"] = Json::Value(0);
		return exposing::to_param_string(writer.write(value));
	}

}
