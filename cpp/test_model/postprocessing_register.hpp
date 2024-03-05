#pragma once
#ifndef _CONCAT_FUNCTIONS_
#define _CONCAT_FUNCTIONS_

#include<vector>
#include<opencv2/opencv.hpp>
#include <Primitives/tensor.hpp>
#include "GenPipline.hpp"
using namespace glasssix;

class Postprocessing {
public:
	using TensorSptr = std::shared_ptr<memory::tensor<float>>;
	virtual const std::map<std::string, PostprocessingFunction> parser_postprocessing_dump() const = 0;
};

class PostprocessingRegister {
public:
	PostprocessingRegister(std::shared_ptr<Postprocessing> pplugin);
};

static std::vector<std::shared_ptr<Postprocessing>>& postprocessing_list_instance();

#define REGISTE_POSTPROCESSING(CLASS_NAME) \
	static PostprocessingRegister postprocessing_##CLASS_NAME##_register(std::shared_ptr<Postprocessing>{new CLASS_NAME});

std::map<std::string, PostprocessingFunction> AddPostprocessing();

void AddPostprocessing(std::map<std::string, PostprocessingFunction>&);

void DumpShowPostprocessingMarket(const std::map<std::string, PostprocessingFunction>& postprocessing_market);

#endif //!_CONCAT_FUNCTIONS_