#ifdef USE_RKNNAPI
//#if 0
#include "RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "RKNN2Wrapper/rknn2_wrapper.hpp"
#endif

#include "Excalibur/pipeline.hpp"
#include "Primitives/tensor_conversions.hpp"
#include <fstream>
#include <algorithm>
#include <numeric>
#include <opencv2/opencv.hpp>
#include "json.hpp"
#include "dbg.h"
#include "auto_infer_output_map.hpp"
#include "makeTable.hpp"
#include "test_model.hpp"

/*
"modelTest":{
	"rknn":"mt2/longinus320.rknn",
	"phai":"mt2/longinus.phai",
	"racy":"mt2/longinus.racy",
	"dataset":"mt2/320.txt",
	"outputMap":"mt2/retina_output_map.json",
	# "imgReSize":"320" // selectable param, no force
	# "convertBGR":"0"  // selectable param, no force
}
*/


int main(int argc, char* argv[])
{
	std::unordered_map<std::string, std::string> run_param_map;

	if (!parse_param(argc, argv, run_param_map)) return 1;

	std::map<std::string, std::string> output_map;

	std::vector<std::string> phai;
	glasssix::rknnwrapper::rknn_wrapper wrapper{ phai, run_param_map["rknn"] };
	glasssix::excalibur::pipeline<float> pipe(run_param_map["phai"], run_param_map["racy"], -1);

	std::map<std::string, std::vector<float>> score_map;
	std::vector<std::string> img_list;
	load_line_txt(run_param_map["dataset"], img_list);

	if (output_map.empty())
	{
		auto_infer_output_map(wrapper, pipe, img_list[0], output_map);
	}
	else
	{
		if (!run_param_map["outputMap"].empty())
		{
			nlohmann::json map_json = read_json(run_param_map["outputMap"]);
			for (auto& x : map_json["output"].items())
				output_map[x.key()] = x.value();
		}
		std::cout << "- specify output map regulation : " << run_param_map["outputMap"] << std::endl;
	}


	std::cout << std::endl;
	for (int idx = 0; idx < img_list.size(); idx++)
	{
		cv::Mat img = cv::imread(img_list[idx]);

		 if (run_param_map.count("convertBGR"))
		 {
			 int convertBGR = std::atoi(run_param_map["convertBGR"].c_str());
			 if (convertBGR) cv::cvtColor(img, img, cv::COLOR_BGR2RGB);
			 if (idx == 0) printf("- convert BGR order !\n");
		 }

		 if (run_param_map.count("imgReSize")&& !run_param_map["imgReSize"].empty())
		 {
			 int rSize = std::atoi(run_param_map["imgReSize"].c_str());
			 cv::resize(img, img, { rSize ,rSize });
			 if (idx == 0) {
				 printf("- resize image to %d * %d !\n", rSize, rSize);
				 printf("  if program crash, check size^2 EQ rknn input shape !\n");
			 }
		 }

		std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, img.rows, img.cols, 3}, -1, glasssix::memory::NHWC));
		std::copy(img.data, img.data + img.step[0] * img.rows, input_tensor_u8->mutable_cpu_data());
		input_tensor_u8->convert_order();

		auto results_wrapper = wrapper.forward(img.data, { 1, img.rows, img.cols, 3 }, RKNN_TENSOR_NHWC);
		auto results_pipe = pipe.forward(input_tensor_u8 | glasssix::memory::tensor_convert_to<float>);

		for (auto& x : output_map)
		{
			const float* f32_out = results_pipe[x.first]->cpu_data();
			const float* i8_out = results_wrapper[x.second]->cpu_data();
			CHECK_EQ(results_pipe[x.first]->count(), results_wrapper[x.second]->count());
			int count = results_pipe[x.first]->count();
			float cos = CosineSimilarity(f32_out, i8_out, count);
			score_map[x.first].push_back(cos);
			printf("\r[%d/%d]-> cos:%f  %s    ", idx, img_list.size(), cos, img_list[idx].c_str());
		}
	}

	std::ofstream less80_out("less80_samples.txt");

	TableMaker statistics_table_maker(run_param_map["rknn"]);

	for (auto& x : score_map)
	{

		float min_cos = 1.f;
		int less99 = 0, less98 = 0, less95 = 0, less90 = 0, less85 = 0, less80 = 0;
		for (int i = 0; i < x.second.size(); i++)
		{
			if (x.second[i] < 0.99f)
				less99++;
			if (x.second[i] < 0.98f)
				less98++;
			if (x.second[i] < 0.95f)
				less95++;
			if (x.second[i] < 0.90f)
				less90++;
			if (x.second[i] < 0.85f)
				less85++;
			if (x.second[i] < 0.80f)
			{
				less80++;
				less80_out << img_list[i] << " " << x.first << " " << x.second[i] << std::endl;
			}
			if (x.second[i] < min_cos)
				min_cos = x.second[i];
		}

		statistics_table_maker.rowPushLine(x.first, {less99 ,less98 ,less95,less90,less85,less80}, min_cos, x.second.size());

	}

	statistics_table_maker.show();

	return 0;
}
