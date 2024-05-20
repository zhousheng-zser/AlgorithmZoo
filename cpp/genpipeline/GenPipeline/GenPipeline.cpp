#pragma once
#include <Primitives/abi/exceptions.hpp>
#include <Primitives/logger.hpp>
#include <Primitives/tensor.hpp>
#include <opencv2/opencv.hpp>
#include <vector>
#include <GenPipeline/GenPipeline.hpp>

using namespace glasssix;

namespace {
	static auto spilt_file_name_ext_(std::string model) {
		struct {
			std::string file_name;
			std::string file_ext;
		} rst;

		if (model.empty() || model.rfind('.') == -1)
		{
			throw glasssix::exposing::abi_invalid_argument("Input model path format error.");
		}
		else {
			rst.file_name = model.substr(0, model.find_last_of('.'));
			rst.file_ext = model.substr(model.find_last_of('.'));
		}
		return rst;
	}
}

void GenPipeline::backend_create_(const std::vector<std::string>& model_exts)
{
	auto& registry = BackendFactoryRegistry::getInstance();
	std::string backendTypeName = registry.BackendFormatsHint(model_exts);
	backend = registry.createBackend(backendTypeName);
	if (!backend) {
		throw glasssix::exposing::abi_not_initialized("Failed to create backend.");
	}
	else {
		//std::cout << "Backend " << backendTypeName << " create successfully." << std::endl;
	}
}

GenPipeline::GenPipeline(std::string model, int device) :GenPipeline() {
	auto [name, ext] = spilt_file_name_ext_(model);
	std::vector<std::string> model_exts{ ext };
	backend_create_(model_exts);
	int ret = backend->initModel(model, device);
	if (ret) {
		std::string exceptionsInfo = "Init model failed :" + model;
		throw glasssix::exposing::abi_invalid_argument(exceptionsInfo.c_str());
	}
}

GenPipeline::GenPipeline(std::string arch, std::string weight, int device) :GenPipeline() {
	auto [arch_name, arch_ext] = spilt_file_name_ext_(arch);
	auto [weight_name, weight_ext] = spilt_file_name_ext_(weight);
	std::vector<std::string> model_exts{ arch_ext, weight_ext };
	backend_create_(model_exts);
	int ret = backend->initModel(arch, weight, device);
	if (ret) {
		std::string exceptionsInfo = "Init model failed :" + arch + " " + weight;
		throw glasssix::exposing::abi_invalid_argument(exceptionsInfo.c_str());
	}
}

GenPipeline::GenPipeline(const std::vector<std::string>& phai, std::string weight, int device) :GenPipeline() {
	auto [weight_name, weight_ext] = spilt_file_name_ext_(weight);
	std::vector<std::string> model_exts{ weight_ext };
	backend_create_(model_exts);
	int ret = backend->initModel(phai, weight, device);
	if (ret) {
		std::string exceptionsInfo = "Init model failed :" + weight;
		throw glasssix::exposing::abi_invalid_argument(exceptionsInfo.c_str());
	}
}

void GenPipeline::manual_possible_normalization(float mean, float stand) {
	manual_possible_normalization({ mean,mean,mean }, { stand,stand,stand });
}

void GenPipeline::manual_possible_normalization(std::array<float, 3> means, std::array<float, 3> stands) {
	if (!backend) {
		throw glasssix::exposing::abi_not_initialized("Empty infer backend.");
	};

	if (backend->manual_possible_normalization(means, stands) == InferBackend::SupportManualNormalization::Enable) {
		LOG(INFO) << "Support Manual assgin normalization param";
	}
	else {
		LOG(INFO) << "Unsupport Manual assgin normalization param";
	}
}

std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> GenPipeline::forward(cv::Mat image) {
	if (backend) {
		return backend->forward(image); //success
	}
	else {
		throw glasssix::exposing::abi_not_initialized("Empty infer backend."); //fail
	}
}

std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> GenPipeline::forward(const float* input_data, std::vector<int> data_shape, int order) {
	if (backend) {
		return backend->forward(input_data, data_shape, order); //success
	}
	else {
		throw glasssix::exposing::abi_not_initialized("Empty infer backend."); //fail
	}
}


std::unordered_set<std::string> GenPipeline::dump_backend_menu(bool if_print) {
	auto& registry = BackendFactoryRegistry::getInstance();

	auto backend_menu = registry.getBackendMenu();
	if (if_print) {
		printf("BackendFactory has registered backend: ");
		for (const auto& bk : backend_menu) {
			printf("%s, ", bk.c_str());
		}
		printf("\n");
	}

	return backend_menu;
}

const std::string GenPipeline::pipTypeInfo() {
	if (backend) {
		return backend->getBackendTypeName(); //success
	}
	else {
		return "empty infer backend"; //fail
	}
}

const std::string GenPipeline::version() {
	if (backend) {
		return backend->getVersion(); //success
	}
	else {
		return "empty infer backend"; //fail
	}
}

bool GenPipeline::if_backend_empty() {
	return (!backend);
}