#pragma once
#include <map>
#include <GenPipeline/GetPostprocessing.hpp>
#include <Primitives/tensor.hpp>
#include "postprocessing_register.hpp"
using namespace glasssix;

std::map<std::string, PostprocessingFunction> GetPostprocessingMarket()
{
	std::map<std::string, PostprocessingFunction> postprocessing_map;
	for (auto p : PostprocessingRegister::postprocessing_instance())
	{
		auto tmp_pproce = p->parser_postprocessing_dump();
		postprocessing_map.insert(tmp_pproce.begin(), tmp_pproce.end());
	}
	return postprocessing_map;
}
