#include "char_segment.hpp"
#include <Primitives/tensor_conversions.hpp>
namespace glasssix
{
	namespace heimdall
	{
		char_segment::char_segment()
		{
			iou_thres_ = 0.6;
			delet_iou_thres_ = 0.25;
			stride_ = 8;
			add_segement_ = true;
		}
		char_segment::char_segment(float iou_thres, float delet_iou_thres, int stride, bool add_segement)
		{
			iou_thres_ = iou_thres;
			delet_iou_thres_ = delet_iou_thres;
			stride_ = stride;
			add_segement_ = add_segement;
		}

		std::vector<float> char_segment::detect(cv::Mat& img, const bool with_blank, excalibur::pipeline<float>& segement_instance, int factory_type)
		{
			cv::Mat pre_img = pre_handel_img(img, stride_, factory_type);
			//std::cout << "------------------- cut mat input -------------------" << std::endl;
			//for (int i = 0; i < 10; ++i)
			//{
			//	std::cout << (float)img.data[i] << std::endl;
			//}

			auto input_img = std::make_shared <memory::tensor<std::uint8_t>>(std::vector<int>{1, 64, pre_img.cols, pre_img.channels()}, -1, memory::NHWC);
			std::copy(pre_img.data, pre_img.data + pre_img.step[0] * pre_img.rows, input_img->mutable_cpu_data());
			input_img->convert_order();
			

			//std::cout << "------------------- segment input_img input -------------------" << std::endl;
			//for (int i = 0; i < 10; ++i)
			//{
			//	std::cout << (float)input_img.get()->cpu_data()[i] << std::endl;
			//}

			auto result = segement_instance.forward(input_img | memory::tensor_convert_to<float>);


			const float* detections = result["output"]->cpu_data();  //创建保存输出结果的vector
			


			int count = result["output"]->count();

			int w_length = result["output"]->data_shape()[3];

			//std::cout << "------------------- segment ouput -------------------" << w_length << std::endl;
			//for (int j = 0; j < count; j++)
			//	std::cout << detections[j] << std::endl;
			

			std::vector<cv::Point2f> trans_detection;
			if (with_blank) 
			{
				for (size_t i = 0; i < count / 3; i++)
				{
					if (detections[i] >= 2)
					{
						float temp_x = detections[i + w_length] * 6.4 + i * stride_;
						float temp_y = detections[i + 2 * w_length] * 6.4 + i * stride_;
						trans_detection.push_back(cv::Point2f{ temp_x, temp_y });
					}
				}
			}
			else
			{
				for (size_t i = 0; i < count / 2; i++)
				{
					float temp_x = detections[i] * 6.4 + i * stride_;
					float temp_y = detections[i + w_length] * 6.4 + i * stride_;
					trans_detection.push_back(cv::Point2f{ temp_x, temp_y });
				}
			}

			if (add_segement_) // ??
			{
				cv::Mat supply_img = pre_img(cv::Range::all(), cv::Range(pre_img.cols - 64, pre_img.cols)).clone();
				cv::flip(supply_img, supply_img, 1);
				auto input_img1 = std::make_shared <memory::tensor<std::uint8_t>>(std::vector<int>{1, 64, 64, supply_img.channels()}, -1, memory::NHWC);
				std::copy(supply_img.data, supply_img.data + supply_img.step[0] * supply_img.rows, input_img1->mutable_cpu_data());
				input_img1->convert_order();
				auto result1 = segement_instance.forward(input_img1 | memory::tensor_convert_to<float>);
				const float* detections1 = result1["output"]->cpu_data();  //创建保存输出结果的vector
				int count = result1["output"]->count();
				cv::Point2f temp_cordi;
				if (with_blank)
				{
					if (detections1[0] >= 2)
					{
						temp_cordi.x = pre_img.cols - detections1[2] * 6.4;
						temp_cordi.y = pre_img.cols - detections1[1] * 6.4;
						trans_detection.push_back(temp_cordi);
					}
				}
				else
				{
					temp_cordi.x = pre_img.cols - detections1[1] * 6.4;
					temp_cordi.y = pre_img.cols - detections1[0] * 6.4;
					trans_detection.push_back(temp_cordi);
				}
			}

			std::vector<std::pair<cv::Point2f, int>> merge_bbox = get_merge_box(trans_detection, iou_thres_);
			std::sort(merge_bbox.begin(), merge_bbox.end(), [](const std::pair<cv::Point2f, int>& left, const std::pair<cv::Point2f, int>& right) {return left.second > right.second; });
			std::vector<std::pair<cv::Point2f, int>> post_bbox = delet_error_bbox(merge_bbox, delet_iou_thres_);
			std::sort(post_bbox.begin(), post_bbox.end(), [](const std::pair<cv::Point2f, int>& a, const std::pair<cv::Point2f, int>& b) {return a.first.x < b.first.x; });
			std::vector<float> result_cordi = merge_near_box(post_bbox);

			return result_cordi;
		}
		cv::Mat char_segment::pre_handel_img(cv::Mat& img, int& stride, int factory_type) {
			//图像resize到高为64，宽为stride的倍数
			int h = img.rows;
			int w = img.cols;
			int c = img.channels();
			float ratio = (float)h / 64;
			int new_w = (int)(w / ratio);
			if (factory_type == 0) { // 0 : hot roll
				if (new_w > 330 && new_w < 370) {
					new_w = 400;
				}
			}
			cv::resize(img, img, cv::Size2i{ new_w, 64 });
			int right_left_extend_w = factory_type == 1 ? 24 : 0; // 1 : cool roll
			int pad_w = stride - new_w % stride;
			if (pad_w == 8)
			{
				return img;
			}
			else {
				cv::copyMakeBorder(img, img, 0, 0, right_left_extend_w, right_left_extend_w + pad_w, cv::BORDER_CONSTANT, 0);
				return img;
			}
		}

		float char_segment::get_bbox_iou(cv::Point2f& point1, cv::Point2f& point2) {
			float max_start = std::max(point1.x, point2.x);
			float min_start = std::min(point1.x, point2.x);
			float max_end = std::max(point1.y, point2.y);
			float min_end = std::min(point1.y, point2.y);
			if (min_end <= max_start)
			{
				return 0.0;
			}
			else {
				return (min_end - max_start) / (max_end - min_start);
			}
		}
		void char_segment::keep_index_vector(std::vector<cv::Point2f>& trans_bbox, std::vector<int>& retain_index) {
			auto temp_bbox = trans_bbox;
			trans_bbox.clear();
			for (size_t i = 0; i < retain_index.size(); i++)
			{
				trans_bbox.push_back(temp_bbox[retain_index[i]]);
			}
		}
		void char_segment::keep_index_vector_pair(std::vector<std::pair<cv::Point2f, int>>& merge_bbox, std::vector<int>& retain_index) {
			auto temp_bbox = merge_bbox;
			merge_bbox.clear();
			for (size_t i = 0; i < retain_index.size(); i++)
			{
				merge_bbox.push_back(temp_bbox[retain_index[i]]);
			}
		}
		std::vector<std::pair<cv::Point2f, int>> char_segment::get_merge_box(std::vector<cv::Point2f> trans_detections, float& thres) {
			// std::pair<cv::Point2f, int>代表<框坐标，合并个数>
			std::vector<std::pair<cv::Point2f, int>> merge_detections;
			cv::Point2f frist_bbox;
			while (trans_detections.size() > 1)
			{
				frist_bbox = trans_detections[0];
				int count_num = 1;
				std::vector<int> retain_index;
				for (int i = 1; i < trans_detections.size(); i++)
				{

					float ratio = get_bbox_iou(frist_bbox, trans_detections[i]);
					if (ratio >= thres)
					{
						frist_bbox.x = (frist_bbox.x * count_num + trans_detections[i].x) / (1 + count_num);
						frist_bbox.y = (frist_bbox.y * count_num + trans_detections[i].y) / (1 + count_num);
						count_num += 1;
					}
					else {
						retain_index.push_back(i);
					}
				}
				merge_detections.push_back(std::pair{ frist_bbox, count_num });
				if (retain_index.size() == 1)
				{
					merge_detections.push_back(std::pair{ trans_detections[retain_index[0]], 1 });
				}
				keep_index_vector(trans_detections, retain_index);


			}
			return merge_detections;
		}
		std::vector<std::pair<cv::Point2f, int>> char_segment::delet_error_bbox(std::vector<std::pair<cv::Point2f, int>>& merge_bbox, const float& num_thres) {
			std::vector<std::pair<cv::Point2f, int>> retain_box;
			size_t num = merge_bbox.size();
			while (merge_bbox.size() > 1)
			{
				cv::Point2f frist_box = merge_bbox[0].first;
				retain_box.push_back(merge_bbox[0]);
				std::vector<int> retain_index;
				for (int i = 1; i < merge_bbox.size(); i++)
				{
					float ratio = get_bbox_iou(frist_box, merge_bbox[i].first);
					if (ratio <= num_thres)
					{
						retain_index.push_back(i);
					}
				}
				if (retain_index.size() == 1)
				{
					retain_box.push_back(merge_bbox[retain_index[0]]);
				}
				keep_index_vector_pair(merge_bbox, retain_index);
			}
			return num > 1 ? retain_box : merge_bbox;
		}
		std::vector<float> char_segment::merge_near_box(std::vector<std::pair<cv::Point2f, int>>& box) {
			std::vector<float> result_cordi;
			result_cordi.push_back(box[0].first.x);
			for (size_t i = 0; i < box.size() - 1; i++)
			{
				float cordi_one = box[i].first.y;
				float cordi_second = box[i + 1].first.x;
				float save_num = (box[i].second * cordi_one + box[i + 1].second * cordi_second) / (box[i].second + box[i + 1].second);
				result_cordi.push_back(save_num);
			}
			result_cordi.push_back(box[box.size() - 1].first.y);
			return result_cordi;
		}
	}
}
