#include "../postprocessing_register.hpp"
#include <vector>

class pp_pedestrian : public Postprocessing
{
	static std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>> pedestrian_concat_score(std::unordered_map<std::string, std::shared_ptr<memory::tensor<float>>>& input_tensor_map)
	{
		std::vector<std::shared_ptr<memory::tensor<float>>> outs;
		std::vector<std::string> out_names = { "355", "340", "output0" };
		for (size_t i = 0; i < out_names.size(); i++) // 对输出数据做处理
		{
			outs.push_back(input_tensor_map[out_names[i]]);
		}

		std::vector<float> cat(65 * 33600); // 1*65*33600 = 64*33600 + 1*33600
		const float* data80 = outs[2]->cpu_data();
		const float* data40 = outs[1]->cpu_data();
		const float* data20 = outs[0]->cpu_data();

		int Candidate = 33600;
		for (int i = 0; i < 65; i++)
		{
			int j = 0;
			for (; j < 25600; j++)
			{
				cat[i * Candidate + j] = data80[i * 25600 + j];
			}
			for (; j < 32000; j++)
			{
				cat[i * Candidate + j] = data40[i * 6400 + j - 25600];
			}

			for (; j < 33600; j++)
			{
				cat[i * Candidate + j] = data20[i * 1600 + j - 32000];
			}
		}


        // boxes cat[0:64*33600]

        std::vector<float> reshape_box(33600 * 64);
        // tranpose and softmax
        for (int i = 0; i < 64; i++)
        {
            for (int j = 0; j < 33600; j++)
            {
                reshape_box[j * 64 + i] = cat[i * 33600 + j];
            }
        }

        int index = 0;
        for (int i = 0; i < 33600; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                Softmax(reshape_box.data() + 16 * index, 16);
                index++;
            }
        }

        // reshape and tranpose  64*33600 ->33600*64
        std::vector<float> reshape_box2(16 * 4 * 33600);

        std::array<float, 64> temp;

        for (int i = 0; i < 33600; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                for (int k = 0; k < 16; k++)
                {
                    reshape_box2[k * 4 * 33600 + j * 33600 + i] = reshape_box[i * 16 * 4 + j * 16 + k];
                }
            }
        }

        std::vector<float> conv(4 * 33600);

        for (int i = 0; i < 4 * 33600; i++)
        {
            conv[i] = 0.f;
        }

        // 16个通道 1*1卷积
        for (int i = 0; i < 16; i++)
        {
            for (int j = 0; j < 4 * 33600; j++)
            {
                int location = 4 * 33600;
                reshape_box2[i * location + j] = reshape_box2[i * location + j] * i;
                conv[j] = conv[j] + reshape_box2[i * location + j];
            }
        }

        // slice and function operator

        std::vector<float> sub_add(33600 * 2);

        for (int i = 0; i < 25600; i++)
        {
            sub_add[i] = i % 160 - 0.5f + 1.f;
        }
        for (int i = 0; i < 6400; i++)
        {
            sub_add[25600 + i] = i % 80 - 0.5f + 1.f;
        }
        for (int i = 0; i < 1600; i++)
        {
            sub_add[32000 + i] = i % 40 - 0.5f + 1.f;
        }

        for (int i = 0; i < 25600; i++)
        {
            sub_add[33600 + i] = i / 160 - 0.5f + 1.f;
        }
        for (int i = 0; i < 6400; i++)
        {
            sub_add[33600 + 25600 + i] = i / 80 - 0.5f + 1.f;
        }
        for (int i = 0; i < 1600; i++)
        {
            sub_add[33600 + 32000 + i] = i / 40 - 0.5f + 1.f;
        }

        // 2次sub and add   此处应该是xyxy2xywh
        std::vector<float> sub_data(33600 * 2);
        std::vector<float> add_data(33600 * 2);
        for (int i = 0; i < 33600 * 2; i++)
        {
            sub_data[i] = sub_add[i] - conv[i];
            add_data[i] = conv[i + 33600 * 2] + sub_add[i];
        }

        std::vector<float> add2_data(33600 * 2);
        std::vector<float> sub2_data(33600 * 2);

        for (int i = 0; i < 33600 * 2; i++)
        {
            add2_data[i] = sub_data[i] + add_data[i];
            sub2_data[i] = add_data[i] - sub_data[i];
        }

        // div concat
        std::vector<float> concat(33600 * 24);
        for (int i = 0; i < 33600 * 2; i++)
        {
            concat[i] = add2_data[i] / 2.f;
            concat[i + 33600 * 2] = sub2_data[i];
        }

        std::vector<float> MUL(33600);

        for (int i = 0; i < 25600; i++)
        {
            MUL[i] = 8;
            if (i < 6400)
            {
                MUL[i + 25600] = 16;
            }
            if (i < 1600)
            {
                MUL[i + 32000] = 32;
            }
        }

        //std::shared_ptr<glasssix::memory::tensor<float>> output0(new memory::tensor<float>(std::vector<int>{1, 5, 33600}, -1, memory::NCHW));
        //score_array
        std::shared_ptr<glasssix::memory::tensor<float>> output0(new memory::tensor<float>(std::vector<int>{1, 1, 1, 33600}, -1, memory::NCHW));
        float* output = output0->mutable_cpu_data();
        for (int i = 0; i < 33600; i++)
        {
            concat[33600 * 0 + i] = concat[33600 * 0 + i] * MUL[i];
            concat[33600 * 1 + i] = concat[33600 * 1 + i] * MUL[i];
            concat[33600 * 2 + i] = concat[33600 * 2 + i] * MUL[i];
            concat[33600 * 3 + i] = concat[33600 * 3 + i] * MUL[i];

            //output[33600 * 0 + i] = concat[33600 * 0 + i];
            //output[33600 * 1 + i] = concat[33600 * 1 + i];
            //output[33600 * 2 + i] = concat[33600 * 2 + i];
            //output[33600 * 3 + i] = concat[33600 * 3 + i];
            //output[33600 * 4 + i] = sigmoid_x(cat[33600 * 64 + i]);
            output[i] = sigmoid_x(cat[33600 * 64 + i]);

        }

        std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> postprocessing_rstmap;
        postprocessing_rstmap.try_emplace("pp_pedestrian_score_out", output0); // complement result 
        return postprocessing_rstmap;
	}


public:
	virtual const std::map<std::string, postprocessing_function> parser_postprocessing_dump() const override
	{
		std::map<std::string, postprocessing_function> pp_map;
		pp_map["pedestrian"] = &pedestrian_concat_score;
		return pp_map;
	}
};

REGISTE_POSTPROCESSING(pp_pedestrian)