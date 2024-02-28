#pragma once
//#ifndef _AUTOMATIC_OUTPUTS_MAP_HPP_
//#define _AUTOMATIC_OUTPUTS_MAP_HPP_

#include "Excalibur/pipeline.hpp"
#ifdef USE_RKNNAPI
//#if 0
#include "RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "RKNN2Wrapper/rknn2_wrapper.hpp"
#endif
#include "Primitives/tensor_conversions.hpp"
#include <opencv2/opencv.hpp>
#include <map>
#include <set>
#include "dbg.h"
#include "GenPipline.hpp"
#include "numpy_extensor/numpyExtensor.hpp"

template<typename T>
std::string printVect(std::vector<T> vec) {
	if (vec.empty()) {
		return "{}";
	}
	else {
		std::stringstream ss;
		ss << "{ ";
		for (auto& v : vec) {
			ss << v << ", ";
		}
		auto sstr = ss.str();
		return sstr.substr(0, sstr.find_last_of(',')) + " }";
	}

}

// true: can not judge out map
static inline std::map<std::string, std::string> output_map_analyse(
	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& results_test,
	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& results_base)
{
	std::map<std::string, std::string> output_map;

	auto printOutsMap = [](const std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& rst_map, std::string info) {
		std::cout << info << " outs map : {" << std::endl;
		for (auto& node_p : rst_map) {
			std::cout << "\t\"" << node_p.first << '"' << ":" << printVect(node_p.second->data_shape()) << std::endl;
		}
		std::cout << "}" << std::endl;
	};

	std::cout << "\n-------------" << std::endl;
	printOutsMap(results_base, "BASE pipline");
	printOutsMap(results_test, "TEST pipline");
	std::cout << "-------------" << std::endl;

	// auto judge

	if (results_test.size() == 1 && results_base.size() == 1) {
		auto& single_test = *results_test.begin();
		auto& single_base = *results_base.begin();
		if (single_test.second->count() == single_base.second->count()) {
			output_map[single_base.first] = single_test.first;
			return output_map;
		}
	}

	if (results_test.size() != results_base.size()) return output_map;

	std::set<std::vector<int>> shapes_rknn;
	std::set<std::vector<int>> shapes_excb;

	auto shapes_statistics = [](const std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& rst_map, std::set<std::vector<int>>& shapes)->bool {
		for (auto& node : rst_map) {
			shapes.insert(node.second->data_shape());
		}

		return shapes.size() != rst_map.size();
	};

	if (shapes_statistics(results_test, shapes_rknn)) return output_map;
	if (shapes_statistics(results_base, shapes_excb)) return output_map;

	if (shapes_rknn == shapes_excb) {
		for (auto& single_test : results_test) {
			for (auto& single_base : results_base) {
				if (single_base.second->data_shape() == single_test.second->data_shape())
					output_map[single_base.first] = single_test.first;
			}
		}

		return output_map;
	}

	return output_map;
}

static inline void auto_infer_output_map(std::shared_ptr<GenPiplineInterface> pip_TEST, std::shared_ptr<GenPiplineInterface> pip_BASE, std::string fimg, std::map<std::string, std::string>& output_map)
{
	cv::Mat img = cv::imread(fimg);
	auto results_TEST = pip_TEST->forward(img);
	auto results_BASE = pip_BASE->forward(img);
	//npy::SAVE_TENSOR_TO_NUMPY(results_A.begin()->second, "/home/glasssix/yhc/test_model/pedestrain/concat_a.npy");
	//npy::SAVE_TENSOR_TO_NUMPY(results_B.begin()->second, "/home/glasssix/yhc/test_model/pedestrain/concat_b.npy");

	//if (results_A.size() || results_B.size()) {
	//	throw glasssix::exposing::abi_not_implemented("Invalid pipline output empty!");
	//}

	auto automic_map = output_map_analyse(results_TEST, results_BASE);
	if (automic_map.empty()) {
		std::cout << "-- failed to get output map automatically !" << std::endl;
		std::cout << "-- please specify output map regulation (xx.json) by hand" << std::endl;
		std::cout << "// xx.json" << std::endl;
		std::cout << "{" << std::endl;
		std::cout << "	\"output\":" << std::endl;
		std::cout << "	{" << std::endl;
		std::cout << "		\"excalibur_ouput_1\":\"rknn_ouput_1\"" << std::endl;
		std::cout << "	}" << std::endl;
		std::cout << "}" << std::endl;
	}
	else {
		output_map = automic_map;
		std::cout << "succeed to get output map automatically ~" << std::endl;
		std::cout << "<auto_map> " << pip_BASE->pipTypeInfo() << " - " << pip_TEST->pipTypeInfo() << std::endl;

		for (auto output_map_elm : output_map) {
			std::cout << "\t" << output_map_elm.first << ':' << printVect(results_BASE[output_map_elm.first]->data_shape())
				<< " - " << output_map_elm.second << ':' << printVect(results_TEST[output_map_elm.second]->data_shape()) << std::endl;
		}
		std::cout << "</auto_map>" << std::endl;
		std::cout << "-------------" << std::endl;
	}

}

//#endif //!_AUTOMATIC_OUTPUTS_MAP_HPP_