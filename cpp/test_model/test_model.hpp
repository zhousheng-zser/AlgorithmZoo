#pragma once
#ifndef _MODEL_TEST_HPP_
#define _MODEL_TEST_HPP_
#include <fstream>
#include "json.hpp"
#include "dbg.h"
#include <algorithm>

#ifdef EXPERIMENTAL_FILESYSTEM
#include <experimental/filesystem>
//using namespace fs std::experimental::filesystem;
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
//using namespace std::filesystem;
namespace fs = std::filesystem;
#endif // EXPERIMENTAL_FILESYSTEM

static inline nlohmann::json read_json(std::string file) {
	fs::path ap(file);
	if (!fs::exists(ap))
	{
		//dbg(file);
		printf("not exists file %s !\n", file.c_str());
		exit(1);
	}

	std::ifstream json_file(file);
	std::string buffer(std::istreambuf_iterator<char>{json_file}, std::istreambuf_iterator<char>{});
	nlohmann::json rst_json = nlohmann::json::parse(buffer);
	return rst_json;
}



static inline void show_usage() {
	std::cout << "USAGE: input rknn excalibur dataset outmap e.g. [1] or [2] or [3] .." << std::endl << std::endl;
	
	std::cout << "[1]> ./test_model _.rknn _.phai _.racy dataset.txt // for single-out-node net test" << std::endl << std::endl;
	
	std::cout << "[2]> ./test_model _.rknn _.phai _.racy dataset.txt outmap.json // for multi-out-node net test, especially those nodes with same shape" << std::endl;
	std::cout << "     # outmap.json e.g." << std::endl;
	std::cout << "     {" << std::endl;
	std::cout << "         \"output\":" << std::endl;
	std::cout << "         {" << std::endl;
	std::cout << "             \"ouput0\":\"rknn_out0_0\"," << std::endl;
	std::cout << "             \"ouput1\":\"rknn_out1_0\"," << std::endl;
	std::cout << "         }" << std::endl;
	std::cout << "     }" << std::endl;
	std::cout << std::endl;
	
	std::cout << "[3]> ./test_model RunConfig.json" << std::endl;
	std::cout << "     # RunConfig.json e.g." << std::endl;
	std::cout << "     {" << std::endl;
	std::cout << "         \"testModel\":" << std::endl;
	std::cout << "         {" << std::endl;
	std::cout << "             \"rknn\":\"**.rknn\"," << std::endl;
	std::cout << "             \"phai\":\"**.phai\"," << std::endl;
	std::cout << "             \"racy\":\"**.racy\"," << std::endl;
	std::cout << "             \"dataset\":\"**.txt\"," << std::endl;
	std::cout << "             \"outputMap\":\"**.json\"" << std::endl;
	std::cout << "         }" << std::endl;
	std::cout << "     }" << std::endl;
	std::cout << std::endl;
}

static inline bool parse_param(int argc, char* argv[], std::unordered_map<std::string, std::string>& run_param_map)
{
	if (argc < 2)
	{
		show_usage();
		return false;
		dbg("exit parse_param argc < 2");
	}
	else if (argc == 2)
	{
		std::string param(argv[1]);
		std::cout << "- config test_model by json file " << param << std::endl;

		if (param.size() > 5 || param.find(".json") != std::string::npos)
		{
			auto json_map = read_json(param);

			
			if (!json_map.count("TestModel"))
			{
				// no exist
				printf("- config file not exist param \"TestModel\"\n");
				return false;
			}

			auto modelTest_param = json_map["TestModel"];
			auto param_push = [&run_param_map, &modelTest_param](std::string pkey, bool force = false)->bool {
				if (modelTest_param.count(pkey))
				{
					run_param_map[pkey] = modelTest_param[pkey];
				}
				else if (force)
				{
					printf("- parse_param : not exist param \"%s\"", pkey.c_str());
					return false; // false = error
				}
				return true; // true is ok :)
			};

			if(!param_push("rknn", true)) return false;
			if(!param_push("phai", true)) return false;
			if(!param_push("racy", true)) return false;
			if(!param_push("dataset", true)) return false;
			if(!param_push("outputMap", true)) return false;
			if(!param_push("imgReSize", false)) return false;
			if(!param_push("convertBGR", false)) return false;
		}
		else {
			std::cout << "- " << param << " isnt json file !" << std::endl;
			return false;
		}
	}
	else
	{
		run_param_map["rknn"] = argv[1];
		run_param_map["phai"] = argv[2];
		run_param_map["racy"] = argv[3];
		run_param_map["dataset"] = argv[4];
		run_param_map["outputMap"] = argc == 6 ? argv[5] : "";
	}
	return true;
}


template<typename T>
static inline float CosineSimilarity(T emb1, T emb2, int len)
{
	float dot = 0.f;
	float emb1_sum = 0.f;
	float emb2_sum = 0.f;
	for (size_t i = 0; i < len; i++) {
		dot += emb1[i] * emb2[i];
		emb1_sum += emb1[i] * emb1[i];
		emb2_sum += emb2[i] * emb2[i];
	}
	dot /= std::max(std::sqrt(emb1_sum) * std::sqrt(emb2_sum),
		std::numeric_limits<float>::epsilon());
	return dot;

}

static inline void load_line_txt(std::string _txt, std::vector<std::string>& lines) {
	std::ifstream in(_txt);
	std::string tempLine;
	if (!in.is_open()) {
		std::cout << "CANT OPEN FILE " << _txt << " !" << std::endl;
	}

	while (std::getline(in, tempLine))
	{
		if (!tempLine.empty())
			lines.push_back(tempLine);
		//std::string img_ = tempLine.substr(0, tempLine.find_first_of(' '));
	}
}

#endif //!_MODEL_TEST_HPP_