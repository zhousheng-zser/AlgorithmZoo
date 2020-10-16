#pragma once
#ifndef _OPERATION_SAFTY_CUT_HPP_
#define _OPERATION_SAFTY_CUT_HPP_
#include "operation_make_border.hpp"
#include "operation_cut_border.hpp"
#include "operation_rotate.hpp"
#include <algorithm>

namespace glasssix
{
	namespace excalibur
	{
		template <typename Dtype>
		class rectangle
		{
		public:
			Dtype x;
			Dtype y;
			Dtype h;
			Dtype w;

			rectangle()
			{
				this->x = Dtype(0);
				this->y = Dtype(0);
				this->h = Dtype(0);
				this->w = Dtype(0);
			}

			rectangle(Dtype x, Dtype y, Dtype h, Dtype w)
			{
				CHECK_GE(h, 0);
				CHECK_GE(w, 0);
				this->x = x;
				this->y = y;
				this->h = h;
				this->w = w;
			}

			rectangle(point<Dtype> top_left, point<Dtype> bottom_right)
			{
				this->x = top_left.x;
				this->y = top_left.y;
				this->h = bottom_right.y - top_left.y;
				this->w = bottom_right.x - top_left.x;
				CHECK_GE(h, 0);
				CHECK_GE(w, 0);
			}

			rectangle& operator=(const rectangle& r)
			{
				if (this == &r)
				{
					return *this;
				}
				x = r.x;
				y = r.y;
				h = r.h;
				w = r.w;
				return *this;
			}

			rectangle(const rectangle& r)
			{
				x = r.x;
				y = r.y;
				h = r.h;
				w = r.w;
			}

			Dtype IoU(const rectangle r)
			{
				return Dtype(0);// Todo
			}

			point<Dtype> center()
			{
				return point<Dtype>(Dtype(x + w * 0.5f), Dtype(y + h * 0.5f));
			}

			rectangle& operator &= (const rectangle& b)
			{
				Dtype x1 = std::max(x, b.x);
				Dtype y1 = std::max(y, b.y);
				w = std::min(x + w, b.x + b.w) - x1;
				h = std::min(y + h, b.y + b.h) - y1;
				x = x1;
				y = y1;
				if (w <= 0 || h <= 0)
					x = y = h = w = 0;
				return *this;
			}
		};

		/// <summary>
			/// get ROI(region of interest) from image. Similar to roi_cpu, but more safe, if rectangle exceeds border, fill 0 instead. 
			/// </summary>
			/// <param name="src">original memory::tensor</param>
			/// <param name="dst">ROI memory::tensor</param>
			/// <param name="rect">region of interest</param>
		template <typename Dtype, typename Rtype>
		static void safty_cut_cpu(const std::shared_ptr<memory::tensor<Dtype>>& src, std::shared_ptr<memory::tensor<Dtype>>& dst, rectangle<Rtype>* rect)
		{
			if (src->device() >= 0)
			{
				LOG(ERROR) << "device wrong, invoke function xxx_gpu() instead!!!";
				return;
			}

			if (rect->x >= 0 && rect->y >= 0 && (rect->x + rect->w <= src->width()) && (rect->y + rect->h <= src->height()))
			{
				cut_border_cpu(src, dst, rect->y, (src->height() - rect->y - rect->h), rect->x, (src->width() - rect->x - rect->w));
			}
			else
			{
				int top = std::max(int(0), int(-1 * rect->y));
				int bottom = std::max(int(rect->y + rect->h - src->height()), int(0));
				int left = std::max(int(0), int(-1 * rect->x));
				int right = std::max(int(rect->x + rect->w - src->width()), int(0));
				std::shared_ptr<memory::tensor<Dtype>> temp;
				if (src->order() == memory::NCHW)
				{
					temp.reset(new memory::tensor<Dtype>(
						std::vector<int>{src->num(), src->channels(), src->height() + top + bottom, src->width() + left + right},
						src->device(), src->order(), src->allocator()));
				}
				else if (src->order() == memory::NHWC)
				{
					temp.reset(new memory::tensor<Dtype>(
						std::vector<int>{src->num(), src->height() + top + bottom, src->width() + left + right, src->channels()},
						src->device(), src->order(), src->allocator()));
				}
				else
				{
					NOT_IMPLEMENTED;
				}

				make_border(src, temp, top, bottom, left, right, border_constant);
				cut_border_cpu(temp, dst, rect->y + top, temp->height() - rect->y - rect->h - top, rect->x + left, temp->width() - rect->x - rect->w - left);
			}
		}
	}
}
#endif