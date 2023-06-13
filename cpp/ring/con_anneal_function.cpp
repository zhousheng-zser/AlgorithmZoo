#include "con_anneal_function.hpp"
#include <algorithm>
#include <numeric>
#include <math.h>
#include <opencv2/imgproc/types_c.h>

namespace glasssix
{
namespace ring
{
static inline float sigmoid_x(float x)
{
	return static_cast<float>(1.f / (1.f + exp(-x)));
}
// ca yolo
std::shared_ptr<glasssix::memory::tensor<float>> tensor_transpose_0231(const std::shared_ptr<glasssix::memory::tensor<float>>& bottom) {
	int num = bottom->num();
	int C = 3;
	int D = 6;
	int height = bottom->height();
	int width = bottom->width();
	//CHECK_EQ(bottom->channels(), D * C);
	auto top = std::make_shared<glasssix::memory::tensor<float>>(std::vector<int>{num* C, height, width, D}, -1, memory::NCHW);
	int DHW_step = width * height * D;
	int HW_step = width * height;

	int count = top->count();
	int countb = bottom->count();
	for (int nc = 0; nc < num * C; nc++) {
		const float* bottom_ptr = bottom->cpu_data() + DHW_step * nc; // bottom_ptr -> D * HW
		float* top_ptr = top->mutable_cpu_data() + DHW_step * nc; // top_ptr -> HW * D

		for (int i = 0; i < HW_step; i++)
			for (int d = 0; d < D; d++)
				top_ptr[i * D + d] = bottom_ptr[d * HW_step + i];
	}
	top->reshape(std::vector<int>{num, C, height* width, D}); // 1 * C * HW * D
	return top;
}

const float stride[3] = { 8, 16, 32 };
const float anchors[3][6] = {
{10,13,  16,30,   33,23},    /* OP [120 160] FOR {ch1} {ch2} {ch3} */
{30,61,  62,45,   59,119},   /*    [60  80 ] */
{116,90, 156,198, 373,326}   /*    [30  40 ] */
};

std::vector<std::array<float, 6>> concat_yolo(std::array<std::shared_ptr<glasssix::memory::tensor<float>>, 3>& yoloRst) {
	std::vector<std::array<float, 6>> result;
	for (int n = 0; n < 3; n++)
	{
		auto& block = yoloRst[n];
		block = tensor_transpose_0231(block);
		int order = block->order();
		int num = block->num();
		int C = block->channels(); // 3
		int H = block->height(); // [rows * cols]
		int W = block->width(); // 6
		int HW_step = H * W;

		int num_grid_x = (int)(1280 / stride[n]);
		int num_grid_y = (int)(960 / stride[n]);
		// channel
		for (int q = 0; q < 3; q++)
		{
			const float anchor_w = anchors[n][q * 2];
			const float anchor_h = anchors[n][q * 2 + 1];
			for (int i = 0; i < num_grid_y; i++)
			{
				for (int j = 0; j < num_grid_x; j++)
				{
					std::array<float, 6> element;
					int cur = q * HW_step + (i * num_grid_x + j) * W; // W==6
					float* pdata = block->mutable_cpu_data() + cur;

					pdata[0] = (sigmoid_x(pdata[0]) * 2.f - 0.5f + j) * stride[n];  //cx
					pdata[1] = (sigmoid_x(pdata[1]) * 2.f - 0.5f + i) * stride[n];  //cy
					pdata[2] = powf(sigmoid_x(pdata[2]) * 2.f, 2.f) * anchor_w;     //w
					pdata[3] = powf(sigmoid_x(pdata[3]) * 2.f, 2.f) * anchor_h;     //h
					pdata[4] = sigmoid_x(pdata[4]);
					pdata[5] = sigmoid_x(pdata[5]);

					element[0] = pdata[0];
					element[1] = pdata[1];
					element[2] = pdata[2];
					element[3] = pdata[3];
					element[4] = pdata[4];
					element[5] = pdata[5];
					result.push_back(element);
				}
			}
		}
	}
	return result;
}

std::vector<Bbox> nms_cpu(std::vector<std::array<float, 6>>& target_info, float iou_thres, float conf_thres) {
	std::vector<Bbox> bboxes;
	for (int row = 0; row < target_info.size(); row++) {
		std::array<float, 6> info_sentence = target_info[row];
		float conf = info_sentence[4];
		if (conf < conf_thres) {
			continue;
		}
		else {
			Bbox box;
			box.xmin = info_sentence[0] - info_sentence[2] / 2;
			box.ymin = info_sentence[1] - info_sentence[3] / 2;
			box.xmax = info_sentence[0] + info_sentence[2] / 2;
			box.ymax = info_sentence[1] + info_sentence[3] / 2;
			box.score = conf;
			bboxes.push_back(box);
		}
	}

	if (bboxes.empty()) return bboxes;
	// 1.之前需要按照score排序
	std::sort(bboxes.begin(), bboxes.end(), [&](Bbox b1, Bbox b2) {return b1.score > b2.score; });
	// 2.先求出所有bbox自己的大小
	std::vector<float> area(bboxes.size());
	for (int i = 0; i < bboxes.size(); ++i) {
		area[i] = (bboxes[i].xmax - bboxes[i].xmin + 1) * (bboxes[i].ymax - bboxes[i].ymin + 1);
	}
	// 3.循环
	for (int i = 0; i < bboxes.size(); ++i) {
		for (int j = i + 1; j < bboxes.size(); ) {
			float left = std::max(bboxes[i].xmin, bboxes[j].xmin);
			float right = std::min(bboxes[i].xmax, bboxes[j].xmax);
			float top = std::max(bboxes[i].ymin, bboxes[j].ymin);
			float bottom = std::min(bboxes[i].ymax, bboxes[j].ymax);
			float width = std::max(right - left + 1, 0.f);
			float height = std::max(bottom - top + 1, 0.f);
			float u_area = height * width;
			float iou = (u_area) / (area[i] + area[j] - u_area);
			if (iou >= iou_thres) {
				bboxes.erase(bboxes.begin() + j);
				area.erase(area.begin() + j);
			}
			else {
				++j;
			}
		}
	}
	if (bboxes.size() < 2) return bboxes;
	std::sort(bboxes.begin(), bboxes.end(), [&](Bbox b1, Bbox b2) {return b1.score > b2.score; });
	return bboxes;
}

}
}