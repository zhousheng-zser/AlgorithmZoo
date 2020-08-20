#include <vector>
#include "retina_net_internal.hpp"
#include "Excalibur/operation_make_border.hpp"
#include "Excalibur/operation_resize.hpp"
#include "Primitives/tensor_conversions.hpp"

namespace glasssix::longinus
{
	//processing
	anchor_win  whctrs(anchor_box anchor)
	{
		//Return width, height, x center, and y center for an anchor (window).
		anchor_win win;
		win.w = anchor.w;
		win.h = anchor.h;
		win.x_ctr = anchor.x + 0.5 * (win.w - 1);
		win.y_ctr = anchor.y + 0.5 * (win.h - 1);

		return win;
	}

	anchor_box make_anchors(anchor_win win)
	{
		//Given a vector of widths (ws) and heights (hs) around a center
		//(x_ctr, y_ctr), output a set of anchors (windows).
		anchor_box anchor;
		anchor.x = win.x_ctr - 0.5 * (win.w - 1);
		anchor.y = win.y_ctr - 0.5 * (win.h - 1);
		anchor.w = win.w;
		anchor.h = win.h;

		return anchor;
	}

	std::vector<anchor_box> ratio_enum(anchor_box anchor, std::vector<float> ratios)
	{
		//Enumerate a set of anchors for each aspect ratio wrt an anchor.
		std::vector<anchor_box> anchors;
		for (size_t i = 0; i < ratios.size(); i++)
		{
			anchor_win win = whctrs(anchor);
			float size = win.w * win.h;
			float scale = size / ratios[i];

			win.w = std::round(sqrt(scale));
			win.h = std::round(win.w * ratios[i]);

			anchor_box tmp = make_anchors(win);
			anchors.push_back(tmp);
		}

		return anchors;
	}

	std::vector<anchor_box> scale_enum(anchor_box anchor, std::vector<int> scales)
	{
		//Enumerate a set of anchors for each scale wrt an anchor.
		std::vector<anchor_box> anchors;
		for (size_t i = 0; i < scales.size(); i++)
		{
			anchor_win win = whctrs(anchor);

			win.w = win.w * scales[i];
			win.h = win.h * scales[i];

			anchor_box tmp = make_anchors(win);
			anchors.push_back(tmp);
		}

		return anchors;
	}

	std::vector<anchor_box> generate_anchors(int base_size = 16, std::vector<float> ratios = { 0.5, 1, 2 },
		std::vector<int> scales = { 8, 64 }, int stride = 16, bool dense_anchor = false)
	{
		//Generate anchor (reference) windows by enumerating aspect ratios X
		//scales wrt a reference (0, 0, 15, 15) window.

		anchor_box base_anchor;
		base_anchor.x = 0;
		base_anchor.y = 0;
		base_anchor.h = base_size;
		base_anchor.w = base_size;

		std::vector<anchor_box> ratio_anchors;
		ratio_anchors = ratio_enum(base_anchor, ratios);

		std::vector<anchor_box> anchors;
		for (size_t i = 0; i < ratio_anchors.size(); i++)
		{
			std::vector<anchor_box> tmp = scale_enum(ratio_anchors[i], scales);
			anchors.insert(anchors.end(), tmp.begin(), tmp.end());
		}

		if (dense_anchor)
		{
			assert(stride % 2 == 0);
			std::vector<anchor_box> anchors2 = anchors;
			for (size_t i = 0; i < anchors2.size(); i++) {
				anchors2[i].x += stride / 2;
				anchors2[i].y += stride / 2;
			}
			anchors.insert(anchors.end(), anchors2.begin(), anchors2.end());
		}

		return anchors;
	}

	std::vector<std::vector<anchor_box>> generate_anchors_fpn(bool dense_anchor = false, std::vector<anchor_cfg> cfg = {})
	{
		//Generate anchor (reference) windows by enumerating aspect ratios X
		//scales wrt a reference (0, 0, 15, 15) window.

		std::vector<std::vector<anchor_box>> anchors;
		for (size_t i = 0; i < cfg.size(); i++)
		{
			//stride [32 16 8]
			anchor_cfg tmp = cfg[i];
			int bs = tmp.BASE_SIZE;
			std::vector<float> ratios = tmp.RATIOS;
			std::vector<int> scales = tmp.SCALES;
			int stride = tmp.STRIDE;

			std::vector<anchor_box> r = generate_anchors(bs, ratios, scales, stride, dense_anchor);
			anchors.push_back(r);
		}

		return anchors;
	}

	std::vector<anchor_box> anchors_plane(int height, int width, int stride, std::vector<anchor_box> base_anchors)
	{
		/*
		height: height of plane
		width:  width of plane
		stride: stride ot the original image
		anchors_base: a base set of anchors
		*/

		std::vector<anchor_box> all_anchors;
		for (size_t k = 0; k < base_anchors.size(); k++) {
			for (int ih = 0; ih < height; ih++) {
				int sh = ih * stride;
				for (int iw = 0; iw < width; iw++) {
					int sw = iw * stride;

					anchor_box tmp;
					tmp.x = base_anchors[k].x + sw;
					tmp.y = base_anchors[k].y + sh;
					tmp.h = base_anchors[k].h;
					tmp.w = base_anchors[k].w;
					all_anchors.push_back(tmp);
				}
			}
		}

		return all_anchors;
	}

	void clip_boxes(std::vector<anchor_box>& boxes, int width, int height)
	{
		//Clip boxes to image boundaries.
		for (size_t i = 0; i < boxes.size(); i++)
		{
			if (boxes[i].x < 0)
			{
				boxes[i].x = 0;
			}
			if (boxes[i].y < 0)
			{
				boxes[i].y = 0;
			}
			if (boxes[i].x + boxes[i].w > width)
			{
				boxes[i].w = width - boxes[i].x;
			}
			if (boxes[i].y + boxes[i].h > height)
			{
				boxes[i].h = height - boxes[i].y;
			}
			//        boxes[i].x1 = std::max<float>(std::min<float>(boxes[i].x1, width - 1), 0);
			//        boxes[i].y1 = std::max<float>(std::min<float>(boxes[i].y1, height - 1), 0);
			//        boxes[i].x2 = std::max<float>(std::min<float>(boxes[i].x2, width - 1), 0);
			//        boxes[i].y2 = std::max<float>(std::min<float>(boxes[i].y2, height - 1), 0);
		}
	}

	void clip_box(anchor_box& box, int width, int height)
	{
		//Clip boxes to image boundaries.
		if (box.x < 0) {
			box.x = 0;
		}
		if (box.y < 0) {
			box.y = 0;
		}
		if (box.x + box.w > width) {
			box.w = width - box.x;
		}
		if (box.y + box.h > height) {
			box.h = height - box.y;
		}
		//    boxes[i].x1 = std::max<float>(std::min<float>(boxes[i].x1, width - 1), 0);
		//    boxes[i].y1 = std::max<float>(std::min<float>(boxes[i].y1, height - 1), 0);
		//    boxes[i].x2 = std::max<float>(std::min<float>(boxes[i].x2, width - 1), 0);
		//    boxes[i].y2 = std::max<float>(std::min<float>(boxes[i].y2, height - 1), 0);

	}

	//######################################################################
	//retina_net_native
	//######################################################################

	retina_net_internal::retina_net_internal(std::string phai_path, std::string racy_path, float nms_threshold, int device)
		: nms_threshold_(nms_threshold), device_(device)
	{
		ratio_ = { 1.0 };
		//anchor setting
		feat_stride_fpn_ = { 32, 16, 8 };
		anchor_cfg tmp;
		tmp.SCALES = { 32, 16 };
		tmp.BASE_SIZE = 16;
		tmp.RATIOS = ratio_;
		tmp.ALLOWED_BORDER = 9999;
		tmp.STRIDE = 32;
		cfg_.push_back(tmp);

		tmp.SCALES = { 8, 4 };
		tmp.BASE_SIZE = 16;
		tmp.RATIOS = ratio_;
		tmp.ALLOWED_BORDER = 9999;
		tmp.STRIDE = 16;
		cfg_.push_back(tmp);

		tmp.SCALES = { 2, 1 };
		tmp.BASE_SIZE = 16;
		tmp.RATIOS = ratio_;
		tmp.ALLOWED_BORDER = 9999;
		tmp.STRIDE = 8;
		cfg_.push_back(tmp);

		/* Load the network. */
		pipe.reset(new glasssix::excalibur::pipeline<float>(phai_path, racy_path, device_));
		pipe->enable_profiler();
		bool dense_anchor = false;
		std::vector<std::vector<anchor_box>> anchors_fpn = generate_anchors_fpn(dense_anchor, cfg_);
		for (size_t i = 0; i < anchors_fpn.size(); i++)
		{
			std::string key = "stride" + std::to_string(feat_stride_fpn_[i]);
			anchors_fpn_[key] = anchors_fpn[i];
			num_anchors_[key] = anchors_fpn[i].size();
		}
		allocator = new memory::pool_allocator<float>();
	}

	retina_net_internal::~retina_net_internal()
	{
		delete allocator;
	}

	std::vector<anchor_box> retina_net_internal::bbox_pred(std::vector<anchor_box> anchors, std::vector<std::vector<float>> regress)
	{
		//"""
		//  Transform the set of class-agnostic boxes into class-specific boxes
		//  by applying the predicted offsets (box_deltas)
		//  :param boxes: !important [N 4]
		//  :param box_deltas: [N, 4 * num_classes]
		//  :return: [N 4 * num_classes]
		//  """

		std::vector<anchor_box> rects(anchors.size());
		for (size_t i = 0; i < anchors.size(); i++)
		{
			float width = anchors[i].w;
			float height = anchors[i].h;
			float ctr_x = anchors[i].x + 0.5 * (width - 1.0);
			float ctr_y = anchors[i].y + 0.5 * (height - 1.0);

			float pred_ctr_x = regress[i][0] * width + ctr_x;
			float pred_ctr_y = regress[i][1] * height + ctr_y;
			float pred_w = exp(regress[i][2]) * width;
			float pred_h = exp(regress[i][3]) * height;

			rects[i].x = pred_ctr_x - 0.5 * (pred_w - 1.0);
			rects[i].y = pred_ctr_y - 0.5 * (pred_h - 1.0);
			rects[i].w = pred_w;
			rects[i].h = pred_h;
		}

		return rects;
	}

	anchor_box retina_net_internal::bbox_pred(anchor_box anchor, std::vector<float> regress)
	{
		anchor_box rect;

		float width = anchor.w;
		float height = anchor.h;
		float ctr_x = anchor.x + 0.5 * (width - 1.0);
		float ctr_y = anchor.y + 0.5 * (height - 1.0);

		float pred_ctr_x = regress[0] * width + ctr_x;
		float pred_ctr_y = regress[1] * height + ctr_y;
		float pred_w = exp(regress[2]) * width;
		float pred_h = exp(regress[3]) * height;

		rect.x = pred_ctr_x - 0.5 * (pred_w - 1.0);
		rect.y = pred_ctr_y - 0.5 * (pred_h - 1.0);
		rect.w = pred_w;
		rect.h = pred_h;

		return rect;
	}

	std::vector<face_pts> retina_net_internal::landmark_pred(std::vector<anchor_box> anchors, std::vector<face_pts> facepts)
	{
		std::vector<face_pts> pts(anchors.size());
		for (size_t i = 0; i < anchors.size(); i++)
		{
			float width = anchors[i].w;
			float height = anchors[i].h;
			float ctr_x = anchors[i].x + 0.5 * (width - 1.0);
			float ctr_y = anchors[i].y + 0.5 * (height - 1.0);

			for (size_t j = 0; j < 5; j++)
			{
				pts[i].x[j] = facepts[i].x[j] * width + ctr_x;
				pts[i].y[j] = facepts[i].y[j] * height + ctr_y;
			}
		}

		return pts;
	}

	face_pts retina_net_internal::landmark_pred(anchor_box anchor, face_pts facePt)
	{
		face_pts pt;
		float width = anchor.w;
		float height = anchor.h;
		float ctr_x = anchor.x + 0.5 * (width - 1.0);
		float ctr_y = anchor.y + 0.5 * (height - 1.0);

		for (size_t j = 0; j < 5; j++)
		{
			pt.x[j] = facePt.x[j] * width + ctr_x;
			pt.y[j] = facePt.y[j] * height + ctr_y;
		}

		return pt;
	}

	bool retina_net_internal::compare_bbox(const face_info_internal& a, const face_info_internal& b)
	{
		return a.score > b.score;
	}

	std::vector<face_info_internal> retina_net_internal::nms(std::vector<face_info_internal>& bboxes, float threshold)
	{
		std::vector<face_info_internal> bboxes_nms;
		std::sort(bboxes.begin(), bboxes.end(), compare_bbox);

		int32_t select_idx = 0;
		int32_t num_bbox = static_cast<int32_t>(bboxes.size());
		std::vector<int32_t> mask_merged(num_bbox, 0);
		bool all_merged = false;

		while (!all_merged)
		{
			while (select_idx < num_bbox && mask_merged[select_idx] == 1)
				select_idx++;

			if (select_idx == num_bbox)
			{
				all_merged = true;
				continue;
			}

			bboxes_nms.push_back(bboxes[select_idx]);
			mask_merged[select_idx] = 1;

			anchor_box select_bbox = bboxes[select_idx].rect;
			float area1 = static_cast<float>((select_bbox.w) * (select_bbox.h));
			float x1 = static_cast<float>(select_bbox.x);
			float y1 = static_cast<float>(select_bbox.y);
			float x2 = static_cast<float>(select_bbox.w + select_bbox.x - 1);
			float y2 = static_cast<float>(select_bbox.h + select_bbox.y - 1);

			select_idx++;
			for (int32_t i = select_idx; i < num_bbox; i++)
			{
				if (mask_merged[i] == 1)
					continue;

				anchor_box& bbox_i = bboxes[i].rect;
				float x = std::max<float>(x1, static_cast<float>(bbox_i.x));
				float y = std::max<float>(y1, static_cast<float>(bbox_i.y));
				float w = std::min<float>(x2, static_cast<float>(bbox_i.w + bbox_i.x - 1)) - x + 1;   //<- float ÐÍ²»¼Ó1
				float h = std::min<float>(y2, static_cast<float>(bbox_i.h + bbox_i.y - 1)) - y + 1;
				if (w <= 0 || h <= 0)
					continue;

				float area2 = static_cast<float>((bbox_i.w) * (bbox_i.h));
				float area_intersect = w * h;


				if (static_cast<float>(area_intersect) / (area1 + area2 - area_intersect) > threshold)
				{
					mask_merged[i] = 1;
				}
			}
		}

		return bboxes_nms;
	}

	std::string retina_net_internal::version()
	{
		return "1.0.0";
	}


	std::vector<face_info_internal> retina_net_internal::detect(const unsigned char* bitmap, int channels, int height, int width, int min_size, float threshold, int order)
	{
		if (!bitmap)
		{
			return std::vector<face_info_internal>();
		}

		std::shared_ptr<memory::tensor<std::uint8_t>> temp(new memory::tensor<std::uint8_t>{channels, height, width, device_, (memory::orderType)order, nullptr });
		std::copy(bitmap, bitmap + channels * height * width, temp->mutable_cpu_data());

		if (min_size < 16)
			min_size = 16;

		float scale = min_size / 16.0f;
		int ws = (int(width / scale) + 31) / 32 * 32;
		int hs = (int(height / scale) + 31) / 32 * 32;

		excalibur::resize_cpu(temp, temp, int(height / scale), int(width / scale));
		excalibur::make_border(temp, temp, 0, hs - int(height / scale), 0, ws - int(width / scale));

		auto blob_data = pipe->forward(temp | memory::tensor_convert_to<float>);

		std::string name_bbox = "face_rpn_bbox_pred_";
		std::string name_score = "face_rpn_cls_prob_reshape_";
		std::string name_landmark = "face_rpn_landmark_pred_";

		std::vector<face_info_internal> face_infos;
		for (size_t i = 0; i < feat_stride_fpn_.size(); i++)
		{
			std::string key = "stride" + std::to_string(feat_stride_fpn_[i]);
			int stride = feat_stride_fpn_[i];

			std::string str = name_score + key;
			auto score_blob = pipe->get_featmap(str);
			auto score_blob_count = score_blob->count();
			const float* scoreB = score_blob->cpu_data() + score_blob_count / 2;
			const float* scoreE = scoreB + score_blob_count / 2;
			std::vector<float> score = std::vector<float>(scoreB, scoreE);

			str = name_bbox + key;
			auto bbox_blob = pipe->get_featmap(str);
			auto bbox_blob_count = bbox_blob->count();
			const float* bboxB = bbox_blob->cpu_data();
			const float* bboxE = bboxB + bbox_blob_count;
			std::vector<float> bbox_delta = std::vector<float>(bboxB, bboxE);

			str = name_landmark + key;
			auto landmark_blob = pipe->get_featmap(str);
			auto landmark_blob_count = landmark_blob->count();
			const float* landmarkB = landmark_blob->cpu_data();
			const float* landmarkE = landmarkB + landmark_blob_count;
			std::vector<float> landmark_delta = std::vector<float>(landmarkB, landmarkE);

			int score_width = score_blob->width();
			int score_height = score_blob->height();
			size_t count = score_width * score_height;
			size_t num_anchor = num_anchors_[key];

			//store order: h * w * num_anchor
			std::vector<anchor_box> anchors = anchors_plane(score_height, score_width, stride, anchors_fpn_[key]);

			for (size_t num = 0; num < num_anchor; num++)
			{
				for (size_t j = 0; j < count; j++)
				{
					float conf = score[j + count * num];
					if (conf <= threshold)
					{
						continue;
					}

					float dx = bbox_delta[j + count * (0 + num * 4)];
					float dy = bbox_delta[j + count * (1 + num * 4)];
					float dw = bbox_delta[j + count * (2 + num * 4)];
					float dh = bbox_delta[j + count * (3 + num * 4)];
					auto regress = std::vector<float>{ dx, dy, dw, dh };

					// regression face bbox
					anchor_box rect = bbox_pred(anchors[j + count * num], regress);
					//Out of bounds
					clip_box(rect, ws, hs);

					face_pts pts;
					for (size_t k = 0; k < 5; k++)
					{
						pts.x[k] = landmark_delta[j + count * (num * 10 + k * 2)];
						pts.y[k] = landmark_delta[j + count * (num * 10 + k * 2 + 1)];
					}
					//regression facial landmark
					face_pts landmarks = landmark_pred(anchors[j + count * num], pts);

					face_info_internal tmp;
					tmp.score = conf;
					tmp.rect = rect;
					tmp.pts = landmarks;
					face_infos.push_back(tmp);
				}
			}
		}

		face_infos = nms(face_infos, nms_threshold_);

		return face_infos;
	}
}


