#pragma once
#include <opencv2/opencv.hpp>
#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include <GenPipeline/GetPostprocessing.hpp>
//#include <GenPipeline/GenPipeTools.hpp>
#include "../genpipeline/market/yolov8_GEN.hpp"
#include "kit.hpp"

struct WeldBBox :public GenPipTools::YoloBoxBase{
public:
	using YoloBoxBase::YoloBoxBase; //Inheriting Constructors
};

struct PersonBBox :public GenPipTools::YoloBoxBase {
public:
	using YoloBoxBase::YoloBoxBase; //Inheriting Constructors
};

static inline std::vector<WeldBBox> weld_yolo_detect(LabConfig& config, const std::map<std::string, PostprocessingFunction>& postprocessing_market) {
	cv::Mat image = cv::imread(config.image);
	const int letter_h = config.infr_h;
	const int letter_w = config.infr_w;

	// new pipeline
	auto ioprocess_pipeline = PrePostProcessGenPipeline::mkSharePipeline(config.model, 0);
	//auto ioprocess_pipeline = std::make_shared<PrePostProcessGenPipeline>(std::make_shared<GenPipeline>(config.model, 0)); // === mkSharePipeline
	
	// set normal param
	ioprocess_pipeline->manual_possible_normalization({ 0,0,0 }, { 0.003921568,0.003921568,0.003921568 }); //also right
	//ioprocess_pipeline->manual_possible_normalization(0, 0.003921568); //also right

	// set postprocessor
	/* ************************************************ */
	/*       self-defined net tensors postprocessor     */
	/* ************************************************ */
	ioprocess_pipeline->set_postprocessing<true>(yolov8_GEN<2, 0>); // compile time decision
	//ioprocess_pipeline->set_postprocessing<true>(postprocessing_market, config.postFuncStr); // run time decision

	// image preprocessing
	GenPipTools::LetterInfo letter_op;
	auto letter_img = GenPipTools::letter_image(image, letter_w, letter_h, letter_op, true);
	
	//// time cost calcu
	//constexpr int LOOP = 10;
	//if constexpr (LOOP > 0) {
	//	for (int i = 0; i < 5; i++)
	//		ioprocess_pipeline->forward(letter_img); //warm up
	//	ioprocess_pipeline->clear_profiler_record();
	//	for (int i = 0; i < LOOP; i++)
	//		ioprocess_pipeline->forward(letter_img);
	//	ioprocess_pipeline->show_avg_infer_post_cost();
	//}

	// network inference with post-processing which u set (look up).
	auto rst_map = ioprocess_pipeline->forward(letter_img);
	auto tensor_out = rst_map.begin()->second;
	const int visual_field_nums = tensor_out->height();
	const int per_raw_line_length = tensor_out->width();

	// yolo to box
	/* ************************************** */
	/*        Customized implementation       */
	/* ************************************** */
	std::vector<WeldBBox> box_list;
	for (size_t idx = 0; idx < visual_field_nums; idx++) {
		float* pdata = tensor_out->mutable_cpu_data() + idx * per_raw_line_length;
		float chassis_conf = pdata[4];//chassis_conf
		float tube_conf = pdata[5];//tube_conf
		if (chassis_conf > 0.3) {
			WeldBBox obj_box(pdata[0] * letter_w, pdata[1] * letter_h, pdata[2] * letter_w, pdata[3] * letter_h, tube_conf, 0);
			box_list.push_back(obj_box);
		}
	}
	/* ************************************** */


	// original coordinate mapping
	GenPipTools::letter_map_origin_location(box_list, letter_op);
	GenPipTools::nms_cpu(box_list, 0.4);

	// draw
	for (auto obj : box_list) {
		cv::rectangle(image, obj.get_rect(), { 0,255,0 }, 3);
	}
	AdpShow(image);

	return box_list;
}

static inline std::vector<PersonBBox> person_yolo_detect(LabConfig& config, const std::map<std::string, PostprocessingFunction>& postprocessing_market) {
	// init
	auto ioprocess_pipeline = PrePostProcessGenPipeline::mkSharePipeline(config.model, 0);
	ioprocess_pipeline->manual_possible_normalization(0, 1.f / 255);
	ioprocess_pipeline->set_postprocessing(yolov8_GEN<1, 1>);
	// run
	cv::Mat image = cv::imread(config.image);
	GenPipTools::LetterInfo letter_op;
	auto letter_img = GenPipTools::letter_image(image, config.infr_w, config.infr_h, letter_op, true);
	auto tensor_out = ioprocess_pipeline->forward(letter_img).begin()->second;
	const int vf_nums = tensor_out->height(); //vf, visual field
	const int per_vf_len = tensor_out->width();
	std::vector<PersonBBox> box_list;
	for (size_t idx = 0; idx < vf_nums; idx++) {
		float* pdata = tensor_out->mutable_cpu_data() + idx * per_vf_len;
		float conf = pdata[4];
		if (conf > 0.3) {
			PersonBBox obj_box(pdata[0] * config.infr_w, pdata[1] * config.infr_h, pdata[2] * config.infr_w, pdata[3] * config.infr_h, conf, 0);
			box_list.push_back(obj_box);
		}
	}
	GenPipTools::letter_map_origin_location(box_list, letter_op);
	GenPipTools::nms_cpu(box_list, 0.4);
	return box_list;
}


// helper function
template<typename YoloBoxDerived>
void visual_person(std::string img, const std::vector<YoloBoxDerived>& person_list) {
	static_assert(std::is_base_of_v<GenPipTools::YoloBoxBase, YoloBoxDerived>, "The element type must be derived from YoloBoxBase");
	auto image = cv::imread(img);
	for (auto obj : person_list) {
		cv::rectangle(image, obj.get_rect(), { 0,255,0 }, 3);
	}
	AdpShow(image);
}