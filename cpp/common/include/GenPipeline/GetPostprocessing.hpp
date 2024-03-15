#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
#include <Primitives/tensor.hpp>

#ifndef _GENERAL_PIPELINE_TOOLS_HPP_
#include "GenPipeTools.hpp"
#endif // !_GENERAL_PIPELINE_TOOLS_HPP_


using PostprocessingFunction = std::function<
	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>
	(std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>&)>;


EXPORT_EXCALIBUR_PRIMITIVES std::map<std::string, PostprocessingFunction> GetPostprocessingMarket();


static inline void DumpShowPostprocessingMarket(const std::map<std::string, PostprocessingFunction>& postprocessing_market)
{
	printf("[INFO] POSTPROCESSING MARKET[%d]: { ", postprocessing_market.size());
	for (auto&& ppf : postprocessing_market) std::cout << ppf.first << ", ";
	std::cout << "}" << std::endl;
}
