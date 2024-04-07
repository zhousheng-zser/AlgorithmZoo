#include <opencv2/opencv.hpp>
#ifdef BUILD_DEBUG_INFO
//#include "dbg.h"
#endif // BUILD_DEBUG_INFO

#include <GenPipeline/PrePostProcessGenPipeline.hpp>
#include <GenPipeline/GetPostprocessing.hpp>
//#include <GenPipeline/GenPipeline_tools.hpp>

#include "kit.hpp"
#include "detect.hpp"

//#include <BMNNWrapper/bmnn_pipline.hpp>

using namespace glasssix;


int main(int argc, char* argv[]) {
	SILENT_CV_LOG;

// init environment
	std::map<std::string, PostprocessingFunction> postprocessing_market = GetPostprocessingMarket();
	DumpShowPostprocessingMarket(postprocessing_market);
	GenPipeline::dump_backend_menu(true);

//// Weld Detect Demo
//	LabConfig lab_weld{ "D:/yo8gen/weld/pump_weld.onnx", "D:/yo8gen/weld/weld.jpeg", "yolov8_c2_loc", 640, 1152 };
//
//	weld_yolo_detect(lab_weld, postprocessing_market);

// Pedestrian Detect Demo
	LabConfig lab_peron{ "D:/yo8gen/person/mul/person.onnx", "D:/yo8gen/person/mul/tcg.jpg", "yolov8_loc_c1", 1280, 1280 };
	LabConfig lab_peron_1920_1088{ "D:/yo8gen/person/1920-1088_Person_best_detection.onnx", "D:/yo8gen/person/tcgb.jpg", "yolov8_loc_c1", 1088, 1920 };
	//LabConfig lab_peron_1088_1920{ "D:/yo8gen/person/1088-1920_Person_best_detection.onnx", "D:/yo8gen/person/tcgb.jpg", "yolov8_loc_c1", 1920, 1088 };
	for (auto& lab : { lab_peron, lab_peron_1920_1088 })
	{
		std::vector<PersonBBox> person_list = person_yolo_detect(lab_peron, postprocessing_market);
		visual_person(lab_peron.image, person_list);
	}

	return 0;
}

