#include "flame_function.hpp"
#include <algorithm>
#include <numeric>
#include <math.h>

namespace glasssix
{
namespace flame
{
static inline float sigmoid_x(float x)
{
	return static_cast<float>(1.f / (1.f + exp(-x)));
}
// ca yolo
std::shared_ptr<glasssix::memory::tensor<float>> tensor_transpose_0231(const std::shared_ptr<glasssix::memory::tensor<float>>& bottom) {
	int num = bottom->num();
	int C = 3;
	int D = 7;
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

std::vector<Bbox> concat_yolo(std::vector<std::shared_ptr<glasssix::memory::tensor<float>>>& yoloRst, float conf_threshold) {
	std::vector<Bbox> result;
	for (int n = 0; n < 3; n++)
	{
		auto& block = yoloRst[n];
		block = tensor_transpose_0231(block);
		int order = block->order();
		int num = block->num();
		int C = block->channels(); // 3
		int H = block->height(); // [rows * cols]
		int W = block->width(); // 7
		int HW_step = H * W;

		int num_grid_x = (int)(640 / stride[n]);
		int num_grid_y = (int)(640 / stride[n]);
		// channel
		for (int q = 0; q < 3; q++)
		{
			const float anchor_w = anchors[n][q * 2];
			const float anchor_h = anchors[n][q * 2 + 1];
			for (int i = 0; i < num_grid_y; i++)
			{
				for (int j = 0; j < num_grid_x; j++)
				{
					int cur = q * HW_step + (i * num_grid_x + j) * W; // W==7
					float* pdata = block->mutable_cpu_data() + cur;

					float x = (sigmoid_x(pdata[0]) * 2.f - 0.5f + j) * stride[n];  //cx
					float y = (sigmoid_x(pdata[1]) * 2.f - 0.5f + i) * stride[n];  //cy
					float w = powf(sigmoid_x(pdata[2]) * 2.f, 2.f) * anchor_w;     //w
					float h = powf(sigmoid_x(pdata[3]) * 2.f, 2.f) * anchor_h;     //h
					float obj_conf = sigmoid_x(pdata[4]);

					float fire_conf = sigmoid_x(pdata[5]);
					float like_conf = sigmoid_x(pdata[6]);

					if (obj_conf > conf_threshold && fire_conf > like_conf) {
						Bbox bbox;
						bbox.xmin = x - w / 2;
						bbox.xmax = x + w / 2;
						bbox.ymin = y - h / 2;
						bbox.ymax = y + h / 2;
						bbox.score = fire_conf * obj_conf;
						bbox.cid = 0;

						result.push_back(bbox);
					}
				}
			}
		}
	}
	return result;
}

void nms_cpu(std::vector<Bbox>& bboxes, float iou_thres) {
	if (bboxes.empty()) return;
	std::sort(bboxes.begin(), bboxes.end(), [&](Bbox b1, Bbox b2) {return b1.score > b2.score; });
	std::vector<float> area(bboxes.size());
	for (int i = 0; i < bboxes.size(); ++i) {
		area[i] = (bboxes[i].xmax - bboxes[i].xmin + 1) * (bboxes[i].ymax - bboxes[i].ymin + 1);
	}
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
	if (bboxes.size() < 2) return;
	std::sort(bboxes.begin(), bboxes.end(), [&](Bbox b1, Bbox b2) {return b1.score > b2.score; });
}

}
}