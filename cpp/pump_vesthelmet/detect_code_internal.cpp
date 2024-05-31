#include "detect_code_internal.hpp"
#include "box_info_internal.hpp"
#include "box_info_impl.hpp"

#include <algorithm>
#include <numeric>

#include <Excalibur/pipeline.hpp>
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include "Primitives/tensor_conversions.hpp"
#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_resize.hpp"
#include "Excalibur/operation_rgb2gray.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"

#include "../posture/detect_code.hpp"
#include "../posture/detect_code_internal.hpp"
#include "../posture/general.hpp"
#include "head_det.hpp"
#include "obj_box_info.hpp"


#include <GenPipeline/GenPipeline.hpp>
#include <YoloFamily/Yolo_wrapper.hpp>
//#include "dbg.h"

namespace glasssix::pump_vesthelmet
{
	class detect_code_internal::impl
	{
	public:
		impl(std::string_view model_directory, int device)
		{
			std::vector<std::string> phai;

#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
			if (model_type_)
				net_posture_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/posture1280_17.rknn", device);
			else
				net_posture_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/posture1280_12.rknn", device);

#elif defined(USE_BMNN)
			if (model_type_ == 1)
				net_posture_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/posture1280_17.bmodel", device);
			else
				net_posture_ = std::make_shared<GenPipeline>(std::string(model_directory) + "/posture1280_12.bmodel", device);
#endif
			net_posture_->manual_possible_normalization(std::array<float, 3>{0.f, 0.f, 0.f}, std::array<float, 3>{1.f / 255.f, 1.f / 255.f, 1.f / 255.f});
			yolov8_instance = std::make_shared<Yolov8<GenPipeline, false, true>>(1280, 1280, net_posture_);

			vest_cls_instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(phai, std::string(model_directory) + "/" + "pump_vesthelmet_vest_cls.rknn", device);
			head_det_instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(phai, std::string(model_directory) + "/" + "pump_vesthelmet_head_det.rknn", device);
			helmet_cls_instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(phai, std::string(model_directory) + "/" + "pump_vesthelmet_helmet_cls.rknn", device);
		}

		exposing::param_vector<pump_vesthelmet::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, std::map<std::string, float>& param_map_std)
		{
			auto result = exposing::make_param_vector<pump_vesthelmet::box_info>();
			std::vector<box_info_internal> output;

			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}
			CHECK_EQ(channels, 3);
			CHECK_EQ(bitmap.size(), channels * height * width);
			cv::Mat image(cv::Size(width, height), CV_8UC3);

			std::memcpy(image.data, bitmap.data(), sizeof(uint8_t) * channels * height * width);

			float posture_conf_thres = param_map_std.count("posture_conf_thres") ? param_map_std["posture_conf_thres"] : 0.1f;
			float iou_thres = param_map_std.count("nms_thres") ? param_map_std["nms_thres"] : 0.6f;
			float head_conf_thres = param_map_std.count("head_conf_thres") ? param_map_std["head_conf_thres"] : 0.6f;
			float head_min_h_thres = param_map_std.count("head_min_h_thres") ? param_map_std["head_min_h_thres"] : 24.0f;
			float head_min_w_thres = param_map_std.count("head_min_w_thres") ? param_map_std["head_min_w_thres"] : 24.0f;
			//float vest_cls_thres = param_map_std.count("vest_cls_thres") ? param_map_std["vest_cls_thres"] : 0.7f;
			//float helmet_cls_thres = param_map_std.count("helmet_cls_thres") ? param_map_std["helmet_cls_thres"] : 0.7f;
			constexpr float is_vest_score_thres = 0.8;
			constexpr float no_helmet_score_thres = 0.7;

			constexpr int NO_REF_VEST = -1;
			constexpr int IS_REF_VEST = 100;

			constexpr int NO_HELMET = 0; //alarm target
			constexpr int HAS_HELMET = 1;
			constexpr int OTHER_HAT = 2;
			constexpr int LOW_NO_HELMET = 3;

			auto posture_param_abi = exposing::make_param_hash_map<exposing::param_string, float>();
			posture_param_abi.add_or_update("conf_thres", posture_conf_thres);
			std::vector<box_info_internal> objects;
			cv::Mat image(cv::Size(width, height), CV_8UC3, const_cast<uint8_t*>(bitmap.data()));
			auto objects_of_full_figure = yolov8_instance->get_objects(image, posture_conf_thres);

			std::vector<std::vector<float>> nms_input;

			std::vector<ObjectInfo> Need_to_filter;
			for (const auto& var : Need_to_filter)
				nms_input.push_back({ float(var.x1), float(var.y1), float(var.x2 - var.x1), float(var.y2 - var.y1), float(var.score) });

			auto nms_result_index = object_nms(nms_input, iou_thres);

			auto fin_result = exposing::make_param_vector<posture::box_info>();

			std::vector<posture::box_info_internal> result_posture;

			for (auto& id : nms_result_index)
			{
				posture::box_info_internal temp_result;
				temp_result.x1 = Need_to_filter[id].x1 + 0;
				temp_result.y1 = Need_to_filter[id].y1 + 0;
				temp_result.x2 = Need_to_filter[id].x2 + 0;
				temp_result.y2 = Need_to_filter[id].y2 + 0;
				temp_result.score = Need_to_filter[id].score;
				temp_result.key_points = exposing::make_param_vector<float>();
				for (int j = 0; j < Need_to_filter[id].key_points.size(); j++)
				{
					temp_result.key_points.push_back(Need_to_filter[id].key_points[j].x + 0);
					temp_result.key_points.push_back(Need_to_filter[id].key_points[j].y + 0);
					temp_result.key_points.push_back(Need_to_filter[id].key_points[j].score);
				}
				result_posture.push_back(temp_result);
			}

			for (auto& i : result_posture)
				fin_result.push_back(exposing::make_as_first<posture::box_info_impl>(i));
			//exposing::param_vector<posture::box_info> posture_info_list_raw = yolov8_instance.detect(bitmap, channels, height, width, 0, 0, width, height, posture_param_abi);

//cv::Mat vi = image.clone();
//std::string lg = "";

			for (auto pinfo : fin_result)
			{
//lg += '_';
//if (lg != "__") continue;
				PostureInfo postureInfo{ pinfo };

				if (postureInfo.if_pump_vesthelmet_bodyerr()) {
					continue; //body error
				}

				auto vest_cls_rect = postureInfo.get_vest_det_region();
				if (vest_cls_rect.height <= 1 || vest_cls_rect.width <= 1) continue; //invalid input
				auto vest_cls_region = safty_cut(image, vest_cls_rect);
				//vest_cls_region = letterbox(vest_cls_region, 128, 128);

//cv::imwrite("/home/glasssix/yhc/AlgorithmZoo/cpp/pump_vesthelmet/vest_cls_region" + lg + ".png", vest_cls_region);
//vest_cls_region = cv::imread("/home/glasssix/yhc/AlgorithmZoo/cpp/pump_vesthelmet/lQDPJ.jpg");

				cv::resize(vest_cls_region, vest_cls_region, cv::Size2i{ 128, 128 });

				cv::cvtColor(vest_cls_region, vest_cls_region, cv::COLOR_BGR2RGB);

				auto vest_cls_rst_map = vest_cls_instance_->forward(vest_cls_region.data, { 1, vest_cls_region.rows, vest_cls_region.cols, vest_cls_region.channels() }, RKNN_TENSOR_NHWC);
				auto vest_cls_rst = vest_cls_rst_map.begin()->second;
				auto vest_cls_scores = vest_cls_rst->cpu_data();
				float is_refvest_score = vest_cls_scores[1];
				
//printf("## is_refvest_score %f ¡ª¡ª t %f\n", is_refvest_score, is_vest_score_thres);

				int refvest_status = IS_REF_VEST;
				if (is_refvest_score < is_vest_score_thres) {
					//printf("### NO_REF_VEST\n");
					refvest_status = NO_REF_VEST;
				}

				auto people_img_rect = postureInfo.get_rect();
				//auto people_start = people_img_rect.tl(); //only for head box map origin picture

				if (refvest_status == NO_REF_VEST) {
					//Pack NO_REF_VEST Person
					box_info_internal person_vest_unit;
					person_vest_unit.x1 = people_img_rect.x;
					person_vest_unit.y1 = people_img_rect.y;
					person_vest_unit.x2 = people_img_rect.x + people_img_rect.width;
					person_vest_unit.y2 = people_img_rect.y + people_img_rect.height;
					person_vest_unit.score = vest_cls_scores[1];
					person_vest_unit.category = NO_REF_VEST;
					output.push_back(person_vest_unit);
				}
				else {
					auto people_img = safty_cut(image, people_img_rect);
					const int people_height = people_img.rows;
					std::vector<HeadInfo> head_info = head_det(people_img, head_conf_thres);
					if (head_info.empty())
					{
						// wear refvest but det no head
						box_info_internal person_vest_nohead_unit;
						person_vest_nohead_unit.x1 = people_img_rect.x;
						person_vest_nohead_unit.y1 = people_img_rect.y;
						person_vest_nohead_unit.x2 = people_img_rect.x + people_img_rect.width;
						person_vest_nohead_unit.y2 = people_img_rect.y + people_img_rect.height;
						person_vest_nohead_unit.score = vest_cls_scores[1];
						person_vest_nohead_unit.category = IS_REF_VEST;
						output.push_back(person_vest_nohead_unit);
					}
					else {
						for (auto& head : head_info)
						{
							auto head_rect = head.get_rect();
							auto head_center = head.get_center();
							if (head_rect.width < head_min_w_thres || head_rect.height < head_min_h_thres)
								continue;
							if (head_center.y > people_height * 0.2)
								continue;

							box_info_internal person_head_unit;
							//person_head_unit.x1 = head.x1 + people_start.x;
							//person_head_unit.y1 = head.y1 + people_start.y;
							//person_head_unit.x2 = head.x2 + people_start.x;
							//person_head_unit.y2 = head.y2 + people_start.y;
							person_head_unit.x1 = people_img_rect.x;
							person_head_unit.y1 = people_img_rect.y;
							person_head_unit.x2 = people_img_rect.x + people_img_rect.width;
							person_head_unit.y2 = people_img_rect.y + people_img_rect.height;

							auto helmet_cls = safty_cut(people_img, head_rect);
							//auto helmet_cls = letterbox(helmet_cls_region, 96, 96);
							cv::resize(helmet_cls, helmet_cls, cv::Size2i{ 96, 96 });

							cv::cvtColor(helmet_cls, helmet_cls, cv::COLOR_BGR2RGB);
							auto helmet_cls_rst_map = helmet_cls_instance_->forward(helmet_cls.data, { 1, helmet_cls.rows, helmet_cls.cols, helmet_cls.channels() }, RKNN_TENSOR_NHWC);
							auto helmet_cls_rst = helmet_cls_rst_map.begin()->second;
							auto helmet_cls_scores = helmet_cls_rst->mutable_cpu_data();// 3 * float

							//Pack score & tag
							struct HelmetClsStatus
							{
								float conf;
								int status;
							};

							std::array<HelmetClsStatus, 3> socre_idx_list{
							HelmetClsStatus{helmet_cls_scores[NO_HELMET],NO_HELMET},
							HelmetClsStatus{helmet_cls_scores[HAS_HELMET],HAS_HELMET},
							HelmetClsStatus{helmet_cls_scores[OTHER_HAT],OTHER_HAT},
							};

							std::sort(socre_idx_list.begin(), socre_idx_list.end(), [](HelmetClsStatus& a, HelmetClsStatus& b) {
								return a.conf > b.conf;
								});

							auto max_conf_status = socre_idx_list[0];

							person_head_unit.score = max_conf_status.conf;

							// carefullly judge if NO_HELMET for lowering mistake alarm
							if (max_conf_status.status == NO_HELMET)
							{
								if (max_conf_status.conf > no_helmet_score_thres)
								{
									person_head_unit.category = NO_HELMET; // alarm !
								}
								else
								{
									person_head_unit.category = LOW_NO_HELMET; // no helmet but low confidence
								}
							}
							else
							{
								person_head_unit.category = max_conf_status.status;
							}

							output.push_back(person_head_unit);
						}
					}
				}
			}

			for (auto& it : output)
			{
				result.push_back(glasssix::exposing::make_as_first<box_info_impl>(it));
			}
			return result;
		}

		std::string version()
		{
			const std::string algo_module_version = "1.5.2";
			std::string nn_frame_version = "rknn";
			return fmt::format(R"({ {"nn_frame_version":"{}", "algo_module_version" : "{}"} })", nn_frame_version, algo_module_version);
		}

		inline cv::Mat safty_cut(cv::Mat& img, cv::Rect roi)
		{
			int width = roi.width;
			int height = roi.height;
			int x = roi.x;
			int y = roi.y;

			cv::Mat mat(height, width, img.type(), cv::Scalar(0));
			int _x = x;
			int _y = y;
			int _width = width;
			int _height = height;
			if (x < 0)
			{
				_x = 0;
				_width = width + x;
			}

			if (_x + _width > img.cols)
				_width = img.cols - _x;

			if (y < 0)
			{
				_y = 0;
				_height = height + y;
			}

			if (_y + _height > img.rows)
				_height = img.rows - _y;

			img(cv::Rect(_x, _y, _width, _height)).copyTo(mat(cv::Rect(_x - x, _y - y, _width, _height)));
			return mat;
		}

		static inline cv::Mat letterbox(cv::Mat img, int hope_w, int hope_h)
		{
			int H = img.rows;
			int W = img.cols;
			float ratio_w = (float)W / (float)hope_w;
			float ratio_h = (float)H / (float)hope_h;
			cv::Mat resize_img;
			if (ratio_w == ratio_h)
			{
				cv::resize(img, resize_img, cv::Size2i{ hope_w, hope_h });
			}
			else if (ratio_w > ratio_h)
			{
				int new_x = hope_w;
				int new_y = (int)(H / ratio_w);
				int pad1 = (int)((hope_h - new_y) / 2);
				int pad2 = hope_h - new_y - pad1;
				cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
				cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
			}
			else
			{
				int new_y = hope_h;
				int new_x = (int)(W / ratio_h);
				int pad1 = (int)((hope_w - new_x) / 2);
				int pad2 = hope_w - new_x - pad1;
				cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
				cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
			}
			return resize_img;
		}

		std::vector<HeadInfo> head_det(cv::Mat image, float head_det_conf_thres = 0.6, float nms_threshold = 0.5) {
			std::vector<HeadInfo> head_list;

			constexpr int reShapeSide = 128;
			auto letter_img = letterbox(image, reShapeSide, reShapeSide);
			cv::cvtColor(letter_img, letter_img, cv::COLOR_BGR2RGB);
			auto det_rst_map = head_det_instance_->forward(letter_img.data, { 1, letter_img.rows, letter_img.cols, letter_img.channels() }, RKNN_TENSOR_NHWC);
			auto det_rst_vec = sort_yolo_rst(det_rst_map);
			auto tensor_out = yolov8_complement(det_rst_vec);

			int targetnum = tensor_out->height();
			int infonum = tensor_out->width();
			for (size_t idx = 0; idx < targetnum; idx++) {
				float* pdata = tensor_out->mutable_cpu_data() + idx * infonum;
				float conf = pdata[4];
				if (conf > head_det_conf_thres) {
					//dbg(conf);
					//std::cout << "pdata m640: " << pdata[0] * 640 << " " << pdata[1] * 640 << " " << pdata[1] * 640 << " " << pdata[1] * 640 << std::endl;
					HeadInfo headbox(pdata[0] * reShapeSide, pdata[1] * reShapeSide, pdata[2] * reShapeSide, pdata[3] * reShapeSide, conf);
					head_list.push_back(headbox);
				}
			}

			int pad = std::abs(image.cols - image.rows) / 2;
			bool is_vertical_pad = image.cols > image.rows;
			float mapping_ratio = static_cast<float>(std::max(image.cols, image.rows)) / reShapeSide;

			for (auto& bbox : head_list) {
				bbox.mul_ratio(mapping_ratio);
				if (is_vertical_pad) {
					bbox.ymin -= pad;
					bbox.ymax -= pad;
				}
				else {
					bbox.xmin -= pad;
					bbox.xmax -= pad;
				}
			}

			headinfo_nms_cpu(head_list, nms_threshold);
			return head_list;
		}

	private:
		posture::detect_code posture_instance_;
		std::unique_ptr<rknnwrapper::rknn_wrapper> vest_cls_instance_;
		std::unique_ptr<rknnwrapper::rknn_wrapper> head_det_instance_;
		std::unique_ptr<rknnwrapper::rknn_wrapper> helmet_cls_instance_;

        int model_type_=1;
		std::shared_ptr<GenPipeline> net_posture_;
		std::shared_ptr<Yolov8<GenPipeline, false, true>> yolov8_instance;
	};

	detect_code_internal::detect_code_internal(std::string_view model_directory, int device)
		: impl_{ std::make_unique<impl>(model_directory, device) }
	{
	}

	detect_code_internal::~detect_code_internal()
	{
	}

	exposing::param_vector<pump_vesthelmet::box_info> detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, std::map<std::string, float>& param_map_std)
	{
		return impl_->detect(bitmap, channels, height, width, param_map_std);
	}

	std::string detect_code_internal::version()
	{
		return impl_->version();
	}
}
