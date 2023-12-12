#include "../postprocessing_register.hpp"
#include <vector>

class pp_pedestrian_cut6 : public Postprocessing
{
    static TensorSptr sp_concat_tensor(TensorSptr& A, TensorSptr& B) {
        auto A_shape = A->data_shape();
        auto B_shape = B->data_shape();
        CHECK_EQ(A_shape[2], B_shape[2]);
        CHECK_EQ(A_shape[3], B_shape[3]);
        CHECK_EQ(A_shape[2], A_shape[3]);
        CHECK_GT(A_shape[1], 1);
        CHECK_EQ(B_shape[1], 1);
        int sideLength = A_shape[2];

        auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{1, A_shape[1] + B_shape[1], sideLength, sideLength}, -1, memory::NCHW);
        CHECK_EQ(top->count(), A->count() + B->count());

        std::copy(A->mutable_cpu_data(), A->mutable_cpu_data() + A->count(), top->mutable_cpu_data());
        std::copy(B->mutable_cpu_data(), B->mutable_cpu_data() + B->count(), top->mutable_cpu_data() + A->count());

        return top;
    }

    static std::shared_ptr<glasssix::memory::tensor<float>> tensor_transpose_0132(const std::shared_ptr<glasssix::memory::tensor<float>>& bottom) {
        int num = bottom->num();
        int channels = bottom->channels();
        int height = bottom->height();
        int width = bottom->width();
        //CHECK_EQ(bottom->channels(), D * C);
        auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{num, channels, width, height}, -1, memory::NCHW);

        int W_step = width; //8400
        int countb = bottom->count();

        for (int nc = 0; nc < num; nc++) {
            const float* bottom_ptr = bottom->cpu_data() + countb * nc; // bottom_ptr -> D * HW
            float* top_ptr = top->mutable_cpu_data() + countb * nc; // top_ptr -> HW * D

            for (int i = 0; i < W_step; i++) { //for 8400
                for (int line = 0; line < height; line++) { //for 6
                    top_ptr[i * height + line] = bottom_ptr[line * W_step + i];
                }
            }
        }
        return top;
    }

    static TensorSptr yolov8_complement(std::vector<TensorSptr>& vec_ts_rstSort)
    {
        static constexpr int blockSide[3] = { 160, 80, 40 }; //ScaleSteps[3][2] = { {80, 80}, {40, 40}, {20, 20} };
        //CHECK_EQ(3, vec_ts_rstSort.size());

        const int INTEGRATED_ONNX_OUT_UINTLINE_NUM_ = 5;
        int INTEGRATED_ONNX_OUT_LINES = 0;
        for (auto v : blockSide) {
            INTEGRATED_ONNX_OUT_LINES += v * v;
        }


        auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{1, 1, INTEGRATED_ONNX_OUT_LINES, INTEGRATED_ONNX_OUT_UINTLINE_NUM_}, -1, memory::NCHW);
        float* top_data = top->mutable_cpu_data();
        size_t top_line_counter = 0;

        for (int i = 0; i < 3; i++) {

            auto& Scaleblock = vec_ts_rstSort[i];
            Scaleblock->reshape(std::vector<int>{1, 1, 65, blockSide[i] * blockSide[i]});
            Scaleblock = tensor_transpose_0132(Scaleblock); // 1, 1, 65, 6400 -> 1, 1, 6400, 65

            int line_num = Scaleblock->data_shape()[2]; // 6400 + 1600 + 400 = 8400
            int per_line_length = Scaleblock->data_shape()[3]; // 65
            for (int line = 0; line < line_num; line++) // loop 6400 |
            {
                float* uintInfoLinePtrData = Scaleblock->mutable_cpu_data() + line * per_line_length;
                // {65 = 64 + 2}, {64 = 16 * 4}, {16 * 4 conv 16 -> 4}, 4 means raw location

                float raw_location[4] = { 0.f,0.f,0.f,0.f };
                float sotfmax_total[4] = { 0.f,0.f,0.f,0.f };

                // softmax
                for (int exp_i = 0; exp_i < 64; exp_i++) // per conv_group kernel = {0,1,2,3,...,15}, len 16
                {
                    uintInfoLinePtrData[exp_i] = exp(uintInfoLinePtrData[exp_i]);
                    sotfmax_total[exp_i / 16] += uintInfoLinePtrData[exp_i];
                }

                // convolution with after-softmax
                for (int exp_i = 0; exp_i < 64; exp_i++) // per conv_group kernel = {0,1,2,3,...,15}, len 16
                {
                    // exp_i / 16 : loop div 0...0, 1..1, 2..2, 3..3 per length 16
                    // exp_i % 16 : loop mul 0,1,2..15,  0,1,2..15,  0,1,2..15,  0,1,2..15
                    uintInfoLinePtrData[exp_i] /= sotfmax_total[exp_i / 16]; // after-softmax operation
                    raw_location[exp_i / 16] += uintInfoLinePtrData[exp_i] * (exp_i % 16);// convolution
                }
                // <score>
                // score[0], score[1] = [64+0], [64+1]...
                uintInfoLinePtrData[64] = sigmoid_x(uintInfoLinePtrData[64]);
                // </score>

                raw_location[0] = 0.5 + line % blockSide[i] - raw_location[0];
                raw_location[1] = 0.5 + line / blockSide[i] - raw_location[1];

                raw_location[2] = 0.5 + line % blockSide[i] + raw_location[2];
                raw_location[3] = 0.5 + line / blockSide[i] + raw_location[3];

                float loaction_0 = (raw_location[2] + raw_location[0]) / 2;
                float loaction_1 = (raw_location[3] + raw_location[1]) / 2;

                float loaction_2 = raw_location[2] - raw_location[0];
                float loaction_3 = raw_location[3] - raw_location[1];

                loaction_0 = loaction_0 / blockSide[i]; // Equivalent operation for * mul_{8,16,32} / div_{360} 
                loaction_1 = loaction_1 / blockSide[i];
                loaction_2 = loaction_2 / blockSide[i];
                loaction_3 = loaction_3 / blockSide[i];

                auto top_line_data = top_data + top_line_counter * INTEGRATED_ONNX_OUT_UINTLINE_NUM_;
                top_line_data[0] = loaction_0;
                top_line_data[1] = loaction_1;
                top_line_data[2] = loaction_2;
                top_line_data[3] = loaction_3;
                top_line_data[4] = uintInfoLinePtrData[64];
                top_line_counter++;
            }
        }
        return top;
    }

	static std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> pedestrian_concat_score(std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>>& input_tensor_map)
	{
        auto det_rst_vec = sort_yolo_rst(input_tensor_map);
        if (det_rst_vec.size() == 6) {
            det_rst_vec[0] = sp_concat_tensor(det_rst_vec[0], det_rst_vec[3]);
            det_rst_vec[1] = sp_concat_tensor(det_rst_vec[1], det_rst_vec[4]);
            det_rst_vec[2] = sp_concat_tensor(det_rst_vec[2], det_rst_vec[5]);
        }
        TensorSptr concat_tensor_ptr = yolov8_complement(det_rst_vec);
        auto concat_tensor_data = concat_tensor_ptr->cpu_data();

        //score_array
        std::shared_ptr<glasssix::memory::tensor<float>> output0(new memory::tensor<float>(std::vector<int>{1, 1, 1, 33600}, -1, memory::NCHW));
        float* output = output0->mutable_cpu_data();
        for (int i = 0; i < 33600; i++)
        {
            output[i] = concat_tensor_data[i*5+4];
        }

        std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> postprocessing_rstmap;
        postprocessing_rstmap.try_emplace("pp_pedestrian_cut6_score_out", output0); // complement result 
        return postprocessing_rstmap;
	}


public:
	virtual const std::map<std::string, postprocessing_function> parser_postprocessing_dump() const override
	{
		std::map<std::string, postprocessing_function> pp_map;
		pp_map["pedestrian_cut6"] = &pedestrian_concat_score;
		return pp_map;
	}
};

REGISTE_POSTPROCESSING(pp_pedestrian_cut6)