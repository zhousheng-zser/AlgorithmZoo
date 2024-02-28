#include "pumptop_helmet_detector_impl.hpp"
#include "pumptop_helmet_info_impl.hpp"
#ifdef USE_RKNNAPI
#include "RKNNWrapper/rknn_wrapper.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#elif defined(USE_RKNN2API)
#include "RKNN2Wrapper/rknn2_wrapper.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#else
#include "Excalibur/pipeline.hpp"
#include "Excalibur/operation_safty_cut.hpp"
#include "Excalibur/operation_resize.hpp"
#include "Primitives/tensor_conversions.hpp"
#endif

namespace glasssix::pumptop_helmet
{
	class pumptop_helmet_detector_impl::impl
	{
	public:
		impl() = delete;

		impl(std::string_view model_directory, int device)
			: model_directory_{std::string(model_directory)}, device_{device}
		{
			//算法传过来的模型名:泵检测模型 640-v1_ori_TAL
			net_detect_1 = std::make_unique<rknnwrapper::rknn_wrapper>(phais, std::string(model_directory) + "/" + "pumptop_helmet_pump.rknn", device);

			//算法传过来的模型名:人检测模型 1280T320-0108_Person_best_detection
			net_detect_2 = std::make_unique<rknnwrapper::rknn_wrapper>(phais, std::string(model_directory) + "/" + "pumptop_helmet_person.rknn", device);

			//算法传过来的模型名:人头检测模型 640T320-200epft-baoshinegtivev2-atss-nwd-wop
			net_detect_3 = std::make_unique<rknnwrapper::rknn_wrapper>(phais, std::string(model_directory) + "/" + "pumptop_helmet_head.rknn", device);

			//算法传过来的模型名:人头分类检测模型 helmetclassify-v2-96-labelsmooth-0.05
			net_detect_4 = std::make_unique<rknnwrapper::rknn_wrapper>(phais, std::string(model_directory) + "/" + "pumptop_helmet_helmet.rknn", device);
		}

		~impl()
		{
		}

		// 检测前的预处理以及后处理相关函数
		void init_data640()
		{
			posture_add_weight_1280.resize(8400 * 2);
			posture_mul_weight_1280.resize(8400);
			for (int i = 0; i < 8400; i++)
			{
				if (i < 6400)
				{
					posture_add_weight_1280[i] = i % 80;
					posture_add_weight_1280[i + 8400] = i / 80;
					posture_mul_weight_1280[i] = 8.f;
				}
				else if (i < 8000)
				{
					posture_add_weight_1280[i] = (i - 6400) % 40;
					posture_add_weight_1280[i + 8400] = (i - 6400) / 40;
					posture_mul_weight_1280[i] = 16.f;
				}
				else
				{
					posture_add_weight_1280[i] = (i - 8000) % 20;
					posture_add_weight_1280[i + 8400] = (i - 8000) / 20;
					posture_mul_weight_1280[i] = 32.f;
				}
			}
		}
		void init_data320()
		{
			posture_add_weight_1280.resize(2100 * 2);
			posture_mul_weight_1280.resize(2100);
			for (size_t i = 0; i < 2100; i++)
			{
				if (i < 1600)
				{
					posture_add_weight_1280[i] = i % 40;
					posture_add_weight_1280[i + 2100] = i / 40;
					posture_mul_weight_1280[i] = 8.f;
				}
				else if (i < 2000)
				{
					posture_add_weight_1280[i] = (i - 1600) % 20;
					posture_add_weight_1280[i + 2100] = (i - 1600) / 20;
					posture_mul_weight_1280[i] = 16.f;
				}
				else
				{
					posture_add_weight_1280[i] = (i - 2000) % 10;
					posture_add_weight_1280[i + 2100] = (i - 2000) / 10;
					posture_mul_weight_1280[i] = 32.f;
				}
			}
		}
		void init_data128()
		{
			posture_add_weight_1280.resize(336 * 2);
			posture_mul_weight_1280.resize(336);
			for (size_t i = 0; i < 336; i++)
			{
				if (i < 256)
				{
					posture_add_weight_1280[i] = i % 16;
					posture_add_weight_1280[i + 336] = i / 16;
					posture_mul_weight_1280[i] = 8.f;
				}
				else if (i < 300)
				{
					posture_add_weight_1280[i] = (i - 256) % 8;
					posture_add_weight_1280[i + 336] = (i - 256) / 8;
					posture_mul_weight_1280[i] = 16.f;
				}
				else
				{
					posture_add_weight_1280[i] = (i - 300) % 4;
					posture_add_weight_1280[i + 336] = (i - 300) / 4;
					posture_mul_weight_1280[i] = 32.f;
				}
			}
		}
		std::tuple<cv::Mat, float> preprocess_detection(cv::Mat src, int &pad_h, int &pad_w, cv::Size input_shape = cv::Size(640, 640))
		{
			float scale = std::min((float)input_shape.width / (float)src.cols, (float)input_shape.height / (float)src.rows);
			cv::Mat cut_image;
			cv::Mat mask_image(input_shape, CV_8UC3, cv::Scalar(114, 114, 114));
			if (src.rows != input_shape.height || src.cols != input_shape.width)
			{
				cv::resize(src, cut_image, cv::Size((int)(src.cols * scale), (int)(src.rows * scale)), cv::INTER_LINEAR);

				pad_h = int((input_shape.height - cut_image.rows) / 2);
				pad_w = int((input_shape.width - cut_image.cols) / 2);
				cv::copyMakeBorder(cut_image, mask_image, pad_h, input_shape.height - cut_image.rows - pad_h, pad_w, input_shape.width - cut_image.cols - pad_w, cv::BORDER_CONSTANT, cv::Scalar{114, 114, 114});
			}
			else
			{
				src.copyTo(mask_image);
			}
			cv::cvtColor(mask_image, mask_image, cv::COLOR_BGR2RGB);
			return {mask_image, scale};
		}
		static inline float sigmoid_x(float x)
		{
			return static_cast<float>(1.f / (1.f + exp(-x)));
		}

		void tranpose(const float *sou, float *dest, int sourows, int soucols)
		{
			for (int i = 0; i < sourows; i++)
				for (int j = 0; j < soucols; j++)
					dest[j * sourows + i] = sou[i * soucols + j];
		}

		void Softmax(float *data, int num)
		{
			double L2_Sum = 0.f;
			for (size_t i = 0; i < num; i++)
			{
				data[i] = (exp(data[i]));
				L2_Sum += data[i];
			}
			for (size_t i = 0; i < num; i++)
				data[i] = data[i] / L2_Sum;
		}

		inline float de_sigmoid(float x)
		{
			if (x >= 1 || x < 0)
				return NAN;
			return static_cast<float>(log(x / (1 - x)));
		}
		std::shared_ptr<memory::tensor<float>> Yovo8se_Concat(std::vector<std::shared_ptr<memory::tensor<float>>> &outs, float conf, int &candicate_num)
		{
			conf = de_sigmoid(conf);
			int input = 640;
			int box_tmp_size = 64;
			int stride_8_num = input / 8;
			int stride_16_num = input / 16;
			int stride_32_num = input / 32;

			int candidate_num = stride_8_num * stride_8_num + stride_16_num * stride_16_num + stride_32_num * stride_32_num;
			int totol_size = stride_8_num * stride_8_num + stride_16_num * stride_16_num + stride_32_num * stride_32_num;
			// 20 40 80
			const float *data_stride_8 = outs[2]->cpu_data();
			const float *data_stride_16 = outs[1]->cpu_data();
			const float *data_stride_32 = outs[0]->cpu_data();

			std::vector<int> match_index;

			const float *data_stride_8_conf = data_stride_8 + stride_8_num * stride_8_num * box_tmp_size;
			for (size_t i = 0; i < stride_8_num * stride_8_num; i++)
				if (data_stride_8_conf[i] > conf)
					match_index.push_back(i);
			const float *data_stride_16_conf = data_stride_16 + stride_16_num * stride_16_num * box_tmp_size;
			for (size_t i = 0; i < stride_16_num * stride_16_num; i++)
				if (data_stride_16_conf[i] > conf)
					match_index.push_back(i + stride_8_num * stride_8_num);
			const float *data_stride_32_conf = data_stride_32 + stride_32_num * stride_32_num * box_tmp_size;
			for (size_t i = 0; i < stride_32_num * stride_32_num; i++)
				if (data_stride_32_conf[i] > conf)
					match_index.push_back(i + stride_8_num * stride_8_num + stride_16_num * stride_16_num);

			// concat the 80*40 40*40 20*20
			std::vector<float> cat(65 * candidate_num); // 1*65*candidate_num = 64*candidate_num + 1*candidate_num
			for (int i = 0, j = 0; i < 65; i++, j = 0)
			{
				std::copy(data_stride_8 + i * stride_8_num * stride_8_num, data_stride_8 + (i + 1) * stride_8_num * stride_8_num, cat.data() + i * candidate_num);
				std::copy(data_stride_16 + i * stride_16_num * stride_16_num, data_stride_16 + (i + 1) * stride_16_num * stride_16_num, cat.data() + i * candidate_num + stride_8_num * stride_8_num);
				std::copy(data_stride_32 + i * stride_32_num * stride_32_num, data_stride_32 + (i + 1) * stride_32_num * stride_32_num, cat.data() + i * candidate_num + stride_8_num * stride_8_num + stride_16_num * stride_16_num);
			}

			// tranpose and softmax
			std::vector<float> reshape_box(candidate_num * 64);
			tranpose(cat.data(), reshape_box.data(), 64, candidate_num);

			candidate_num = match_index.size();

			candicate_num = candidate_num;
			std::vector<float> reshape_boxtmp(candidate_num * 64);
			std::shared_ptr<glasssix::memory::tensor<float>> output0(new memory::tensor<float>(std::vector<int>{1, 5, candidate_num}, -1, memory::NCHW));

			for (size_t i = 0; i < match_index.size(); i++)
				std::copy(reshape_box.data() + match_index[i] * 64, reshape_box.data() + match_index[i] * 64 + 64, reshape_boxtmp.data() + i * 64);

			int index = 0;
			for (int i = 0; i < candidate_num; i++)
				for (int j = 0; j < 4; j++)
					Softmax(reshape_boxtmp.data() + 16 * index++, 16); // inplace softamax

			for (int i = 0; i < candidate_num; i++)
				for (int j = 0; j < 4; j++)
					for (int k = 0; k < 16; k++)
						cat[k * 4 * candidate_num + j * candidate_num + i] = reshape_boxtmp[i * 16 * 4 + j * 16 + k];

			// 16 channels 1*1convolution
			std::vector<float> conv(4 * candidate_num, 0);
			for (int i = 0; i < 16; i++)
				for (int j = 0; j < 4 * candidate_num; j++)
					conv[j] = conv[j] + cat[i * 4 * candidate_num + j] * i;

			std::vector<float> concat(candidate_num * 4);
			for (int i = 0; i < candidate_num * 2; i++)
			{
				concat[i] = (conv[i + candidate_num * 2] - conv[i]) / 2.f + posture_add_weight_1280[i < candidate_num ? match_index[i] : (match_index[i - candidate_num] + totol_size)] + 0.5;
				concat[i + candidate_num * 2] = (conv[i + candidate_num * 2] + conv[i]); // add_data[i]-sub_data[i]) ;
			}

			// concat the output
			float *output = output0->mutable_cpu_data();
			for (int i = 0; i < candidate_num; i++)
			{
				output[candidate_num * 0 + i] = concat[candidate_num * 0 + i] * posture_mul_weight_1280[match_index[i]];
				output[candidate_num * 1 + i] = concat[candidate_num * 1 + i] * posture_mul_weight_1280[match_index[i]];
				output[candidate_num * 2 + i] = concat[candidate_num * 2 + i] * posture_mul_weight_1280[match_index[i]];
				output[candidate_num * 3 + i] = concat[candidate_num * 3 + i] * posture_mul_weight_1280[match_index[i]];
				output[candidate_num * 4 + i] = sigmoid_x(cat[totol_size * 64 + match_index[i]]);
			}
			return output0;
		}

		std::shared_ptr<memory::tensor<float>> Yovo8se_Concat_320(std::vector<std::shared_ptr<memory::tensor<float>>> &outs, float conf, int &candicate_num)
		{
			conf = de_sigmoid(conf);
			int input = 320;
			int box_tmp_size = 64;
			int stride_8_num = input / 8;
			int stride_16_num = input / 16;
			int stride_32_num = input / 32;

			int candidate_num = stride_8_num * stride_8_num + stride_16_num * stride_16_num + stride_32_num * stride_32_num;
			int totol_size = candidate_num;
			// 10 20 40
			const float *data_stride_8 = outs[2]->cpu_data();
			const float *data_stride_16 = outs[1]->cpu_data();
			const float *data_stride_32 = outs[0]->cpu_data();

			std::vector<int> match_index;

			const float *data_stride_8_conf = data_stride_8 + stride_8_num * stride_8_num * box_tmp_size;
			for (size_t i = 0; i < stride_8_num * stride_8_num; i++)
				if (data_stride_8_conf[i] > conf)
					match_index.push_back(i);
			const float *data_stride_16_conf = data_stride_16 + stride_16_num * stride_16_num * box_tmp_size;
			for (size_t i = 0; i < stride_16_num * stride_16_num; i++)
				if (data_stride_16_conf[i] > conf)
					match_index.push_back(i + stride_8_num * stride_8_num);
			const float *data_stride_32_conf = data_stride_32 + stride_32_num * stride_32_num * box_tmp_size;
			for (size_t i = 0; i < stride_32_num * stride_32_num; i++)
				if (data_stride_32_conf[i] > conf)
					match_index.push_back(i + stride_8_num * stride_8_num + stride_16_num * stride_16_num);

			// concat the 80*80 40*40 20*20
			std::vector<float> cat(65 * candidate_num); // 1*65*candidate_num = 64*candidate_num + 1*candidate_num
			for (int i = 0, j = 0; i < 65; i++, j = 0)
			{
				std::copy(data_stride_8 + i * stride_8_num * stride_8_num, data_stride_8 + (i + 1) * stride_8_num * stride_8_num, cat.data() + i * candidate_num);
				std::copy(data_stride_16 + i * stride_16_num * stride_16_num, data_stride_16 + (i + 1) * stride_16_num * stride_16_num, cat.data() + i * candidate_num + stride_8_num * stride_8_num);
				std::copy(data_stride_32 + i * stride_32_num * stride_32_num, data_stride_32 + (i + 1) * stride_32_num * stride_32_num, cat.data() + i * candidate_num + stride_8_num * stride_8_num + stride_16_num * stride_16_num);
			}

			// tranpose and softmax
			std::vector<float> reshape_box(candidate_num * 64);
			tranpose(cat.data(), reshape_box.data(), 64, candidate_num);

			candidate_num = match_index.size();

			candicate_num = candidate_num;
			std::vector<float> reshape_boxtmp(candidate_num * 64);
			std::shared_ptr<glasssix::memory::tensor<float>> output0(new memory::tensor<float>(std::vector<int>{1, 5, candidate_num}, -1, memory::NCHW));

			for (size_t i = 0; i < match_index.size(); i++)
				std::copy(reshape_box.data() + match_index[i] * 64, reshape_box.data() + match_index[i] * 64 + 64, reshape_boxtmp.data() + i * 64);

			int index = 0;
			for (int i = 0; i < candidate_num; i++)
				for (int j = 0; j < 4; j++)
					Softmax(reshape_boxtmp.data() + 16 * index++, 16); // inplace softamax

			for (int i = 0; i < candidate_num; i++)
				for (int j = 0; j < 4; j++)
					for (int k = 0; k < 16; k++)
						cat[k * 4 * candidate_num + j * candidate_num + i] = reshape_boxtmp[i * 16 * 4 + j * 16 + k];

			// 16 channels 1*1convolution
			std::vector<float> conv(4 * candidate_num, 0);
			for (int i = 0; i < 16; i++)
				for (int j = 0; j < 4 * candidate_num; j++)
					conv[j] = conv[j] + cat[i * 4 * candidate_num + j] * i;

			std::vector<float> concat(candidate_num * 4);
			for (int i = 0; i < candidate_num * 2; i++)
			{
				concat[i] = (conv[i + candidate_num * 2] - conv[i]) / 2.f + posture_add_weight_1280[i < candidate_num ? match_index[i] : (match_index[i - candidate_num] + totol_size)] + 0.5;
				concat[i + candidate_num * 2] = (conv[i + candidate_num * 2] + conv[i]); // add_data[i]-sub_data[i]) ;
			}

			// concat the output
			float *output = output0->mutable_cpu_data();
			for (int i = 0; i < candidate_num; i++)
			{
				output[candidate_num * 0 + i] = concat[candidate_num * 0 + i] * posture_mul_weight_1280[match_index[i]];
				output[candidate_num * 1 + i] = concat[candidate_num * 1 + i] * posture_mul_weight_1280[match_index[i]];
				output[candidate_num * 2 + i] = concat[candidate_num * 2 + i] * posture_mul_weight_1280[match_index[i]];
				output[candidate_num * 3 + i] = concat[candidate_num * 3 + i] * posture_mul_weight_1280[match_index[i]];
				output[candidate_num * 4 + i] = sigmoid_x(cat[totol_size * 64 + match_index[i]]);
			}
			return output0;
		}

		std::shared_ptr<memory::tensor<float>> Yovo8se_Concat_128(std::vector<std::shared_ptr<memory::tensor<float>>> &outs, float conf, int &candicate_num)
		{
			conf = de_sigmoid(conf);
			int input = 128;
			int box_tmp_size = 64;
			int stride_8_num = input / 8;
			int stride_16_num = input / 16;
			int stride_32_num = input / 32;

			int candidate_num = stride_8_num * stride_8_num + stride_16_num * stride_16_num + stride_32_num * stride_32_num;
			int totol_size = candidate_num;
			// 10 20 40
			const float *data_stride_8 = outs[2]->cpu_data();
			const float *data_stride_16 = outs[1]->cpu_data();
			const float *data_stride_32 = outs[0]->cpu_data();

			std::vector<int> match_index;

			const float *data_stride_8_conf = data_stride_8 + stride_8_num * stride_8_num * box_tmp_size;
			for (size_t i = 0; i < stride_8_num * stride_8_num; i++)
				if (data_stride_8_conf[i] > conf)
					match_index.push_back(i);
			const float *data_stride_16_conf = data_stride_16 + stride_16_num * stride_16_num * box_tmp_size;
			for (size_t i = 0; i < stride_16_num * stride_16_num; i++)
				if (data_stride_16_conf[i] > conf)
					match_index.push_back(i + stride_8_num * stride_8_num);
			const float *data_stride_32_conf = data_stride_32 + stride_32_num * stride_32_num * box_tmp_size;
			for (size_t i = 0; i < stride_32_num * stride_32_num; i++)
				if (data_stride_32_conf[i] > conf)
					match_index.push_back(i + stride_8_num * stride_8_num + stride_16_num * stride_16_num);

			// concat the 80*80 40*40 20*20
			std::vector<float> cat(65 * candidate_num); // 1*65*candidate_num = 64*candidate_num + 1*candidate_num
			for (int i = 0, j = 0; i < 65; i++, j = 0)
			{
				std::copy(data_stride_8 + i * stride_8_num * stride_8_num, data_stride_8 + (i + 1) * stride_8_num * stride_8_num, cat.data() + i * candidate_num);
				std::copy(data_stride_16 + i * stride_16_num * stride_16_num, data_stride_16 + (i + 1) * stride_16_num * stride_16_num, cat.data() + i * candidate_num + stride_8_num * stride_8_num);
				std::copy(data_stride_32 + i * stride_32_num * stride_32_num, data_stride_32 + (i + 1) * stride_32_num * stride_32_num, cat.data() + i * candidate_num + stride_8_num * stride_8_num + stride_16_num * stride_16_num);
			}

			// tranpose and softmax
			std::vector<float> reshape_box(candidate_num * 64);
			tranpose(cat.data(), reshape_box.data(), 64, candidate_num);

			candidate_num = match_index.size();

			candicate_num = candidate_num;
			std::vector<float> reshape_boxtmp(candidate_num * 64);
			std::shared_ptr<glasssix::memory::tensor<float>> output0(new memory::tensor<float>(std::vector<int>{1, 5, candidate_num}, -1, memory::NCHW));

			for (size_t i = 0; i < match_index.size(); i++)
				std::copy(reshape_box.data() + match_index[i] * 64, reshape_box.data() + match_index[i] * 64 + 64, reshape_boxtmp.data() + i * 64);

			int index = 0;
			for (int i = 0; i < candidate_num; i++)
				for (int j = 0; j < 4; j++)
					Softmax(reshape_boxtmp.data() + 16 * index++, 16); // inplace softamax

			for (int i = 0; i < candidate_num; i++)
				for (int j = 0; j < 4; j++)
					for (int k = 0; k < 16; k++)
						cat[k * 4 * candidate_num + j * candidate_num + i] = reshape_boxtmp[i * 16 * 4 + j * 16 + k];

			// 16 channels 1*1convolution
			std::vector<float> conv(4 * candidate_num, 0);
			for (int i = 0; i < 16; i++)
				for (int j = 0; j < 4 * candidate_num; j++)
					conv[j] = conv[j] + cat[i * 4 * candidate_num + j] * i;

			std::vector<float> concat(candidate_num * 4);
			for (int i = 0; i < candidate_num * 2; i++)
			{
				concat[i] = (conv[i + candidate_num * 2] - conv[i]) / 2.f + posture_add_weight_1280[i < candidate_num ? match_index[i] : (match_index[i - candidate_num] + totol_size)] + 0.5;
				concat[i + candidate_num * 2] = (conv[i + candidate_num * 2] + conv[i]); // add_data[i]-sub_data[i]) ;
			}

			// concat the output
			float *output = output0->mutable_cpu_data();
			for (int i = 0; i < candidate_num; i++)
			{
				output[candidate_num * 0 + i] = concat[candidate_num * 0 + i] * posture_mul_weight_1280[match_index[i]];
				output[candidate_num * 1 + i] = concat[candidate_num * 1 + i] * posture_mul_weight_1280[match_index[i]];
				output[candidate_num * 2 + i] = concat[candidate_num * 2 + i] * posture_mul_weight_1280[match_index[i]];
				output[candidate_num * 3 + i] = concat[candidate_num * 3 + i] * posture_mul_weight_1280[match_index[i]];
				output[candidate_num * 4 + i] = sigmoid_x(cat[totol_size * 64 + match_index[i]]);
			}
			return output0;
		}

		std::vector<std::vector<float>> post_process(std::shared_ptr<glasssix::memory::tensor<float>> &net_result, cv::Mat &blob, int pad_h, int pad_w, float scale, int candicate_num, float threshold = 0.1, float iou_thres = 0.6)
		{
			std::vector<std::vector<float>> output;

			int shape = 5;
			const int candidate_num = candicate_num;
			// const int candidate_num = 34000;
			std::shared_ptr<glasssix::memory::tensor<float>> dest(new glasssix::memory::tensor<float>(candidate_num, shape, -1, glasssix::memory::NCHW, nullptr));

			tranpose(net_result->cpu_data(), dest->mutable_cpu_data(), shape, candidate_num);
			const float *dest_ptr = dest->cpu_data();

			std::vector<float> scores;
			std::vector<int> indices_body; // 候选框顺序
			std::vector<cv::Rect2d> xywh_boxes;
			std::vector<std::vector<float>> key_points;

			for (int i = 0; i < candidate_num; i++)
			{
				// if(dest_ptr[shape*i+4]>threshold)
				{
					indices_body.push_back(i);
					cv::Rect2d boxwh;
					boxwh.x = static_cast<double>(dest_ptr[shape * i] - dest_ptr[shape * i + 2] / 2);
					boxwh.y = static_cast<double>(dest_ptr[shape * i + 1] - dest_ptr[shape * i + 3] / 2);
					boxwh.width = static_cast<double>(dest_ptr[shape * i + 2]);
					boxwh.height = static_cast<double>(dest_ptr[shape * i + 3]);
					{
						xywh_boxes.push_back(boxwh);
						scores.push_back(dest_ptr[shape * i + 4]);
						// indices_body.push_back(i);
					}
				}
			}

			std::vector<int> indices_body_copy(indices_body.size());
			for (int i = 0; i < indices_body_copy.size(); i++)
			{
				indices_body_copy[i] = i;
			}
			cv::dnn::NMSBoxes(xywh_boxes, scores, threshold, iou_thres, indices_body_copy, 1.f, 0);

			for (int i = 0; i < indices_body_copy.size(); i++)
			{
				int index = indices_body_copy[i];
				std::vector<float> temp_output(5);
				temp_output[0] = (xywh_boxes[index].x - pad_w) * scale;
				temp_output[1] = (xywh_boxes[index].y - pad_h) * scale;
				temp_output[2] = (xywh_boxes[index].width + xywh_boxes[index].x - pad_w) * scale;
				temp_output[3] = (xywh_boxes[index].height + xywh_boxes[index].y - pad_h) * scale;
				temp_output[4] = scores[index];
				output.emplace_back(temp_output);
			}
			int k = 0;
			return output;
		}

		exposing::param_vector<pumptop_helmet_info> detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::map<std::string, float> &param_map)
		{
			std::vector<int> categorys;
			std::vector<float> scores;
			std::vector<float> helmet_scores;
			// CHECK_EQ(channels, 24);
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}
			auto results = exposing::make_param_vector<pumptop_helmet::pumptop_helmet_info>();
			cv::Mat image(height, width, CV_8UC3, bitmap.data());
			std::vector<cv::Rect> ori_rect = pump_detect(image, param_map, categorys,scores, helmet_scores);
			for (int i = 0; i < ori_rect.size(); i++)
			{
				int category = -1;
				category = categorys[i];
				if(category != 0)
				{
					continue;
				}
				int x1 = ori_rect[i].x;
				int y1 = ori_rect[i].y;
				int x2 = ori_rect[i].x + ori_rect[i].width;
				int y2 = ori_rect[i].y + ori_rect[i].height;
				float score = scores[i];
				float helmet_score = helmet_scores[i];
				pumptop_helmet::pumptop_helmet_info_internal goal{x1, y1, x2, y2, category, score, helmet_score};
				results.push_back(glasssix::exposing::make_as_first<pumptop_helmet::pumptop_helmet_info_impl>(goal));
			}
			return results;
		}

		//! 开始泵检测,人检测,人头检测,人头分类
		//~ 泵检测
		std::vector<cv::Rect> pump_detect(cv::Mat &image, std::map<std::string, float> &param_map, std::vector<int> &categorys, std::vector<float> &scores, std::vector<float> &helmet_scores)
		{
			cv::Rect ori_rect;
			std::vector<cv::Rect> result_rect;
			// 预处理
			init_data640();
			float con_thres = param_map.count("pump_conf_thres") ? param_map["pump_conf_thres"] : 0.6f;
			float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;
			auto new_shape = cv::Size(640, 640);
			cv::Mat blob;
			float ratio = 1.f;
			int pad_h = 0;
			int pad_w = 0;

			std::tie(blob, ratio) = preprocess_detection(image, pad_h, pad_w, new_shape);
			unsigned char *blobdata = blob.ptr<uchar>();
			std::vector<int> v_blob;
			v_blob.push_back(1);
			v_blob.push_back(blob.rows);
			v_blob.push_back(blob.cols);
			v_blob.push_back(blob.channels());
			auto network_results = net_detect_1->forward(blob.data, v_blob, RKNN_TENSOR_NHWC);
			std::vector<std::string> out_names = {"355", "340", "output0"};

			std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forwards;

			for (size_t i = 0; i < out_names.size(); ++i)
			{
				forwards.push_back(network_results[out_names[i]]);
			}
			int candicate_num = 1;

			auto real_output = Yovo8se_Concat(forwards, con_thres, candicate_num);
			auto nms_result = post_process(real_output, blob, pad_h, pad_w, 1.f / ratio, candicate_num, con_thres, iou_thres);
			// 对泵的结果循环
			for (auto &pump : nms_result)
			{
				int x1 = std::round(pump[0]) > 0 ? std::round(pump[0]) : 0;
				int y1 = std::round(pump[1]) > 0 ? std::round(pump[1]) : 0;
				int x2 = std::round(pump[2]) < image.cols ? std::round(pump[2]) : image.cols;
				int y2 = std::round(pump[3]) < image.rows ? std::round(pump[3]) : image.rows;

				cv::Rect roiRect(x1, y1, x2 - x1, y2 - y1);
				cv::Mat roi = image(roiRect).clone();
				cv::Point p(x1, x2);
				// 人检测
				int category;
				float score;
				float helmet_score;
				ori_rect = people_detect(image, roiRect, param_map, category, score, helmet_score); // roiRect 是泵的坐标
				if(category != -1)//满足目标才放入
				{
					categorys.push_back(category);
					result_rect.push_back(ori_rect);
					scores.push_back(score);
					helmet_scores.push_back(helmet_score);
				}
			}
			return result_rect;
		}

		//~ 人检测,包含了人,人头,人头分类检测
		cv::Rect people_detect(cv::Mat image_ori_all, cv::Rect rect, std::map<std::string, float> &param_map, int &category , float &score, float &helmet_score)
		{
			cv::Rect rect_head;
			cv::Rect rect_peple_ori;
			category = -1;
			float con_thres = param_map.count("people_conf_thres") ? param_map["people_conf_thres"] : 0.6f;
			float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.6f;
			cv::Rect roi(rect.x, rect.y, rect.width, rect.height);
			cv::Mat image = image_ori_all(roi).clone();
			cv::Rect ori_rect;
			cv::Mat img_det;
			cv::Point point_people_feet_center{0, 0};
			cv::Point point_pump_xy_ori = {rect.x, rect.y};
			// 泵顶的区域 根据算法要求，改为 0.4
			float scale = 0.4;
			float X = rect.x + rect.width  * (1 - scale) / 2;
			float Y = rect.y + rect.height * (1 - scale) / 2;

			cv::Rect pump_top_ori(X, Y, rect.width * scale, rect.height * scale);
			init_data320();
			auto new_shape = cv::Size(320, 320);
			cv::Mat blob;
			float ratio = 1.f;
			int pad_h = 0;
			int pad_w = 0;

			std::tie(blob, ratio) = preprocess_detection(image, pad_h, pad_w, new_shape);
			unsigned char *blobdata = blob.ptr<uchar>();
			std::vector<int> v_blob;
			v_blob.push_back(1);
			v_blob.push_back(blob.rows);

			v_blob.push_back(blob.cols);
			v_blob.push_back(blob.channels());
			auto network_results = net_detect_2->forward(blob.data, v_blob, RKNN_TENSOR_NHWC);

			std::vector<std::string> out_names = {"355", "340", "output0"};

			std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forwards;

			for (size_t i = 0; i < out_names.size(); ++i)
			{
				forwards.push_back(network_results[out_names[i]]);
			}

			int candicate_num = 0;

			auto real_output = Yovo8se_Concat_320(forwards, con_thres, candicate_num);

			auto nms_result = post_process(real_output, blob, pad_h, pad_w, 1.f / ratio, candicate_num, con_thres, iou_thres);
			//从结果里面拿取人的坐标
			for (auto &pump : nms_result)
			{
				int x1 = std::round(pump[0]) > 0 ? std::round(pump[0]) : 0;
				int y1 = std::round(pump[1]) > 0 ? std::round(pump[1]) : 0;
				int x2 = std::round(pump[2]) < image.cols ? std::round(pump[2]) : image.cols;
				int y2 = std::round(pump[3]) < image.rows ? std::round(pump[3]) : image.rows;

				cv::Rect roiRect(x1, y1, x2 - x1, y2 - y1);
				cv::Mat roi = image(roiRect).clone();
				// 找到人的原始坐标
				cv::Point people_ori_1 = {x1 + rect.x, y1 + rect.y};
				cv::Point people_ori_2 = {x2 + rect.x, y2 + rect.y};
				cv::Point point_people_feet_center = {(people_ori_2.x - people_ori_1.x) / 2 + people_ori_1.x, people_ori_2.y};
				cv::Rect people_rect(people_ori_1.x, people_ori_1.y, people_ori_2.x - people_ori_1.x, people_ori_2.y - people_ori_1.y);
				// 比对:人是否在泵顶里面
				if (pump_top_ori.contains(point_people_feet_center))
				{
					rect_peple_ori = people_rect;

					// std::cout << "The point is inside the rectangle!" << std::endl;
					// 人头检测
					rect_head = head_detect(image_ori_all, rect_peple_ori, param_map, score); // 这里 rect_peple_ori 需要替换成人的原始坐标(而且还必须是满足要求的坐标)
					//! 这里需要对人头检测进行判空,不然 人头分类检测 拿到的就是空数据,会报错
					// 根据算法需求,宽高分别小于24要过滤
					if(rect_head.width < 24 || rect_head.height < 24)
					{
						continue;
					}
					// 人头分类检测
					category = helmet_detect(image_ori_all, rect_head, param_map, helmet_score);
				}
				else
				{
					// std::cout << "The point is outside the rectangle!" << std::endl;
				}
			}
			if(category == -1)
			{
				return {};
			}
			return rect_peple_ori;//改为返回人的目标
		}

		cv::Rect head_detect(cv::Mat image_ori_all, cv::Rect rect, std::map<std::string, float> &param_map, float &score)
		{
			cv::Rect roi(rect.x, rect.y, rect.width, rect.height);
			cv::Mat image = image_ori_all(roi).clone();
			cv::Rect roiRect;

			float con_thres = param_map.count("head_conf_thres") ? param_map["head_conf_thres"] : 0.6f;
			float iou_thres = param_map.count("nms_thres") ? param_map["nms_thres"] : 0.5f;
			init_data128();
			auto new_shape = cv::Size(128, 128);
			cv::Mat blob;
			float ratio = 1.f;
			int pad_h = 0;
			int pad_w = 0;

			std::tie(blob, ratio) = preprocess_detection(image, pad_h, pad_w, new_shape);
			unsigned char *blobdata = blob.ptr<uchar>();
			std::vector<int> v_blob;
			v_blob.push_back(1);
			v_blob.push_back(blob.rows);

			v_blob.push_back(blob.cols);
			v_blob.push_back(blob.channels());
			auto network_results = net_detect_3->forward(blob.data, v_blob, RKNN_TENSOR_NHWC);

			std::vector<std::string> out_names = {"355", "340", "output0"};

			std::vector<std::shared_ptr<glasssix::memory::tensor<float>>> forwards;

			for (size_t i = 0; i < out_names.size(); ++i)
			{
				forwards.push_back(network_results[out_names[i]]);
			}

			int candicate_num = 0;
			std::vector<cv::Mat> v_roi;

			auto real_output = Yovo8se_Concat_128(forwards, con_thres, candicate_num);

			auto nms_result = post_process(real_output, blob, pad_h, pad_w, 1.f / ratio, candicate_num, con_thres, iou_thres);
			int num = 0;
			//从人头结果里面拿取人头的坐标
			for (auto &pump : nms_result)
			{
				int x1 = std::round(pump[0]) > 0 ? std::round(pump[0]) : 0;
				int y1 = std::round(pump[1]) > 0 ? std::round(pump[1]) : 0;
				int x2 = std::round(pump[2]) < image.cols ? std::round(pump[2]) : image.cols;
				int y2 = std::round(pump[3]) < image.rows ? std::round(pump[3]) : image.rows;
				score = pump[4];
				int x1_ori = x1 + rect.x;
				int x2_ori = x2 + rect.x;
				int y1_ori = y1 + rect.y;
				int y2_ori = y2 + rect.y;
				//这里可能出现多个目标的时候,导致 x1 > x2 、y1 > y2 的情况
				if(x1_ori > x2_ori || y1_ori > y2_ori)
				{
					continue;
				}
				num++;
				roiRect = {x1_ori, y1_ori, x2_ori - x1_ori, y2_ori - y1_ori};
				cv::Mat roi = image_ori_all(roiRect).clone();
				v_roi.push_back(roi);
			}
			return roiRect;
		}

		int helmet_detect(cv::Mat image_ori_all, cv::Rect rect, std::map<std::string, float> &param_map, float &helmet_score)
		{
			float con_thres = param_map.count("head_score_conf_thres") ? param_map["head_score_conf_thres"] : 0.7f;

			cv::Rect roi(rect.x, rect.y, rect.width, rect.height);
			cv::Mat image = image_ori_all(roi).clone();

			auto new_shape = cv::Size(96, 96);
			cv::Mat blob;
			float ratio = 1.f;
			int pad_h = 0;
			int pad_w = 0;

			std::tie(blob, ratio) = preprocess_detection(image, pad_h, pad_w, new_shape);
			unsigned char *blobdata = blob.ptr<uchar>();
			std::vector<int> v_blob;
			v_blob.push_back(1);
			v_blob.push_back(blob.rows);

			v_blob.push_back(blob.cols);
			v_blob.push_back(blob.channels());
			auto network_results = net_detect_4->forward(blob.data, v_blob, RKNN_TENSOR_NHWC);

			std::vector<std::string> out_names = {"output0"};

			//& 直接拿结果
			auto result = network_results[out_names[0]]->cpu_data();

			int index = std::max_element(result, result + 3) - result;
			// 如果检测结果为 {0: 'head', 1: 'helmet', 2: 'no'} 中的head,当值低于0.7,要过滤
			if(index == 0 && *result < con_thres)
			{
				index = -1;
			}
			helmet_score = result[index];

			return index;
		}
		//! 四个模型检测结束

		exposing::param_string version() const
		{
			return "1.0.0";
		}

	private:
		std::unique_ptr<rknnwrapper::rknn_wrapper> net_detect_1;
		std::unique_ptr<rknnwrapper::rknn_wrapper> net_detect_2;
		std::unique_ptr<rknnwrapper::rknn_wrapper> net_detect_3;
		std::unique_ptr<rknnwrapper::rknn_wrapper> net_detect_4;

		std::vector<std::string> phais;

		std::string model_directory_;
		int device_;

		std::vector<int> posture_add_weight_1280;
		std::vector<int> posture_mul_weight_1280;
		std::vector<int> data;
	};

	pumptop_helmet_detector_impl::pumptop_helmet_detector_impl()
	{
	}

	pumptop_helmet_detector_impl::~pumptop_helmet_detector_impl()
	{
	}
	void pumptop_helmet_detector_impl::init(const exposing::param_string &models_directory, std::int32_t device)
	{
		impl_ = std::make_unique<impl>(models_directory, device);
	}

	exposing::param_string pumptop_helmet_detector_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<pumptop_helmet_info> pumptop_helmet_detector_impl::detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, const exposing::param_hash_map<exposing::param_string, float> &param_map_abi) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"pumptop_helmet_detector_internal object not initialized");
		std::map<std::string, float> param_map;
		for (auto it : param_map_abi)
		{
			param_map.insert(std::make_pair(it.key(), it.value()));
		}
		return impl_->detect(bitmap, channels, height, width, param_map);
	}
}
