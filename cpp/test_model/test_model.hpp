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



static inline void show_usage(int argc) {
	if (argc < 2)
	{
		std::cout << "USAGE >./test_model -t minifce.rknn -b face.phai -tp yolo8 -bp yolo8 -d add_1000.txt -c 0 -s 1280 -m outs_map.json" << std::endl
			<< "     -b: base model  // (if use excalibur model, xx.phai = xx.phai + xx.racy, ask excalibur model files name must be equal !)" << std::endl
			<< "     -t: test model" << std::endl
			<< "     -bp: assign postprocessing for base model from POSTPROCESSING MARKET" << std::endl
			<< "     -tp: assign postprocessing for test model from POSTPROCESSING MARKET" << std::endl
			<< "     -d: dataset file" << std::endl
			<< "     -c: set 1 to convert BGR " << std::endl
			<< "     -s: resize image to S*S" << std::endl
			<< "     -m: oput nodes mapping file" << std::endl
			<< "#### note:" << std::endl
			<< "     outmap.json e.g." << std::endl
			<< "     {" << std::endl
			<< "         \"output\":" << std::endl
			<< "         {" << std::endl
			<< "             \"ouput0\":\"rknn_out0_0\"," << std::endl
			<< "             \"ouput1\":\"rknn_out1_0\"," << std::endl
			<< "         }" << std::endl
			<< "     }" << std::endl;

		std::cout << "#### (simple net test uasge) >./test_model -t minifce.rknn -b face.phai -d add_1000.txt" << std::endl << std::endl;
	}
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

	constexpr float zero_epsilo = 0.000001f; //1e-6

	if (std::abs(dot) < zero_epsilo && std::sqrt(emb1_sum) * std::sqrt(emb2_sum) < zero_epsilo)
	{
		return 1.f; // rst = 0.f / 0,f -> 1
	}
	else
	{
		dot /= std::max(std::sqrt(emb1_sum) * std::sqrt(emb2_sum),
			std::numeric_limits<float>::epsilon());
		return dot;
	}
}

struct DiffStatistics
{
	size_t count = 0;;
	std::array<size_t, 100> step_table{ 0 }; // int count = step_table[82] means avg df=0.32`s number is count, step_table[25] ~ df=0.-25

	void set(double v) {
		int idx = (v + 0.5)*100;
		idx = std::min(idx, 99);
		idx = std::max(idx, 0);
		step_table[idx]++;
		count++;
	}

	void print() {
		std::cout << "diff statistics " << count << std::endl;
		std::cout << "---------------------- " << std::endl;
		for (int i = 9; i >= 0; i--) {
			std::cout << "| ";
			for (int j = 9; j >= 0; j--) {
				int idx = i * 10 + j;
				if (idx < 10) std::cout << ' ';
				std::cout << idx - 50 << " , "
					<< step_table[idx]
					<< " | ";
			}
			std::cout << std::endl;
		}
		std::cout << "---------------------- " << std::endl;

		for (int i = 0; i < 100; i++) {
			std::cout << step_table[i] << ", ";
		}
		std::cout << "\n---------------------- " << std::endl;

	}
};


static inline float fliter_score_CosineSimilarity(float* emb1, float* emb2, int len, float thresh, DiffStatistics& diffStatistics, size_t& vaild_counter)
{
	std::vector<double> pos_dfs;
	std::vector<double> ngt_dfs;

	float dot = 0.f;
	float emb1_sum = 0.f;
	float emb2_sum = 0.f;
	for (size_t i = 0; i < len; i++) {
		if (thresh > emb1[i] && thresh > emb2[i])continue;
		vaild_counter++; //greater than thresh, counter++
		double df = emb2[i] - emb1[i];
		//std::cout << "f=" << std::fixed << std::setprecision(3) << emb1[i] << ", i=" << emb2[i] << ", df=" << df << std::endl;
		if (df > 0.0002f) {
			pos_dfs.push_back(df);
			diffStatistics.set(df);
		}
		else if(df < -0.0002f) {
			ngt_dfs.push_back(df);
			diffStatistics.set(df);
		}
		else {
			diffStatistics.set(0);
		}

		dot += emb1[i] * emb2[i];
		emb1_sum += emb1[i] * emb1[i];
		emb2_sum += emb2[i] * emb2[i];
	}


	float avg_pos_df = 0;
	float avg_ngt_df = 0;
	for (auto v : pos_dfs) {
		avg_pos_df += v / pos_dfs.size();
	}
	for (auto v : ngt_dfs) {
		avg_ngt_df += v / ngt_dfs.size();
	}

	//std::cout << ">>> avg df*100 : " << int(avg_pos_df * 100) << ", " << int(avg_ngt_df * 100) << std::endl;

	constexpr float zero_epsilo = 0.000001f; //1e-6

	if (std::abs(dot) < zero_epsilo && std::sqrt(emb1_sum) * std::sqrt(emb2_sum) < zero_epsilo)
	{
		return 1.f; // rst = 0.f / 0,f -> 1
	}
	else
	{
		dot /= std::max(std::sqrt(emb1_sum) * std::sqrt(emb2_sum),
			std::numeric_limits<float>::epsilon());
		return dot;
	}

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