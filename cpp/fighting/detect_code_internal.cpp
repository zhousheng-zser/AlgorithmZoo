#include "detect_code_internal.hpp"

#include <algorithm>
#include <numeric>

#include <Excalibur/pipeline.hpp>
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include "Primitives/tensor_conversions.hpp"
#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_resize.hpp"
#include "Excalibur/operation_rgb2gray.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#ifdef USE_RKNNAPI
//#if 0
#include "../../common/include/RKNNWrapper/rknn_wrapper.hpp"
#elif defined(USE_RKNN2API)
#include "../../common/include/RKNN2Wrapper/rknn2_wrapper.hpp"
#endif

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

//#ifdef BUILD_DEBUG_INFO
//#include "dbg.h"
//#include <opencv2/highgui.hpp>
//#include "numpyExtensor.hpp"
//
//#endif // BUILD_DEBUG_INFO

namespace glasssix::fighting
{
    class detect_code_internal::impl
    {
    public:
        impl(std::string_view model_directory, int device, int batch) :BATCH_(batch)
        {
            std::vector<std::string> empty_hold;
            if (BATCH_ == 12)
                instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(empty_hold, std::string(model_directory) + "/" + "fight_12b" + ".rknn", device);
            else if (BATCH_ == 8)
                instance_ = std::make_unique<rknnwrapper::rknn_wrapper>(empty_hold, std::string(model_directory) + "/" + "fight_8b" + ".rknn", device);
            else
                throw exposing::abi_invalid_argument("incorrect BATCH_ param");
        }

        float detect(exposing::param_span<std::uint8_t> bitmap, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map_std)
        {
            auto detect_start = std::chrono::system_clock::now();

            size_t bitmap_size = bitmap.size();
            CHECK_EQ(bitmap.size(), BATCH_ * height * width * 3);
            // split bitmap to img vector
            std::vector<cv::Mat> BatchImgs;
            for (int i = 0; i < BATCH_; i++) {
                cv::Mat InteImage(cv::Size(width, height), CV_8UC3);
                std::memcpy(InteImage.data, bitmap.data() + height * width * 3 * i, sizeof(uint8_t) * height * width * 3);
                BatchImgs.push_back(InteImage);
            }
            CHECK_EQ(BatchImgs.size(), BATCH_);

            // three crop img and sort out
            std::vector<cv::Mat> headCrops;
            std::vector<cv::Mat> mediCrops;
            std::vector<cv::Mat> tailCrops;
            for (auto& integImg : BatchImgs) {
                auto [c_head, c_medi, c_tail] = threecrop(integImg, CROP_SIZE_);
                headCrops.push_back(c_head);
                mediCrops.push_back(c_medi);
                tailCrops.push_back(c_tail);
            }


#if defined(USE_RKNNAPI) || defined(USE_RKNN2API)
            auto inpuTensor = make_rknn_input(headCrops, mediCrops, tailCrops, std::vector<int>{1, BATCH_ * 3 * 3, CROP_SIZE_, CROP_SIZE_});
            auto det_rst_map = instance_->forward(inpuTensor->mutable_cpu_data(), { 1, CROP_SIZE_, CROP_SIZE_, 9 * BATCH_ }, RKNN_TENSOR_NHWC);
#else
            auto inpuTensor = make_onnx_input(headCrops, mediCrops, tailCrops, std::vector<int>{1, BATCH_ * 3 * 3, CROP_SIZE_, CROP_SIZE_});
            auto det_rst_map = instance_->forward(inpuTensor);
#endif

            auto det_scores = det_rst_map.begin()->second->cpu_data();

            return det_scores[0];
        }

        template<typename T>
        std::shared_ptr<glasssix::memory::tensor<T>> tensor_transpose_0132(const std::shared_ptr<glasssix::memory::tensor<T>>& bottom) {
            int num = bottom->num();
            int channels = bottom->channels();
            int height = bottom->height();
            int width = bottom->width();
            auto top = std::make_shared<glasssix::memory::tensor<T>>(std::vector<int>{num, channels, width, height}, -1, memory::NCHW);

            int W_step = width; //8400
            int countb = bottom->count();

            for (int nc = 0; nc < num; nc++) {
                const T* bottom_ptr = bottom->cpu_data() + countb * nc; // bottom_ptr -> D * HW
                T* top_ptr = top->mutable_cpu_data() + countb * nc; // top_ptr -> HW * D

                for (int i = 0; i < W_step; i++) { //for 8400
                    for (int line = 0; line < height; line++) { //for 6
                        top_ptr[i * height + line] = bottom_ptr[line * W_step + i];
                    }
                }
            }
            return top;
        }

        std::shared_ptr<glasssix::memory::tensor<uint8_t>> make_rknn_input(
            std::vector<cv::Mat>& headCrops,
            std::vector<cv::Mat>& mediCrops,
			std::vector<cv::Mat>& tailCrops,
            std::vector<int> input_shape)
        {
            int inpuTensorCopyFlag = 0;
            auto inpuTensor = std::make_shared<glasssix::memory::tensor<uint8_t>>(input_shape, -1, memory::NCHW);
            //input_shape = {1, BATCH_ * 3 * 3, CROP_SIZE_, CROP_SIZE_}

            auto CropsPushTensor = [&inpuTensor, &inpuTensorCopyFlag, this](std::vector<cv::Mat>& Crops) {
                for (auto& crop : Crops) {
                    cv::cvtColor(crop, crop, cv::COLOR_BGR2RGB);
                    auto crop_tensor = matConverTensor<uint8_t>(crop); // 3*8*DS*3 -> 3*8*3*DS
                    int count = crop_tensor->count();

                    auto crop_tensor_data = crop_tensor->mutable_cpu_data();
                    std::copy(crop_tensor_data, crop_tensor_data + count, inpuTensor->mutable_cpu_data() + inpuTensorCopyFlag);
                    inpuTensorCopyFlag += count;
                }
            };

            CropsPushTensor(headCrops);
            CropsPushTensor(tailCrops);
            CropsPushTensor(mediCrops);
            CHECK_EQ(inpuTensorCopyFlag, inpuTensor->count()); // 3*8*3*DS

            inpuTensor->reshape(std::vector{ 1, 1, BATCH_ * 9, 256 * 256 });
            auto inpuTensor_trans = tensor_transpose_0132(inpuTensor); //
            inpuTensor_trans->reshape(std::vector{ 1, 256, 256, BATCH_ * 9 });
            return inpuTensor_trans;
        }

        std::shared_ptr<glasssix::memory::tensor<float>> make_onnx_input(
            std::vector<cv::Mat>& headCrops,
            std::vector<cv::Mat>& mediCrops,
            std::vector<cv::Mat>& tailCrops,
            std::vector<int> input_shape)
        {
            int inpuTensorCopyFlag = 0;
            auto inpuTensor = std::make_shared<glasssix::memory::tensor<float>>(input_shape, -1, memory::NCHW);

            auto CropsPushTensor = [&inpuTensor, &inpuTensorCopyFlag, this](std::vector<cv::Mat>& Crops) {
                for (auto& crop : Crops) {
                    cv::cvtColor(crop, crop, cv::COLOR_BGR2RGB);

                    auto crop_tensor = matConverTensor<float>(crop);

                    for (int i = 0; i < crop_tensor->count(); i++) {
                        crop_tensor->mutable_cpu_data()[i] = crop_tensor->mutable_cpu_data()[i] / 255;
                    }
                    int count = crop_tensor->count();
                    std::memcpy(inpuTensor->mutable_cpu_data() + inpuTensorCopyFlag, crop_tensor->cpu_data(), sizeof(float) * count);
                    inpuTensorCopyFlag += count;
                }
            };

            CropsPushTensor(headCrops);
            CropsPushTensor(tailCrops);
            CropsPushTensor(mediCrops);
            CHECK_EQ(inpuTensorCopyFlag, inpuTensor->count());
            return inpuTensor;
        }

        std::string version()
        {
            const std::string algo_module_version = "1.0.0";
            std::string nn_frame_version = instance_->version();
            return fmt::format(R"({ {"nn_frame_version":"{}", "algo_module_version" : "{}"} })", nn_frame_version, algo_module_version);
        }


        std::array<cv::Mat, 3> threecrop(cv::Mat InteImage, int size = 256) {
            std::array<cv::Mat, 3> rst;
            int H = InteImage.rows;
            int W = InteImage.cols;
            if (H == W) {
                cv::resize(InteImage, InteImage, { 256,256 });
                rst[0] = InteImage;
                rst[1] = InteImage;
                rst[2] = InteImage;
            }
            else {
                float reszie_ratio = size * 1.f / std::min(H, W);
                bool if_horizon = W >= H;
                CHECK_EQ(if_horizon, true); // lazy 2 write vertical image, meet call me -_-
                if (if_horizon) {
                    int new_H = size;
                    int new_W = W * reszie_ratio;
                    cv::resize(InteImage, InteImage, { new_W,new_H }); //cv::Size{W,H}

                    CHECK_GT(new_W, new_H);
                    rst[0] = safty_cut(InteImage, cv::Rect(0, 0, 256, 256));
                    rst[1] = safty_cut(InteImage, cv::Rect((new_W - size) / 2, 0, 256, 256));
                    rst[2] = safty_cut(InteImage, cv::Rect(new_W - size, 0, 256, 256));
                }
            }

            return rst;
        }

        template<typename T>
        std::shared_ptr<glasssix::memory::tensor<T>> matConverTensor(cv::Mat input_image) {
            std::shared_ptr<glasssix::memory::tensor<uint8_t>> input_tensor_u8(new glasssix::memory::tensor<uint8_t>(std::vector<int>{1, input_image.rows, input_image.cols, 3}, -1, glasssix::memory::NHWC));
            std::copy(input_image.data, input_image.data + input_image.step[0] * input_image.rows, input_tensor_u8->mutable_cpu_data());
            input_tensor_u8->convert_order();
            if constexpr (std::is_same_v<T, uint8_t>)
                return input_tensor_u8;
            else
                return input_tensor_u8 | glasssix::memory::tensor_convert_to<T>;            
        }

        cv::Mat safty_cut(cv::Mat& img, cv::Rect roi)
        {
            int width = roi.width;
            int height = roi.height;
            int x = roi.x;
            int y = roi.y;

            cv::Mat mat(height, width, img.type(), cv::Scalar(0));
            int _x = x;
            int _y = y;
            int _width = width;
            int _height = height;
            if (x < 0)
            {
                _x = 0;
                _width = width + x;
            }

            if (_x + _width > img.cols)
                _width = img.cols - _x;

            if (y < 0)
            {
                _y = 0;
                _height = height + y;
            }

            if (_y + _height > img.rows)
                _height = img.rows - _y;

            img(cv::Rect(_x, _y, _width, _height)).copyTo(mat(cv::Rect(_x - x, _y - y, _width, _height)));
            return mat;
        }

    private:
        std::unique_ptr<rknnwrapper::rknn_wrapper> instance_;
        int BATCH_;
        const int CROP_SIZE_ = 256;
    };

    detect_code_internal::detect_code_internal(std::string_view model_directory, int device, int BATCH_)
        : impl_{ std::make_unique<impl>(model_directory, device, BATCH_) }
    {
    }

    detect_code_internal::~detect_code_internal()
    {
    }

    float detect_code_internal::detect(exposing::param_span<std::uint8_t> bitmap, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, std::map<std::string, float>& param_map_std)
    {
        return impl_->detect(bitmap, height, width, roi_x, roi_y, roi_width, roi_height, param_map_std);
    }

    std::string detect_code_internal::version()
    {
        return impl_->version();
    }
}
