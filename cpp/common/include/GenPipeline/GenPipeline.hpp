#pragma once
#ifndef _GENERAL_PIPELINE_HPP_
#define _GENERAL_PIPELINE_HPP_
#include <Primitives/tensor.hpp>
#include <opencv2/opencv.hpp>
#include <vector>
#include <unordered_set>

#include "BackendFactory.hpp"

// Proxy pattern
class GenPipelineInterface {
public:
	virtual ~GenPipelineInterface() = default;
	virtual std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(cv::Mat image) = 0;
	virtual void manual_possible_normalization(std::array<float, 3> means, std::array<float, 3> stands) = 0;
	virtual const std::string pipTypeInfo() = 0;
	virtual const std::string version() = 0;

protected:
	/**
	 * @brief Simplifies the construction process for ProxyPipline and its dependent RealPipline. Not recommended for use in RealPipline.
	 *
	 * @tparam ProxyPipline The proxy class type that must derive from GenPiplineInterface.
	 * @tparam RealPipline The actual class type that must derive from GenPiplineInterface.
	 * @tparam Args The types of the constructor parameters.
	 * @param args The parameters for the RealPipline constructor.
	 * @return A shared pointer pointing to ProxyPipline.
	 * @note ProxyPipline and RealPipline must not be of the same type.
	 */
	template<typename ProxyPipeline, typename RealPipeline, typename... Args>
	static inline std::shared_ptr<ProxyPipeline> mkSharePipelineCommon_(Args&&... args) {
		static_assert(std::is_base_of_v<GenPipelineInterface, ProxyPipeline>, "Type ProxyPipeline must be derived from GenPipelineInterface");
		static_assert(std::is_base_of_v<GenPipelineInterface, RealPipeline>, "Type RealPipeline must be derived from GenPipelineInterface");
		static_assert(!std::is_same_v<ProxyPipeline, RealPipeline>, "Type RealPipeline can not be ProxyPipeline");
		return std::make_shared<ProxyPipeline>(std::make_shared<RealPipeline>(args...));
	}

};

/// <summary>
/// GenPipeline: RealPipeline Type
/// <summary>
class EXPORT_EXCALIBUR_PRIMITIVES GenPipeline: public GenPipelineInterface {
	std::unique_ptr<InferBackend> backend;

private:
	void backend_create_(const std::vector<std::string>& model_exts);

public:
	GenPipeline() = default;

	GenPipeline(const GenPipeline& other) = delete;

	GenPipeline& operator=(const GenPipeline& other) = delete;

	GenPipeline(std::string model, int device = -1);

	GenPipeline(std::string arch, std::string weight, int device = -1);

	void manual_possible_normalization(std::array<float, 3> means, std::array<float, 3> stands) override final;

	std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(cv::Mat image) override final;

	static std::unordered_set<std::string> dump_backend_menu(bool if_print = false);

	const std::string pipTypeInfo() override final;

	const std::string version() override final;

	bool if_backend_empty();
};


#endif //!_GENERAL_PIPELINE_HPP_