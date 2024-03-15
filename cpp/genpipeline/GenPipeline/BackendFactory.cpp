#include <map>
#include <memory>  
#include <string>  
#include <functional>  
#include <unordered_map>
#include <unordered_set>
#include <opencv2/opencv.hpp>
#include <Primitives/tensor.hpp>
#include <Primitives/logger.hpp>
#include <Primitives/abi/exceptions.hpp>
#include <GenPipeline/BackendFactory.hpp>

using namespace glasssix;

bool BackendFactoryRegistry::does_backend_support_model_format_(std::vector<std::string> model_exts, const std::pair<std::string, std::unordered_set<std::string>>& model_format_mapping) {
	bool is_hint{ false };
	for (auto& model_ext : model_exts) {
		if (*model_ext.begin() != '.') model_ext = '.' + model_ext;
		if (model_format_mapping.second.count(model_ext)) {
			is_hint = true; //is acceptableFormat, match successfully
		}
	}
	return is_hint;
}

BackendFactoryRegistry& BackendFactoryRegistry::getInstance() {
	static BackendFactoryRegistry instance;
	return instance;
}

void BackendFactoryRegistry::registerBackend(const std::string& name, const std::vector<std::string> modelFormats, CreatorFunction factory) {
	creators_[name] = std::move(factory);

	std::unordered_set<std::string> modelFormats_with_check;
	for (auto modelFormat : modelFormats) {
		if (*modelFormat.begin() != '.') modelFormat = '.' + modelFormat;
		modelFormats_with_check.insert(modelFormat);
	}
	backend_to_supported_model_formats_[name] = modelFormats_with_check;
}

std::string BackendFactoryRegistry::BackendFormatsHint(const std::vector<std::string>& model_exts) {
	size_t hint_counter = 0;
	std::string matchBackend;

	if (backend_to_supported_model_formats_.empty()) {
		throw glasssix::exposing::abi_invalid_argument("The registered Backends is empty.");
	}

	for (const auto& model_format_mapping : backend_to_supported_model_formats_) {
		if (does_backend_support_model_format_(model_exts, model_format_mapping)) {
			hint_counter++;
			if (hint_counter < 2) {
				matchBackend = model_format_mapping.first;
			}
			else {
				std::string exceptionsInfo = "Bad Backends modelFormats intersection.\
                        Different backends should not have the same acceptance format. Please check beckend " + matchBackend + " and " + model_format_mapping.first;
				throw glasssix::exposing::abi_invalid_operation(exceptionsInfo.c_str());
			}
		}
	}

	// Check if there exists a matching InferBackend.
	if (hint_counter == 0) {
		throw glasssix::exposing::abi_invalid_argument("No matching backends found for the given model extensions.");
	}

	return matchBackend;
}

std::unique_ptr<InferBackend> BackendFactoryRegistry::createBackend(const std::string& name) const {
	auto it = creators_.find(name);
	if (it != creators_.end()) {
		return it->second();
	}
	else {
		throw glasssix::exposing::abi_invalid_argument("Invalid bckend name or empty backend creators_.");
	}
}

std::unordered_set<std::string> BackendFactoryRegistry::getBackendMenu() {
	std::unordered_set<std::string> backends_menu;
	for (const auto& crt : creators_) {
		backends_menu.insert(crt.first);
	}
	return backends_menu;
}