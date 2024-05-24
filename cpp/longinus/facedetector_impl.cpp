#include "facedetector_impl.hpp"
#include "retina_net.hpp"
#include "yolov7_net.hpp"
#include "face_info_impl.hpp"
#include "json.h"

namespace glasssix::longinus
{
	facedetector_impl::facedetector_impl()
	{
	}

	facedetector_impl::~facedetector_impl()
	{
	}

	void facedetector_impl::init(const exposing::param_string& str_params)
	{
		Json::Reader reader(Json::Features::strictMode());
		Json::Value root;
		if (!reader.parse(exposing::to_narrow_string(str_params), root))
			throw Json::Exception("parse json failed");
		algo_type_ = root["algo_type"].asInt();
		std::string models_directory = root["models_directory"].asString();
		int model_type = root.get("model_type", Json::Int(0)).asInt();
		float nms_threshold = root.get("nms_threshold", Json::Value(0.4f)).asFloat();
		int device = root.get("device", Json::Int(-1)).asInt();
		switch (algo_type_)
		{
		case 0:
			impl_ = std::make_unique<retina_net>(models_directory, model_type, nms_threshold, device);
			break;
		case 1:
			impl_ = std::make_unique<yolov7_net>(models_directory, model_type, nms_threshold, device);
			break;
		default:
			throw exposing::abi_invalid_argument(u8"Invalid param 'model_type'");
			break;
		}
	}

	exposing::param_string facedetector_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_string facedetector_impl::execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map)
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"longinus internal object not initialized");

		Json::Reader reader(Json::Features::strictMode());
		Json::FastWriter writer;
		Json::Value root, value;
		if (!reader.parse(exposing::to_narrow_string(exposing::unbox<exposing::param_string>(input_params_map.get_value("params"))), root))
			throw Json::Exception("parse json failed");
		
		auto input_data = exposing::unbox<exposing::param_span<std::uint8_t>>(input_params_map.get_value("input_data"));
		auto output_data = exposing::unbox<exposing::param_span<std::uint8_t>>(input_params_map.get_value("output_data"));
		int order = exposing::unbox<int>(input_params_map.get_value("order"));
		auto data_shape = input_params_map.get_value("data_shape").as<exposing::param_vector<int>>();

		int num = data_shape[0];
		int channels = data_shape[1];
		int height = data_shape[2];
		int width = data_shape[3];

		int command = root["command"].asInt();
		switch (command)
		{
		case 0:
			{
				int min_size = root["min_size"].asInt();
				float threshold = root["threshold"].asFloat();
				bool do_attributing = root["do_attributing"].asBool();

				auto face_vec = impl_->detect(input_data, channels, height, width, min_size, threshold, static_cast<int>(order), do_attributing);

				Json::Value jarray_rect = Json::Value(Json::arrayValue);
				for(auto& obj: face_vec)
				{
					Json::Value jobj_face;
					jobj_face["x"] = Json::Int(obj.x());
					jobj_face["y"] = Json::Int(obj.y());
					jobj_face["width"] = Json::Int(obj.width());
					jobj_face["height"] = Json::Int(obj.height());
					jobj_face["ori_x"] = Json::Int(obj.ori_x());
					jobj_face["ori_y"] = Json::Int(obj.ori_y());
					jobj_face["ori_width"] = Json::Int(obj.ori_width());
					jobj_face["ori_height"] = Json::Int(obj.ori_height());
					jobj_face["confidence"] = Json::Value(obj.confidence());

					if (do_attributing)
					{
						jobj_face["attributes"]["yaw"] = Json::Value(obj.yaw());
						jobj_face["attributes"]["pitch"] = Json::Value(obj.pitch());
						jobj_face["attributes"]["roll"] = Json::Value(obj.roll());
						jobj_face["attributes"]["glass_index"] = Json::Int(obj.glass_index());
						jobj_face["attributes"]["mask_index"] = Json::Int(obj.mask_index());

						Json::Value jarray_landmark;
						for (const auto& pt : obj.pts())
						{

							Json::Value jobj_point;
							jobj_point["x"] = Json::Int((int)pt.key());
							jobj_point["y"] = Json::Int((int)pt.value());
							jarray_landmark.append(jobj_point);
						}
						jobj_face["landmark"] = jarray_landmark;
					}
					else
					{
						jobj_face["attributes"] = Json::Value(Json::nullValue);
						jobj_face["landmark"] = Json::Value(Json::arrayValue);
					}

					jarray_rect.append(jobj_face);
				}
				value["facerectwithfaceinfo_list"] = jarray_rect;
			}
			break;
		case 1:
			{
				face_info_internal face_i;
				face_i.rect.x = root["face"]["x"].asFloat();
				face_i.rect.y = root["face"]["y"].asFloat();
				face_i.rect.h = root["face"]["height"].asFloat();
				face_i.rect.w = root["face"]["width"].asFloat();
				face_info ori_face = exposing::make_as_first<face_info_impl>(face_i);
				auto result = impl_->single_trace(ori_face, input_data, channels, height, width, static_cast<int>(order));

				Json::Value jobj_face;
				if (result.confidence() > 0.1f)
				{
					value["trace_success"] = Json::Value(true);
					jobj_face["x"] = Json::Int(result.x());
					jobj_face["y"] = Json::Int(result.y());
					jobj_face["width"] = Json::Int(result.width());
					jobj_face["height"] = Json::Int(result.height());
					jobj_face["ori_x"] = Json::Int(result.ori_x());
					jobj_face["ori_y"] = Json::Int(result.ori_y());
					jobj_face["ori_width"] = Json::Int(result.ori_width());
					jobj_face["ori_height"] = Json::Int(result.ori_height());
					jobj_face["confidence"] = Json::Value(result.confidence());

					jobj_face["attributes"]["glass_index"] = Json::Int(result.glass_index());
					jobj_face["attributes"]["mask_index"] = Json::Int(result.mask_index());
					jobj_face["attributes"]["yaw"] = Json::Value(result.yaw());
					jobj_face["attributes"]["pitch"] = Json::Value(result.pitch());
					jobj_face["attributes"]["roll"] = Json::Value(result.roll());

					Json::Value jarray_landmark;

					for (const auto& pt : result.pts())
					{
						Json::Value jobj_point;
						jobj_point["x"] = Json::Int((int)pt.key());
						jobj_point["y"] = Json::Int((int)pt.value());
						jarray_landmark.append(jobj_point);
					}
					jobj_face["landmark"] = jarray_landmark;
				}
				else
				{
					value["trace_success"] = Json::Value(false);
				}

				value["facerectwithfaceinfo"] = jobj_face;
			}
			break;
		case 2:
			{
				if (output_data.size() < 128 * 128 * 3)
					throw exposing::abi_invalid_operation(u8"output_data.size() < 128 * 128 * 3");

				float scale = root["scale"].asFloat();

				auto result = impl_->center_scale_align(input_data, channels, height, width, scale, static_cast<int>(order));
				result.copy_to(0, { output_data.data(), 128 * 128 * 3 });

				value["format"] = Json::Value(static_cast<int>(memory::NCHW));
			}
			break;
		default:
			break;
		}
		value["command"] = root["command"];

		return exposing::to_param_string(writer.write(value));
	}
}