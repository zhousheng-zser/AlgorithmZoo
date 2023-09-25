#ifndef COMMMON_H
#define COMMMON_H

#include <cassert>
#include <iterator>
#include <memory>
#include <new>
#include <numeric>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <array>

#include "NvInfer.h"
#include "NvInferPlugin.h"
#include "NvOnnxParser.h"

#include <cuda_runtime_api.h>

#include <opencv2/opencv.hpp>

#include <Excalibur/pipeline.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_resize.hpp>
#include "Excalibur/operation_make_border.hpp"

#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>
#include "Primitives/logger.hpp"


#undef CHECK
#define CHECK(status)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        auto ret = (status);                                                                                           \
        if (ret != 0)                                                                                                  \
        {                                                                                                              \
            std::cerr << "Cuda failure: " << ret << std::endl;                                                         \
            abort();                                                                                                   \
        }                                                                                                              \
    } while (0)


enum class Precision {
	FP32,
	FP16,
	INT8,
};

struct Options {

	Precision precision = Precision::FP16;

	std::string calibrationDataDirectoryPath;

	int32_t calibrationBatchSize = 128;

	int32_t dlaCore = -1;

	int32_t optBatchSize = 1;

	int32_t maxBatchSize = 32;

	int deviceIndex = 0;
};

class Logger : public nvinfer1::ILogger {
	void log(Severity severity, const char* msg) noexcept override;
};

namespace Util {

	inline bool CheckFileExist(const std::string& file_path) {
		std::ifstream f(file_path.c_str());
		return f.good();
	}

	inline void CheckCUDAError(cudaError_t code) {
		if (code != cudaSuccess) {
			std::string errMsg = "CUDA operation failed with code: " + std::to_string(code) + "(" + cudaGetErrorName(code) + "), with message: " + cudaGetErrorString(code);
			std::cout << errMsg << std::endl;
			throw std::runtime_error(errMsg);
		}
	}
}

#endif // COMMMON_H



