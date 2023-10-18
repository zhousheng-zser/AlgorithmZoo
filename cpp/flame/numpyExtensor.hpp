#ifndef _NUMPYEXTENSOR_HPP_
#define _NUMPYEXTENSOR_HPP_
#include "numpy.hpp"
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

}  // namespace npy

#endif  // _NUMPYEXTENSOR_HPP_
