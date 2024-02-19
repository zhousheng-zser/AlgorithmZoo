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
#include "GenPipline.hpp"
#include "postprocessing_register.hpp"
#include "argparse/argparse.hpp"

using namespace glasssix;
#define GetShowRatio(visual_img) std::min(float(1920.f / visual_img.cols), float(1080.f / visual_img.rows)) * 0.75
#define ShowResize(visual_img, showRatio) cv::resize(visual_img, visual_img, cv::Size(), showRatio, showRatio)

struct RunConfig
{
	std::string net_A = "";
	std::string net_B = "";
	std::string net_A_postprocess = "";
	std::string net_B_postprocess = "";
	std::string dataset = "";
	std::string outputMap ="";
	bool convertBGR = false;
	int imgReSize = -1;
};

int main(int argc, char* argv[])
{
	std::map<std::string, postprocessing_function> postprocessing_market;
	AddPostprocessing(postprocessing_market);

	show_usage(argc);
	printf("[INFO] POSTPROCESSING MARKET[%d]: { ", postprocessing_market.size());
	for (auto& ppf : postprocessing_market) std::cout << ppf.first << ", ";
	std::cout << "}" << std::endl;
	if (argc < 2) return false;


	ArgumentParser parser("RKNN MODEL TEST.");

	parser.add_keyword_arg("-t", "--test_model", "", true, "", "test model compare with base model");
	parser.add_keyword_arg("-tp", "--test_model_pp", "", false, "", "test model postprocessing");

	parser.add_keyword_arg("-b", "--base_model", "", true, "", "base model for benchmark");
	parser.add_keyword_arg("-bp", "--base_model_pp", "", false, "", "base model postprocessing");

	parser.add_keyword_arg("-d", "--dataset", "", true, "dataset.txt", "test dataset.txt");
	parser.add_keyword_arg("-m", "--output_map", "", false, "output_map.json", "model pair out nodes mapping");
	parser.add_keyword_arg("-c", "--convert_bgr", "", false, "0", "if to convert BGR in image preprocessing");
	parser.add_keyword_arg("-s", "--img_resize", "", false, "-1", "if to resize image in preprocessing");

	ArgumentResult args = parser.parse_args(argc, argv);

	RunConfig runConfig;
	runConfig.net_A = args.args["test_model"];
	runConfig.net_B = args.args["base_model"];
	runConfig.net_A_postprocess = args.args["test_model_pp"];
	runConfig.net_B_postprocess = args.args["base_model_pp"];

	runConfig.dataset = args.args["dataset"];
	runConfig.outputMap = args.args["output_map"];
	runConfig.convertBGR = std::stoi(args.args["convert_bgr"]) == 1;
	runConfig.imgReSize = std::stoi(args.args["img_resize"]);



	std::map<std::string, std::string> output_map;

	std::vector<std::string> phai;
	GenPipline pipline_A(runConfig.net_A);
	GenPipline pipline_B(runConfig.net_B);
	pipline_A.set_inference_time_cost(false);
	pipline_B.set_inference_time_cost(false);
	pipline_A.set_image_preprocess(runConfig.imgReSize, runConfig.convertBGR);
	pipline_B.set_image_preprocess(runConfig.imgReSize, runConfig.convertBGR);
	pipline_A.set_postprocessing(runConfig.net_A_postprocess, postprocessing_market);
	pipline_B.set_postprocessing(runConfig.net_B_postprocess, postprocessing_market);


	std::map<std::string, std::vector<float>> score_map;
	std::vector<std::string> img_list;
	load_line_txt(runConfig.dataset, img_list);

	if (output_map.empty())
	{
		auto_infer_output_map(pipline_A, pipline_B, img_list[0], output_map);
	}
	else
	{
		if (!runConfig.outputMap.empty())
		{
			nlohmann::json map_json = read_json(runConfig.outputMap);
			for (auto& x : map_json["output"].items())
				output_map[x.key()] = x.value();
		}
		std::cout << "- specify output map regulation : " << runConfig.outputMap << std::endl;
	}

	DiffStatistics diffStatistics;
	std::cout << std::endl;
	for (int idx = 0; idx < img_list.size(); idx++)
	{
		cv::Mat img = cv::imread(img_list[idx]);

		if (idx == 0)
		{
			if (runConfig.convertBGR) printf("- convert BGR order !\n");
			if (runConfig.imgReSize > 0)
			{
				printf("- resize image to %d * %d !\n", runConfig.imgReSize, runConfig.imgReSize);
				printf("  if program crash, check size^2 EQ rknn input shape !\n");
			}
		}
		auto results_pip_A = pipline_A.forward(img);
		auto results_pip_B = pipline_B.forward(img);

		for (auto& x : output_map)
		{
			float* B_out = results_pip_B[x.first]->mutable_cpu_data();
			float* A_out = results_pip_A[x.second]->mutable_cpu_data();
			CHECK_EQ(results_pip_B[x.first]->count(), results_pip_A[x.second]->count());
			int count = results_pip_B[x.first]->count();


			if (x.first == "score_out") {
				size_t vaild_counter = 0;
				float cos = fliter_score_CosineSimilarity(B_out, A_out, count, 0.1f, diffStatistics, vaild_counter);
				score_map[x.first].push_back(cos);
				printf("[%d/%d]-> fcos:%f : score_out[>0.1]%d | %s\n", idx, img_list.size(), cos, vaild_counter, img_list[idx].c_str());

			}
			else {
				float cos = CosineSimilarity(B_out, A_out, count);
				score_map[x.first].push_back(cos);
				printf("[%d/%d]-> cos:%f : %s(%s) | %s\n", idx, img_list.size(), cos, x.second.c_str(), x.first.c_str(), img_list[idx].c_str());
			}


		}
		printf("\n");
	}

	diffStatistics.print();

	std::ofstream less80_out("less80_samples.txt");

	TableMaker statistics_table_maker(runConfig.net_A);

	for (auto& x : score_map)
	{

		float min_cos = 1.f;
		int less99 = 0, less98 = 0, less95 = 0, less90 = 0, less85 = 0, less80 = 0, less70 = 0;
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
			if (x.second[i] < 0.70f)
				less70++;
			if (x.second[i] < min_cos)
				min_cos = x.second[i];
		}

		statistics_table_maker.rowPushLine(x.first, {less99 ,less98 ,less95,less90,less85,less80,less70}, min_cos, x.second.size());

	}

	statistics_table_maker.show();

	return 0;
}
