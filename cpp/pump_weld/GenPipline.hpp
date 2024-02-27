#pragma once
#ifndef _GENERAL_PIPLINE_HPP_
#define _GENERAL_PIPLINE_HPP_

#ifdef EXPERIMENTAL_FILESYSTEM
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
//using namespace std::filesystem;
namespace fs = std::filesystem;
#endif // EXPERIMENTAL_FILESYSTEM
#include <Primitives/abi/exceptions.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <Primitives/logger.hpp>
#include <Primitives/tensor.hpp>
#include <abi/exceptions.hpp>
#include <opencv2/opencv.hpp>
#include <vector>

#include "postprocessing_register.hpp"

#include "Backends/BackendFactory.hpp"

#include "Excalibur/pipeline.hpp"

using namespace glasssix;

namespace GenPiplineTools {

	static auto spilt_file_name_ext_(std::string model) {
		struct {
			std::string file_name;
			std::string file_ext;
		} rst;

		if (model.empty() || model.rfind('.') == -1)
		{
			//LOG(FATAL) << "Input model path format error.";
			throw glasssix::exposing::abi_invalid_argument("Input model path format error.");
		}
		else {
			rst.file_name = model.substr(0, model.find_last_of('.'));
			rst.file_ext = model.substr(model.find_last_of('.'));
		}
		return rst;
	}

	static cv::Mat letter_image(cv::Mat img, int hope_w, int hope_h, bool& is_horizon_pad, int& pad_val, float& resize_scale, bool if_cvtColor = false)
	{
		cv::Mat resize_img;
		int H = img.rows;
		int W = img.cols;

		if (hope_w <= 0 || hope_h <= 0 || (H == hope_h && W == hope_w)) {
			//无效hope_hw时或者HW等于hope_hw时不做任何尺寸操作。
			resize_img = img;
			pad_val = 0;
		}
		else {
			float ratio_w = (float)W / (float)hope_w;
			float ratio_h = (float)H / (float)hope_h;
			if (ratio_w == ratio_h) {
				cv::resize(img, resize_img, cv::Size2i{ hope_w, hope_h });
				pad_val = 0;

			}
			else if (ratio_w > ratio_h) {
				int new_x = hope_w;
				int new_y = (int)(H / ratio_w);
				int pad1 = (int)((hope_h - new_y) / 2);
				int pad2 = hope_h - new_y - pad1;
				pad_val = (pad1 + pad2) / 2;
				is_horizon_pad = true;
				resize_scale = ratio_w;
				cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
				cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
			}
			else {
				int new_y = hope_h;
				int new_x = (int)(W / ratio_h);
				int pad1 = (int)((hope_w - new_x) / 2);
				int pad2 = hope_w - new_x - pad1;
				pad_val = (pad1 + pad2) / 2;
				is_horizon_pad = false;
				resize_scale = ratio_h;
				cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
				cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
			}
		}

		if (if_cvtColor)
			cv::cvtColor(resize_img, resize_img, cv::COLOR_BGR2RGB);

		return resize_img;
	}

	static cv::Mat letter_image(cv::Mat img, int hope_w, int hope_h, bool if_cvtColor = false)
	{
		bool is_horizon_pad_temp;
		int pad_val_temp;
		float resize_scale;
		return letter_image(img, hope_w, hope_h, is_horizon_pad_temp, pad_val_temp, resize_scale, if_cvtColor);
	}
}


// Proxy pattern
class GenPiplineInterface {
public:
	virtual ~GenPiplineInterface() = default;
	virtual std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(cv::Mat image) = 0;
	virtual void handset_possible_normalization(std::array<float, 3> means, std::array<float, 3> stands) = 0;
	virtual const std::string pipTypeInfo() = 0;
	virtual const std::string version() = 0;
};


class GenPipline: public GenPiplineInterface {
	std::unique_ptr<InferBackend> backend;

	void backend_create_(const std::vector<std::string>& model_exts) {
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

public:
	GenPipline() {}

	GenPipline(std::string model, int device = -1) :GenPipline() {
		auto [name, ext] = GenPiplineTools::spilt_file_name_ext_(model);
		std::vector<std::string> model_exts{ ext };
		backend_create_(model_exts);
		int ret = backend->initModel(model, device);
		if (ret) {
			std::string exceptionsInfo = "Init model failed :" + model;
			throw glasssix::exposing::abi_invalid_argument(exceptionsInfo.c_str());
		}
	}

	GenPipline(std::string arch, std::string weight, int device = -1) :GenPipline() {
		auto [arch_name, arch_ext] = GenPiplineTools::spilt_file_name_ext_(arch);
		auto [weight_name, weight_ext] = GenPiplineTools::spilt_file_name_ext_(weight);
		std::vector<std::string> model_exts{ arch_ext, weight_ext };
		backend_create_(model_exts);
		int ret = backend->initModel(arch, weight, device);
		if (ret) {
			std::string exceptionsInfo = "Init model failed :" + arch + " " + weight;
			throw glasssix::exposing::abi_invalid_argument(exceptionsInfo.c_str());
		}
	}

	void handset_possible_normalization(std::array<float, 3> means, std::array<float, 3> stands) override {
		if (!backend) {
			throw glasssix::exposing::abi_not_initialized("Empty infer backend.");
		};

		if (backend ->handset_possible_normalization(means, stands) == InferBackend::SupportHandNormaliztion::Enable) {
			LOG(INFO) << "Support Hand assgin normalization param";
		}
		else {
			LOG(INFO) << "Unsupport hand assgin normalization param";
		}
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(cv::Mat image) override {
		if (backend) {
			return backend->forward(image); //success
		}
		else {
			throw glasssix::exposing::abi_not_initialized("Empty infer backend."); //fail
		}		
	}

	static std::unordered_set<std::string> dump_backend_menu(bool if_print = false) {
		auto& registry = BackendFactoryRegistry::getInstance();

		auto backend_menu = registry.getBackendMenu();
		if (if_print) {
			printf("BackendFactory has registered backend: ");
			for (const auto& bk : backend_menu){
				printf("%s, ", bk.c_str());
			}
			printf("\n");
		}

		return backend_menu;
	}

	const std::string pipTypeInfo() override {
		if (backend) {
			return backend->getBackendTypeName(); //success
		}
		else {
			return "empty infer backend"; //fail
		}
	}

	const std::string version() override {
		if (backend) {
			return backend->getVersion(); //success
		}
		else {
			return "empty infer backend"; //fail
		}
	}

	bool if_backend_empty() {
		return (!backend);
	}

};


/// <summary>
/// PrePostProcessGenPipline: Proxy for GenPiplineInterface, wrapping GenPipline to add pre/post-processing steps.
/// </summary>
class PrePostProcessGenPipline: public GenPiplineInterface {
public:

	using ForwardResultType = std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>>;

	PrePostProcessGenPipline() :pipline_{nullptr}
	{
	}

	PrePostProcessGenPipline(std::shared_ptr<GenPiplineInterface> pipline) : pipline_(pipline)
	{
	}

	void set_pipline(std::shared_ptr<GenPiplineInterface> pipline) {
		pipline_ = pipline;
	}

	void set_image_preprocess(std::function<cv::Mat(cv::Mat)> image_preprocess_function) {
		image_preprocess_function_ = image_preprocess_function;
	}

	void check_set_postprocessing(std::map<std::string, PostprocessingFunction>& postprocessing_market, std::string ppfunc_name) {
		if (postprocessing_market.count(ppfunc_name)) {
			std::cout << pipTypeInfo() << " pipline load postprocessing \"" << ppfunc_name << "\"" << std::endl;
			set_postprocessing(postprocessing_market[ppfunc_name]);
		}
		else if (!ppfunc_name.empty()) {
			std::cout << "postprocessing market not exits \"" << ppfunc_name << "\"" << std::endl;
			std::cout << "[INFO] POSTPROCESSING MARKET: { ";
			for (auto& ppf : postprocessing_market) std::cout << ppf.first << ", ";
			std::cout << "}" << std::endl;
			std::cout << "please check postprocessing config!" << std::endl;
		}
	};

	void set_postprocessing(PostprocessingFunction pp_function) {
		postprocessing_function_ = pp_function;
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(cv::Mat image) override {
		ForwardResultType rst_map;

		if (pipline_ != nullptr) {
			if (image_preprocess_function_ != nullptr) {
				image = image_preprocess_function_(image);
			}
			rst_map = pipline_->forward(image);

			if (postprocessing_function_ != nullptr) {
				rst_map = postprocessing_function_(rst_map);
			}
		}

		return rst_map;
	}

	void handset_possible_normalization(std::array<float, 3> means, std::array<float, 3> stands) override {
		if (pipline_) {
			return pipline_->handset_possible_normalization(means, stands);
		}
	}

	const std::string pipTypeInfo() override {
		if (pipline_) {
			return pipline_->pipTypeInfo();
		}
		else {
			return "";
		}
	}

	const std::string version() override {
		if (pipline_) {
			return pipline_->version();
		}
		else {
			return "";
		}
	}

private:
	std::shared_ptr<GenPiplineInterface> pipline_;
	std::function<cv::Mat(cv::Mat)> image_preprocess_function_;
	PostprocessingFunction postprocessing_function_;
};


#endif //!_GENERAL_PIPLINE_HPP_