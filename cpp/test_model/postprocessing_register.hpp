#pragma once
#ifndef _CONCAT_FUNCTIONS_
#define _CONCAT_FUNCTIONS_

#include<vector>
#include<opencv2/opencv.hpp>
#include <Primitives/tensor.hpp>
using namespace glasssix;


using postprocessing_function = std::function<
	std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>>
		(std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>>&)>;

class Postprocessing {
public:
	using TensorSptr = std::shared_ptr<memory::tensor<float>>;
	virtual const std::map<std::string, postprocessing_function> parser_postprocessing_dump() const = 0;


	static inline float sigmoid_x(float x)
	{
		return static_cast<float>(1.f / (1.f + exp(-x)));
	}

    static inline void Softmax(float* data, int num)
    {

        double L2_Sum = 0.f;
        for (size_t i = 0; i < num; i++)
        {
            data[i] = (exp(data[i]));
            L2_Sum += data[i];
        }
        for (size_t i = 0; i < num; i++)
        {
            data[i] = data[i] / L2_Sum;
        }
    }

	// order=true count_sort{7,6,5,4..}; order=false count_sort{4,5,6..}
	static inline std::vector<TensorSptr> sort_yolo_rst(const std::unordered_map<std::string, TensorSptr>& result, bool order = true) {
		std::vector<TensorSptr> outRst;
		for (auto& out : result) {
			outRst.push_back(out.second);
		}
		std::sort(outRst.begin(), outRst.end(), [&order](const TensorSptr& A, const TensorSptr& B) {
			auto countA = A->count();
			auto countB = B->count();
			return !((countA > countB) ^ order);
			});
		return outRst;
	}

};

class PostprocessingRegister {
public:
	PostprocessingRegister(std::shared_ptr<Postprocessing> pplugin);
};

static std::vector<std::shared_ptr<Postprocessing>>& postprocessing_list_instance();

#define REGISTE_POSTPROCESSING(CLASS_NAME) \
	static PostprocessingRegister postprocessing_##CLASS_NAME##_register(std::shared_ptr<Postprocessing>{new CLASS_NAME});

void AddPostprocessing(std::map<std::string, postprocessing_function>&);


#endif //!_CONCAT_FUNCTIONS_