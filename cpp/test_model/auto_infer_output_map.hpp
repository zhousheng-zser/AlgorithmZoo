#pragma once
#ifndef _AUTOMATIC_OUTPUTS_MAP_HPP_
#define _AUTOMATIC_OUTPUTS_MAP_HPP_
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
std::map<std::string, std::string> auto_infer_output_map_(
	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& results_rknn,
	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>& results_excb);


bool auto_infer_output_map(glasssix::rknnwrapper::rknn_wrapper& rknn_pipeline,
	glasssix::excalibur::pipeline<float>& excalibur_pipeline, 
	std::string fimg, std::map<std::string, std::string>& output_map);


#endif //!_AUTOMATIC_OUTPUTS_MAP_HPP_