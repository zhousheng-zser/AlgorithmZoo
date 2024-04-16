#ifndef _NUMPYEXTENSOR_HPP_
#define _NUMPYEXTENSOR_HPP_
#include "numpy.hpp"
#include <numeric>
#include <Primitives/tensor.hpp>

namespace npy {

template<typename T_ = float>
static inline std::shared_ptr<glasssix::memory::tensor<float>> LoadNpy(std::string npyFile) {
    std::vector<unsigned long> shape;
    bool fortran_order;
    std::vector<T_> data;

    npy::LoadArrayFromNumpy<T_>(npyFile, shape, fortran_order, data);

    std::array<int, 4> pNCHW{ 1,1,1,1 };
    int count = 1;
    auto shape_riter = shape.rbegin();
    auto NCHW_riter = pNCHW.rbegin();
    while (shape_riter != shape.rend() && NCHW_riter != pNCHW.rend()) {
        *NCHW_riter = *shape_riter;
        count *= *shape_riter;
        shape_riter++;
        NCHW_riter++;
    }
    auto top = std::make_shared<glasssix::memory::tensor<T_>>(std::vector<int>{pNCHW[0], pNCHW[1], pNCHW[2], pNCHW[3]}, -1, glasssix::memory::NCHW);
    std::copy(data.data(), data.data() + count, top->mutable_cpu_data());
    return top;
}

static inline void SAVE_TENSOR_TO_NUMPY(const std::shared_ptr<glasssix::memory::tensor<float>>& input_tensor, std::string path = "xclbr_tensor.npy") {
    auto tensor_shape = input_tensor->data_shape();
    std::vector<unsigned long> shape(tensor_shape.begin(), tensor_shape.end());

    const bool fortran_order{ false };
    std::vector<float> out_data(input_tensor->count());
    std::copy(input_tensor->cpu_data(), input_tensor->cpu_data() + input_tensor->count(), out_data.data());
    npy::SaveArrayAsNumpy(path, fortran_order, shape.size(), shape.data(), out_data);
}

static inline void SAVE_ARRAY_TO_NUMPY(float* input_tensor, std::vector<unsigned long> shape,std::string path = "xclbr_tensor.npy") {
    auto count = std::accumulate(std::begin(shape), std::end(shape), 1, std::multiplies<unsigned long>());
    const bool fortran_order{ false };
    std::vector<float> out_data(count);
    std::copy(input_tensor, input_tensor + count, out_data.data());
    npy::SaveArrayAsNumpy(path, fortran_order, shape.size(), shape.data(), out_data);
}

template<typename T>
static inline float CosineSimilarity(T emb1, T emb2, int len)
{
    float dot = 0.f;
    float emb1_sum = 0.f;
    float emb2_sum = 0.f;
    for (size_t i = 0; i < len; i++) {
        //if(i>1000&&i<1020)
        //	std::cout << std::fixed << std::setprecision(2) << emb1[i] << ", " << emb2[i] << " \tdf=" << std::setprecision(3) << emb1[i] - emb2[i] << std::endl;

        dot += emb1[i] * emb2[i];
        emb1_sum += emb1[i] * emb1[i];
        emb2_sum += emb2[i] * emb2[i];
    }

    constexpr float zero_epsilo = 0.000001f; //1e-6

    if (std::abs(dot) < zero_epsilo && std::sqrt(emb1_sum) * std::sqrt(emb2_sum) < zero_epsilo)
    {
        return 1.f; // rst = 0.f / 0,f -> 1
    }
    else
    {
        dot /= std::max(std::sqrt(emb1_sum) * std::sqrt(emb2_sum),
            std::numeric_limits<float>::epsilon());
        return dot;
    }
}


}  // namespace npy

#endif  // _NUMPYEXTENSOR_HPP_
