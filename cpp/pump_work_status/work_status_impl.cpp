#include "work_status_impl.hpp"
// #include "work_status_internal.hpp"
#include <Primitives/tensor_conversions.hpp>

#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include "general.hpp"

#include "json.h"
#include <iostream>
namespace glasssix::pump_work_status
{
    work_status_impl::work_status_impl()
    {
    }

    work_status_impl::~work_status_impl()
    {
    }

    void work_status_impl::init(const exposing::param_string& str_params)
    {
    }

    exposing::param_string work_status_impl::version() const
    {
        std::string work_status_version = "1.0.0";
        return exposing::to_param_string(work_status_version);
    }

    exposing::param_string work_status_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map)
    {
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

        std::vector<int >rois;
        auto mask_roi = Json::Value(Json::arrayValue);;
        mask_roi = params["mask_roi"];
        for (auto p : mask_roi)
            rois.push_back(p.asInt());
        params.removeMember("mask_roi");

        std::map<std::string, float> dparam_map;

        for (auto& param_name : params.getMemberNames()) {
            dparam_map.try_emplace(param_name, params[param_name].asFloat());
        }

        bool big_paint_room = static_cast<bool> (std::round(dparam_map.count("big_paint_room") ? dparam_map["big_paint_room"] : 0));
        exposing::param_span<std::uint8_t> bitmap(std::move(input_data));

        if (bitmap.empty())
            throw exposing::abi_invalid_argument("current frame is empty");

        CHECK_EQ(channels, 3);
        CHECK_EQ(bitmap.size(), channels * height * width);

        cv::Mat image(cv::Size(width, height), CV_8UC3);
        std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * channels * height * width);

        std::vector<std::vector<int>> mask_array(rois.size() / 2);
        for (size_t i = 0; i < rois.size(); i += 2)
            mask_array[i / 2] = std::vector<int>{ rois[i],rois[i + 1] };

        clockwise_sort_by_left_corner(mask_array);
        bool result = (classify_lamp_status(image, big_paint_room, mask_array) && classify_base_plate_status(image, big_paint_room, mask_array) && classify_work_equipment_status(image, big_paint_room, mask_array));
        value["detect_info"]["security_status"] = result ? Json::Value("working") : Json::Value("vacancy");
        return exposing::to_param_string(writer.write(value));
    }

}
