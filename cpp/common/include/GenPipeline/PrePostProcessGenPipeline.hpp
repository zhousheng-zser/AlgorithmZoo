#pragma once
#include <type_traits>
#include <Primitives/abi/exceptions.hpp>
#include <Primitives/logger.hpp>
#include <Primitives/tensor.hpp>
#include <opencv2/opencv.hpp>
#include <vector>

//#include "numpy_extensor/numpyExtensor.hpp"
#include "GenPipeline.hpp"

using namespace glasssix;

using PostprocessingFunction = std::function<
	std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>>
	(std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>>&)>;

/// <summary>
/// PrePostProcessGenPipeline: ProxyPipeline Type, wrapping GenPipeline to add pre/post-processing steps.
/// </summary>
class PrePostProcessGenPipeline: public GenPipelineInterface {
	using ChronoClock = std::chrono::high_resolution_clock;
	using TimePoint = std::chrono::time_point<ChronoClock>;
	TimePoint start_{};

public:
	PrePostProcessGenPipeline() :pipeline_{ nullptr } {}
	PrePostProcessGenPipeline(std::shared_ptr<GenPipelineInterface> pipeline) : pipeline_(pipeline) {}
	~PrePostProcessGenPipeline() {}

public:
	virtual void set_pipeline(std::shared_ptr<GenPipelineInterface> pipeline) {
		pipeline_ = pipeline;
	}

	void set_image_preprocess(std::function<cv::Mat(cv::Mat)> image_preprocess_function) {
		image_preprocess_function_ = image_preprocess_function;
	}

	void set_postprocessing(const std::map<std::string, PostprocessingFunction>& postprocessing_market, std::string ppfunc_name) {
		if (postprocessing_market.count(ppfunc_name)) {
			std::cout << pipTypeInfo() << " pipeline load postprocessing \"" << ppfunc_name << "\"" << std::endl;
			set_postprocessing(postprocessing_market.at(ppfunc_name));
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
		if (pp_function != nullptr) {
			postprocessing_function_ = pp_function;
		}
		else {
			LOG(WARNING) << "Setting Empty Postprocessing.";
		}
	}

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(cv::Mat image) {
		std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> rst_map;

		if (pipeline_ != nullptr) {
			if (image_preprocess_function_ != nullptr) {
				image = image_preprocess_function_(image);
			}

			if (use_forward_probe_) {
				start_ = ChronoClock::now();
			}

			rst_map = pipeline_->forward(image);

			if (use_forward_probe_) {
				probe_infr_time_cost += std::chrono::duration_cast<std::chrono::milliseconds>(ChronoClock::now() - start_);
				infr_cost_add_count++;
			}

			if (postprocessing_function_ != nullptr) {
				start_ = ChronoClock::now();
				rst_map = postprocessing_function_(rst_map);
				if (use_forward_probe_) {
					probe_post_time_cost += std::chrono::duration_cast<std::chrono::milliseconds>(ChronoClock::now() - start_);
					post_cost_add_count++;
				}
			}
		}

		return rst_map;
	}

	void manual_possible_normalization(std::array<float, 3> means, std::array<float, 3> stands) {
		if (pipeline_)
			 pipeline_->manual_possible_normalization(means, stands);
	}

	void manual_possible_normalization(float means, float stnds) {
		if (pipeline_)
			pipeline_->manual_possible_normalization({ means ,means, means }, { stnds ,stnds, stnds });
	}

	const std::string pipTypeInfo() override {
		if (pipeline_) {
			return pipeline_->pipTypeInfo();
		}
		else {
			return "";
		}
	}

	const std::string version() override {
		if (pipeline_) {
			return pipeline_->version();
		}
		else {
			return "";
		}
	}

	void profiler_tic() {
		probe_clear_();
		use_forward_probe_ = true;
	}

	void profiler_toc() {
		if (use_forward_probe_) {
			LOG(INFO) << "Loop " << infr_cost_add_count << ", show_avg_infr_cost = " << (infr_cost_add_count ? probe_infr_time_cost.count() * 1.f / infr_cost_add_count : 0) << " ms";
			LOG(INFO) << "Loop " << post_cost_add_count << ", show_avg_post_cost = " << (post_cost_add_count ? probe_post_time_cost.count() * 1.f / post_cost_add_count : 0) << " ms";
		}
		probe_clear_();
		use_forward_probe_ = false;
	}

	template<typename RealPipeline = GenPipeline>
	static inline std::shared_ptr<PrePostProcessGenPipeline> mkSharePipeline(std::string arch, std::string weight, int device = -1) {
		return mkSharePipelineCommon_<PrePostProcessGenPipeline, RealPipeline>(arch, weight, device);
	}

	template<typename RealPipeline = GenPipeline>
	static inline std::shared_ptr<PrePostProcessGenPipeline> mkSharePipeline(std::string model, int device = -1) {
		return mkSharePipelineCommon_<PrePostProcessGenPipeline, RealPipeline>(model, device);
	}

private:
	std::shared_ptr<GenPipelineInterface> pipeline_;
	std::function<cv::Mat(cv::Mat)> image_preprocess_function_;
	PostprocessingFunction postprocessing_function_;

	bool use_forward_probe_ = false;
	size_t infr_cost_add_count = 0;
	size_t post_cost_add_count = 0;
	std::chrono::milliseconds probe_infr_time_cost{0};
	std::chrono::milliseconds probe_post_time_cost{0};

	void probe_clear_() {
		infr_cost_add_count = 0;
		post_cost_add_count = 0;
		probe_infr_time_cost = std::chrono::milliseconds{ 0 };
		probe_post_time_cost = std::chrono::milliseconds{ 0 };
	}
};

