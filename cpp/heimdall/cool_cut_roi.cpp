#include "cool_cut_roi.hpp"
#include <algorithm>
namespace glasssix
{
	namespace heimdall
	{
		cut_reg_roi::cut_reg_roi(int threshold) : threshold_{threshold} {}
		cordinate_roi cut_reg_roi::cut_roi_gather(std::vector<std::vector<cv::Point2f>> pre_rois, cv::Mat& img) {
			cordinate_roi result;
			if (pre_rois.size() < 2)
				throw 1;

			std::vector<std::vector<cv::Point2f>> new_bbox = get_coordinate_order(pre_rois, img, threshold_);
			std::pair<std::vector<cv::Mat>, std::vector<float>> rois = handel_img(img, new_bbox);
			result.cordinate = new_bbox;
			result.rois = rois.first;
			result.max_R = rois.second;
			return result;
		}
		bool  cut_reg_roi::triangleCircle(const cv::Point2f& p1, const cv::Point2f& p2, const cv::Point2f& p3, cv::Point2f& center, double& radius) {
			//检查三点是否共线
			if (isThreePointsOnOneLine(p1, p2, p3))
				return false;

			double  x1, x2, x3, y1, y2, y3;

			x1 = p1.x;
			x2 = p2.x;
			x3 = p3.x;
			y1 = p1.y;
			y2 = p2.y;
			y3 = p3.y;

			//求外接圆半径
			double a = sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
			double b = sqrt((x1 - x3) * (x1 - x3) + (y1 - y3) * (y1 - y3));
			double c = sqrt((x2 - x3) * (x2 - x3) + (y2 - y3) * (y2 - y3));
			double p = (a + b + c) / 2;
			double S = sqrt(p * (p - a) * (p - b) * (p - c));
			radius = a * b * c / (4 * S);

			//求外接圆圆心
			double t1 = x1 * x1 + y1 * y1;
			double t2 = x2 * x2 + y2 * y2;
			double t3 = x3 * x3 + y3 * y3;
			double temp = x1 * y2 + x2 * y3 + x3 * y1 - x1 * y3 - x2 * y1 - x3 * y2;
			double x = (t2 * y3 + t1 * y2 + t3 * y1 - t2 * y1 - t3 * y2 - t1 * y3) / temp / 2;
			double y = (t3 * x2 + t2 * x1 + t1 * x3 - t1 * x2 - t2 * x3 - t3 * x1) / temp / 2;

			center.x = x;
			center.y = y;

			return true;
		}
		bool cut_reg_roi::isThreePointsOnOneLine(const cv::Point2f& p1, const cv::Point2f& p2, const cv::Point2f& p3) {
			if (p2.x == p1.x) {
				if (p2.x == p3.x)
					return true;
				return false;
			}
			else {
				if (p2.x == p3.x)
					return true;
			}
			//判断依据：p2与p1两点构成直线的斜率=p2与p3两点构成直线的斜率
			double k1 = (p2.y - p1.y) / (p2.x - p1.x);
			double k2 = (p3.y - p2.y) / (p3.x - p2.x);
			double DIFF = 0.00000001;
			if (fabs(k1 - k2) < DIFF)
				return true;
			return false;
		}
		void cut_reg_roi::out_inner_turn(std::vector<cv::Point2f>& singel_point) {
			cv::Point2f point0 = singel_point[0];
			cv::Point2f point3 = singel_point[3];
			singel_point[0] = singel_point[1];
			singel_point[1] = point0;
			singel_point[3] = singel_point[2];
			singel_point[2] = point3;
		}
		float cut_reg_roi::getAngelOfTwoVector(cv::Point2f& pt1, cv::Point2f& pt2, cv::Point2f& c) {
			// c为公共点
			cv::Point2f vec1 = pt2 - c;
			cv::Point2f vec2 = pt1 - c;
			float point_dot = vec1.dot(vec2);
			float cos_theta = point_dot / (std::sqrt(vec1.dot(vec1)) * std::sqrt(vec2.dot(vec2)));
			float theta = std::acos(cos_theta);
			return theta;
		}
		std::vector<std::vector<cv::Point2f>> cut_reg_roi::get_coordinate_order(std::vector<std::vector<cv::Point2f>>& cordi, cv::Mat& img, const int& threshold) {
			std::vector<int> select_cordi1{ 0, 3 };
			std::vector<int> select_cordi2{ 1, 2 };
			std::vector<std::vector<cv::Point2f>> medium_cordi;
			std::vector<std::vector<cv::Point2f>> new_cordi;
			std::vector<std::vector<cv::Point2f>> reback_cordi;
			std::vector<cv::Point2f> temp_cordi;
			for (size_t i = 0; i < cordi.size(); i++) {
				temp_cordi = cordi[i];
				//std::vector<cv::Point2f> cordi_for_sort(4);
				std::vector<float> distance_temp(3);
				for (size_t j = 1; j < temp_cordi.size(); j++) {
					cv::Point2f distance = temp_cordi[j] - temp_cordi[0];
					distance_temp[j - 1] = distance.dot(distance);
				}
				auto biggest_index = std::distance(distance_temp.begin(), std::max_element(distance_temp.begin(), distance_temp.end())) + 1;
				auto minnest_index = std::distance(distance_temp.begin(), std::min_element(distance_temp.begin(), distance_temp.end())) + 1;
				int medium_index = 6 - biggest_index - minnest_index;
				float angel1 = getAngelOfTwoVector(temp_cordi[minnest_index], temp_cordi[biggest_index], temp_cordi[0]);
				float angel2 = getAngelOfTwoVector(temp_cordi[minnest_index], temp_cordi[medium_index], temp_cordi[0]);
				if (angel1 > angel2)
				{
					int temp_index = biggest_index;
					biggest_index = medium_index;
					medium_index = temp_index;
				}
				//如何判断小框框
				float ratio = distance_temp[medium_index - 1] / distance_temp[minnest_index - 1];
				if (ratio > 2.2)
				{
					new_cordi.push_back(std::vector<cv::Point2f>{temp_cordi[0], temp_cordi[minnest_index], temp_cordi[biggest_index], temp_cordi[medium_index]});
					medium_cordi.push_back(std::vector<cv::Point2f>{(temp_cordi[0] + temp_cordi[minnest_index]) / 2, (temp_cordi[biggest_index] + temp_cordi[medium_index]) / 2});
					/*medium_cordi[i][0] = (cordi[i][0] + cordi[i][1])/2;
					medium_cordi[i][1] = (cordi[i][2] + cordi[i][3])/2;*/
				}
			}
			float judge_close, judge_remote;
			bool is_continue = true;
			for (size_t k = 0; k < medium_cordi.size(); k++)
			{
				for (size_t l = k + 1; l < medium_cordi.size(); l++)
				{
					for (size_t for_one = 0; for_one < 2; for_one++)
					{
						for (size_t for_second = 0; for_second < 2; for_second++)
						{
							/*int max_number = std::max(k, l);
							int min_number = std::min(k, l);*/
							cv::Point2f difference_matrix = medium_cordi[k][for_one] - medium_cordi[l][for_second];
							float Euclidean_distance = difference_matrix.dot(difference_matrix);
							if (Euclidean_distance <= threshold)
							{
								//判断又没重复找框
								/*if (!remove_new_bbox.empty()) {
									for (size_t number = 0; number < remove_new_bbox.size(); number++)
									{
										if (remove_new_bbox[number] == cv::Point2i{ max_number, min_number })
										{
											is_continue = false;
											continue;
										}
									}
								}
								if (is_continue)
								{*/
								cv::Point2f medium_1 = new_cordi[k][select_cordi1[for_one]] - new_cordi[l][select_cordi2[for_second]];
								cv::Point2f medium_2 = new_cordi[l][select_cordi2[for_second]] - new_cordi[l][select_cordi2[for_second]];
								float dis_1 = medium_1.dot(medium_1);
								float dis_2 = medium_2.dot(medium_2);
								if (dis_1 < dis_2) {
									out_inner_turn(new_cordi[l]);
								}
								cv::Point2f pt1 = new_cordi[k][select_cordi1[1 - for_one]];
								//cout << cordi[k][select_cordi1[for_one]]<<"mm"<< cordi[l][select_cordi1[for_second]]<<endl;
								//cv::Point2f pt2 = (new_cordi[k][select_cordi1[for_one]] + new_cordi[l][select_cordi1[for_second]]) / 2;
								cv::Point2f pt3 = new_cordi[l][select_cordi1[1 - for_second]];
								cv::Point2f pt4 = new_cordi[k][select_cordi2[1 - for_one]];

								//cv::Point2f pt5 = (new_cordi[k][select_cordi2[for_one]] + new_cordi[l][select_cordi2[for_second]]) / 2;
								cv::Point2f pt6 = new_cordi[l][select_cordi2[1 - for_second]];
								cv::Point2f dis_value_close = new_cordi[k][select_cordi1[for_one]] - new_cordi[l][select_cordi1[for_second]];
								cv::Point2f dis_value_remote = new_cordi[k][select_cordi1[for_one]] - new_cordi[l][select_cordi2[for_second]];
								judge_close = dis_value_close.dot(dis_value_close);
								judge_remote = dis_value_remote.dot(dis_value_remote);
								//找到中间点相邻匹配点
								if (judge_close > judge_remote)
								{
									cv::Point2f pt2 = (new_cordi[k][select_cordi1[for_one]] + new_cordi[l][select_cordi2[for_second]]) / 2;
									cv::Point2f pt5 = (new_cordi[k][select_cordi2[for_one]] + new_cordi[l][select_cordi1[for_second]]) / 2;
									//remove_new_bbox.push_back(cv::Point2i{ max_number, min_number });
									reback_cordi.push_back(std::vector<cv::Point2f>{pt1, pt2, pt6, pt4, pt5, pt3});
								}
								else
								{
									cv::Point2f pt2 = (new_cordi[k][select_cordi1[for_one]] + new_cordi[l][select_cordi1[for_second]]) / 2;
									cv::Point2f pt5 = (new_cordi[k][select_cordi2[for_one]] + new_cordi[l][select_cordi2[for_second]]) / 2;
									//remove_new_bbox.push_back(cv::Point2i{ max_number, min_number });
									reback_cordi.push_back(std::vector<cv::Point2f>{pt1, pt2, pt3, pt4, pt5, pt6});
								}

							}
						}
					}

				}
			}
			return reback_cordi;
		}
		std::pair<cv::Point2f, float> cut_reg_roi::get_start_end(cv::Point2f& center, float& R_iner, float& R_out, cv::Point2f& point_inner, cv::Point2f& point_out, bool is_start) {
			float theta_frist = get_radian(center, point_out);
			float theta_second = get_radian(center, point_inner);
			float theta;
			if (is_start)
			{
				theta = std::max(theta_frist, theta_second);
			}
			else {
				theta = std::min(theta_frist, theta_second);
			}
			/*theta1 = 0.5 * theta_frist + 0.5 * theta_second;
			float new_x = R_out * std::cos(theta1) + center.x;
			float new_y = -R_out * std::sin(theta1) + center.y;*/

			float new_x = R_out * std::cos(theta) + center.x;
			float new_y = -R_out * std::sin(theta) + center.y;
			return std::pair{ cv::Point2f(new_x, new_y), theta };
		}
		float cut_reg_roi::get_radian(cv::Point2f& center, cv::Point2f& point_cor) {
			float dis_x = point_cor.x - center.x;
			float dis_y = point_cor.y - center.y;

			float temp_R = std::sqrt(dis_x * dis_x + dis_y * dis_y);
			//if (std::abs(dis_x) > R) {
			//	dis_x = R * (dis_x / abs(dis_x));
			//}
			//if (std::abs(dis_y) > R) {
			//	dis_y = R * (dis_y / abs(dis_y));
			//}
			float cos_theta = dis_x / temp_R;
			float sin_theta = -dis_y / temp_R;
			float theta = std::acos(cos_theta);
			if (sin_theta < 0) {
				theta = -theta;
			}
			return theta;
		}
		cv::Mat cut_reg_roi::cut_roi_region(cv::Mat& img, cv::Point2f& center, cv::Point2f& start_point, cv::Point2f& end_point, float& start_theta, float& end_theta, float& R_out, float& R_inner) {
			std::vector<cv::Mat> cut_img;
			float temp_R, now_theta;
			int new_x, new_y;
			int R_interval = std::ceil(R_out - R_inner);
			int num_sample_point = std::ceil((start_theta - end_theta) * R_out);
			float theta_jiange = (end_theta - start_theta) / num_sample_point;
			cv::Mat temp_crop_img(R_interval, num_sample_point, CV_8UC3);
			int W = img.cols;
			int H = img.rows;
			cv::Mat_<cv::Vec3b> img_reference = img;
			cv::Mat_<cv::Vec3b> temp_reference = temp_crop_img;
			for (int i = 0; i < R_interval; i++)
			{
				temp_R = R_out - i;
				for (int j = 0; j < num_sample_point; j++)
				{
					now_theta = start_theta + theta_jiange * j;
					new_x = std::round(std::cos(now_theta) * temp_R + center.x) - 1;
					float tem = -std::sin(now_theta) * temp_R;
					new_y = std::round(-std::sin(now_theta) * temp_R + center.y) - 1;
					if (new_y > H - 1)
					{
						new_y = H - 1;
					}
					if (new_x > W - 1)
					{
						new_x = W - 1;
					}

					temp_reference(i, j) = img_reference(new_y, new_x);
				}
			}

			return temp_crop_img;
		};
		std::pair<std::vector<cv::Mat>, std::vector<float>> cut_reg_roi::handel_img(cv::Mat& img, std::vector<std::vector<cv::Point2f>>& bbox) {
			std::pair<std::vector<cv::Mat>, std::vector<float>> roi_gather;
			//std::pair< cv::Mat, float> temp_roi_R;
			double radi_out, radi_in; //, radi_max, radi_min;
			float radi_out_f, radi_in_f, theta1, theta2;
			cv::Point2f center_out, center_in, center, distance, start_point, end_point;
			std::vector<float> array_distance(6);
			for (size_t i = 0; i < bbox.size(); i++)
			{
				bool if_circle1 = triangleCircle(bbox[i][0], bbox[i][1], bbox[i][2], center_out, radi_out);
				bool if_circle2 = triangleCircle(bbox[i][3], bbox[i][4], bbox[i][5], center_in, radi_in);
				radi_out_f = static_cast<float>(radi_out);
				radi_in_f = static_cast<float>(radi_in);
				center = (center_out + center_in) / 2;
				for (size_t j = 0; j < bbox[i].size(); j++)
				{
					distance = bbox[i][j] - center;
					array_distance[j] = distance.dot(distance);
				}
				auto max_posi = std::distance(array_distance.begin(), std::max_element(array_distance.begin(), array_distance.end()));
				auto min_posi = std::distance(array_distance.begin(), std::min_element(array_distance.begin(), array_distance.end()));
				auto max_radi = std::sqrt(array_distance[max_posi]);
				auto min_radi = std::sqrt(array_distance[min_posi]);

				//td::cout << max_radi << std::endl;

				//cv::circle(img, bbox[i][0], 3, cv::Scalar{ 0, 0, 255 });
				//cv::circle(img, bbox[i][2], 3, cv::Scalar{ 0, 255, 255 });

				//改变内外圆排布
				if (max_posi > 2)
				{
					std::vector<cv::Point2f> temp = std::vector<cv::Point2f>{ bbox[i][3], bbox[i][4], bbox[i][5], bbox[i][0], bbox[i][1], bbox[i][2] };
					bbox[i] = temp;
				}
				//计算起始点位置
				auto start_point_theta = get_start_end(center, min_radi, max_radi, bbox[i][3], bbox[i][0], true);
				auto end_point_theta = get_start_end(center, min_radi, max_radi, bbox[i][5], bbox[i][2], false);

				//cv::circle(img, center, 2, cv::Scalar{ 0, 0, 255 });
				float theta_devalue = std::abs(start_point_theta.second - end_point_theta.second);
				if (theta_devalue <= 3.1415)
				{
					if (start_point_theta.second < end_point_theta.second)
					{
						start_point_theta = get_start_end(center, min_radi, max_radi, bbox[i][5], bbox[i][2], true);
						end_point_theta = get_start_end(center, min_radi, max_radi, bbox[i][3], bbox[i][0], false);
						std::vector<cv::Point2f> temp1 = std::vector<cv::Point2f>{ bbox[i][2], bbox[i][1], bbox[i][0], bbox[i][5], bbox[i][4], bbox[i][3] };
						bbox[i] = temp1;
					};
				}
				else {

					if (start_point_theta.second > end_point_theta.second)
					{
						start_point_theta = get_start_end(center, min_radi, max_radi, bbox[i][5], bbox[i][2], true);
						end_point_theta = get_start_end(center, min_radi, max_radi, bbox[i][3], bbox[i][0], false);
						std::vector<cv::Point2f> temp1 = std::vector<cv::Point2f>{ bbox[i][2], bbox[i][1], bbox[i][0], bbox[i][5], bbox[i][4], bbox[i][3] };
						bbox[i] = temp1;
					}
					start_point_theta.second = 3.1415926 * 2 + start_point_theta.second;
				}
				/*cv::line(img, bbox[i][0], bbox[i][1], cv::Scalar{ 0, 0, 255 },2);
				cv::line(img, bbox[i][1], bbox[i][2], cv::Scalar{ 0, 0, 255 },2);
				cv::line(img, bbox[i][2], bbox[i][5], cv::Scalar{ 0, 0, 255 },2);
				cv::line(img, bbox[i][5], bbox[i][4], cv::Scalar{ 0, 0, 255 },2);
				cv::line(img, bbox[i][4], bbox[i][3], cv::Scalar{ 0, 0, 255 },2);
				cv::line(img, bbox[i][3], bbox[i][0], cv::Scalar{ 0, 0, 255 },2);
				cv::imshow("img", img);
				cv::waitKey();*/
				cv::Mat new_img = cut_roi_region(img, center, start_point_theta.first, end_point_theta.first, start_point_theta.second, end_point_theta.second, max_radi, min_radi);
				//temp_roi_R = std::pair{ new_img, max_radi };
				roi_gather.first.push_back(new_img);
				roi_gather.second.push_back(max_radi);
			}
			return roi_gather;
		}
	}
}