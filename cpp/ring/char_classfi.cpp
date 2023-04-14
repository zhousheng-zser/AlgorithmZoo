#include "char_classfi.hpp"
#include <Primitives/tensor_conversions.hpp>
namespace glasssix
{
	namespace ring
	{
		struct CmpIdxScorePair {
			bool operator() (const std::pair<int, float>& a, const std::pair<int, float>& b) {
				return a.second <= b.second;
			}
		};

		const static char cool_roll_label_index[] = { '0', '1','2','3','4','5','6','7','8','9','A','C','I' };
		//const static char heavy_rail_label_index[] = { '0', '1','2','3','4','5','6','7','8','9','A','B','C','G','P','X','E' };
		const static char heavy_rail_label_index[] = { '0', '1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G','H','I',
			'J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z' };
		char_classfi::char_classfi(label_type lt)
		{
			switch (lt)
			{
			case glasssix::ring::label_type::COOL_ROLL:
				label_index_ = cool_roll_label_index;
				break;
			case glasssix::ring::label_type::HEAVY_RAIL:
				label_index_ = heavy_rail_label_index;
				break;
			default:
				break;
			}
		}
		std::pair<char, float> char_classfi::detect(cv::Mat& img, excalibur::pipeline<float>& classfi_instance)
		{
			cv::Mat pre_img = pre_handel_img(img);
			auto input_img = std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{1, 64, 48, pre_img.channels()}, -1, memory::NHWC);
			std::copy(pre_img.data, pre_img.data + pre_img.step[0] * pre_img.rows, input_img->mutable_cpu_data());
			input_img->convert_order();
			auto result = classfi_instance.forward(input_img | memory::tensor_convert_to<float>);
			std::vector<float> detections(result["output"]->cpu_data(), result["output"]->cpu_data() + result["output"]->count());
			auto biggest_index = std::distance(detections.begin(), std::max_element(detections.begin(), detections.end()));
			// softmax
			float sum = 0.f;
			for (int i = 0; i < detections.size(); i++)
			{
				sum += std::exp(detections[i]);
			}
			return { label_index_[biggest_index], std::exp(detections[biggest_index]) / sum };
		}

		std::vector<std::array<std::pair<char, float>,2>> char_classfi::detectBatchTop2(std::vector<cv::Mat>& imgs, excalibur::pipeline<float>& classfi_instance) {
			std::vector<std::array<std::pair<char, float>, 2>> rst;
			for (auto& img : imgs) {
				img = pre_handel_img(img);
			}
			int BatchSize = imgs.size();

			int intSteps = 64 * 48 * imgs[0].channels();
			auto inputs_img = std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{int(imgs.size()), 64, 48, imgs[0].channels()}, -1, memory::NHWC);
			for (int i = 0; i < BatchSize; i++) {
				auto pre_img = imgs[i];
				std::copy(pre_img.data, pre_img.data + pre_img.step[0] * pre_img.rows, inputs_img->mutable_cpu_data() + i * intSteps);
			}
			inputs_img->convert_order();
			auto intput_tensor = inputs_img | memory::tensor_convert_to<float>;

			auto result = classfi_instance.forward(intput_tensor);

			int outSteps = result["output"]->count() / BatchSize;
			for (int i = 0; i < BatchSize; i++) {
				std::vector<float> detection(result["output"]->cpu_data() + i * outSteps, result["output"]->cpu_data() + i * outSteps + outSteps);
				// softmax
				float sum = 0.f;
				for (int i = 0; i < detection.size(); i++)
				{
					sum += std::exp(detection[i]);
				}

				//find max confidence score top 2
				std::priority_queue<std::pair<int, float>,std::vector<std::pair<int, float>>, CmpIdxScorePair> maxes_index_score;
				for (int i = 0; i < detection.size(); i++) {
					maxes_index_score.push({ i,detection[i] });
				}
				std::array<std::pair<char, float>, 2> top2_char_conf;
				for (auto& char_conf : top2_char_conf) {
					auto [idx,score] = maxes_index_score.top();
					char_conf.first = label_index_[idx];
					char_conf.second = std::exp(score) / sum;
					maxes_index_score.pop();
				}

				rst.push_back(top2_char_conf);
			}

			return rst;
		}

		std::vector<std::pair<char, float>> char_classfi::detectBatch(std::vector<cv::Mat>& imgs, excalibur::pipeline<float>& classfi_instance) {
			std::vector<std::pair<char, float>> rst;
			for (auto& img : imgs) {
				img = pre_handel_img(img);
			}
			int BatchSize = imgs.size();

			int intSteps = 64 * 48 * imgs[0].channels();
			auto inputs_img = std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{int(imgs.size()), 64, 48, imgs[0].channels()}, -1, memory::NHWC);
			for (int i = 0; i < BatchSize; i++) {
				auto pre_img = imgs[i];
				std::copy(pre_img.data, pre_img.data + pre_img.step[0] * pre_img.rows, inputs_img->mutable_cpu_data() + i * intSteps);
			}
			inputs_img->convert_order();
			auto intput_tensor = inputs_img | memory::tensor_convert_to<float>;

			auto result = classfi_instance.forward(intput_tensor);

			int outSteps = result["output"]->count() / BatchSize;
			for (int i = 0; i < BatchSize; i++) {
				std::vector<float> detection(result["output"]->cpu_data() + i * outSteps, result["output"]->cpu_data() + i * outSteps + outSteps);
				auto biggest_index = std::distance(detection.begin(), std::max_element(detection.begin(), detection.end()));
				// softmax
				float sum = 0.f;
				for (int i = 0; i < detection.size(); i++)
				{
					sum += std::exp(detection[i]);
				}
				rst.push_back({ label_index_[biggest_index], std::exp(detection[biggest_index]) / sum });
			}

			return rst;
		}

		std::vector<std::vector<std::pair<char, float>>> char_classfi::detectBatch(std::vector<std::vector<cv::Mat>>& imgVec, excalibur::pipeline<float>& classfi_instance) {
			std::vector<std::vector<std::pair<char, float>>> rsts;

			std::vector<int> num_list;
			std::vector<cv::Mat> imgs;
			for (std::vector<cv::Mat> Txt_chars : imgVec) {
				num_list.push_back(Txt_chars.size());
				for(cv::Mat& img: Txt_chars)
					imgs.push_back(pre_handel_img(img));
			}
			int BatchSize = imgs.size();

			int inSteps = 64 * 48 * imgs[0].channels();
			auto inputs_img = std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{int(imgs.size()), 64, 48, imgs[0].channels()}, -1, memory::NHWC);
			for (int i = 0; i < BatchSize; i++) {
				auto pre_img = imgs[i];
				std::copy(pre_img.data, pre_img.data + pre_img.step[0] * pre_img.rows, inputs_img->mutable_cpu_data() + i * inSteps);
			}
			inputs_img->convert_order();
			auto intput_tensor = inputs_img | memory::tensor_convert_to<float>;
			auto result = classfi_instance.forward(intput_tensor);

			// unpack
			int cur = 0;
			const int oneSteps = result["output"]->count() / BatchSize; // in fact EQ 36
			for (int txt_chars_num : num_list) {
				std::vector<std::pair<char, float>> txt_rst;
				for (int i = 0; i < txt_chars_num; ++i) {
					std::vector<float> detection(
						result["output"]->cpu_data() + (cur + i) * oneSteps,
						result["output"]->cpu_data() + (cur + i + 1) * oneSteps
					);
					auto biggest_index = std::distance(detection.begin(), std::max_element(detection.begin(), detection.end()));
					// softmax
					float sum = 0.f;
					for (int i = 0; i < detection.size(); i++)
					{
						sum += std::exp(detection[i]);
					}
					txt_rst.push_back({ label_index_[biggest_index], std::exp(detection[biggest_index]) / sum });
				}
				rsts.push_back(txt_rst);
				cur += txt_chars_num;
			}

			return rsts;
		}

		cv::Mat char_classfi::pre_handel_img(cv::Mat& img) {
			int H = img.rows;
			int W = img.cols;
			float ratio_w = (float)W / 48.0;
			float ratio_h = (float)H / 64.0;
			cv::Mat resize_img;
			if (ratio_w == ratio_h)
			{
				cv::resize(img, resize_img, cv::Size2i{ 48, 64 });
			}
			else if (ratio_w > ratio_h) {
				int new_x = 48;
				int new_y = (int)(H / ratio_w);
				int pad1 = (int)((64 - new_y) / 2);
				int pad2 = 64 - new_y - pad1;
				cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
				cv::copyMakeBorder(resize_img, resize_img, pad1, pad2, 0, 0, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
			}
			else {
				int new_y = 64;
				int new_x = (int)(W / ratio_h);
				int pad1 = (int)((48 - new_x) / 2);
				int pad2 = 48 - new_x - pad1;
				cv::resize(img, resize_img, cv::Size2i{ new_x, new_y });
				cv::copyMakeBorder(resize_img, resize_img, 0, 0, pad1, pad2, cv::BORDER_CONSTANT, cv::Scalar{ 114,114,114 });
			}
			return resize_img;
		}
	}
}
