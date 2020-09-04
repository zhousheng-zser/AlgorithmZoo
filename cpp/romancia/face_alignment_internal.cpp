#include "face_alignment_internal.hpp"

#include <abi/param_span.hpp>
#include <memory>
#include <cmath>
#include <climits>

#include <Primitives/tensor_conversions.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_rotate.hpp>
#include <Excalibur/operation_equalize_hist.hpp>
#include <Excalibur/operation_merge_channel.hpp>
#include <Excalibur/operation_resize.hpp>

using glasssix::excalibur::rectangle;
using glasssix::excalibur::point;

namespace glasssix::romancia
{
	class face_alignment_internal::impl
	{
	public:
		impl() : impl{ -1 }
		{
		}

		impl(std::int32_t device) : device_{ device }
		{
		}

		~impl() = default;

		exposing::param_vector<exposing::param_vector<std::uint8_t>> align(exposing::param_span<std::uint8_t>& gray_bitmap, std::int32_t height, std::int32_t width, 
			exposing::param_vector<longinus::face_info>& faces)
		{
			init_cache(gray_bitmap, height, width);

			std::shared_ptr<memory::tensor<unsigned char>> ROI, rotated_ROI, final_mat, final_mat_gray, color_img, resized_color_img;
			std::vector<std::shared_ptr<memory::tensor<unsigned char>>> src_vector;
			auto res = exposing::make_param_vector<std::uint8_t, 2>();
			
			if (device_ < 0)
			{
				for (size_t i = 0; i < faces.size(); i++)
				{
					src_vector.clear();
					rectangle<int> MarginRect = rectangle<int>(faces[i].x() - faces[i].width() * 0.2,
						faces[i].y() - faces[i].height() * 0.2,
						faces[i].height() * 1.4f,
						faces[i].width() * 1.4f);

					excalibur::safty_cut_cpu(cache_, ROI, &MarginRect);

					point<float> ldmk5[5];
					auto pts = faces[i].pts();
					for (size_t j = 0; j < pts.size(); j++)
					{
						ldmk5[j] = point<float>(pts[j].key() - MarginRect.x, pts[j].value() - MarginRect.y);
					}
					point<float> center_eye = point<float>((ldmk5[0].x + ldmk5[1].x) / 2, (ldmk5[0].y + ldmk5[1].y) / 2);
					point<float> center_mouth = point<float>((ldmk5[3].x + ldmk5[4].x) / 2, (ldmk5[3].y + ldmk5[4].y) / 2);
					point<float> center = point<float>((center_eye.x + center_mouth.x) / 2, (center_eye.y + center_mouth.y) / 2);
					double tan = (center_eye.x - center_mouth.x) / (center_eye.y - center_mouth.y);
					double arctan = atan(tan) * 180 / 3.1415926;

					excalibur::rotate_with_points_cpu(ROI, rotated_ROI, center, -1 * arctan);

					double distance = std::sqrt((center_eye.x - center_mouth.x) * (center_eye.x - center_mouth.x) + (center_eye.y - center_mouth.y) * (center_eye.y - center_mouth.y));

					if (distance < std::numeric_limits<double>::epsilon())
					{
						LOG(FATAL) << "Illegal distance.";
					}

					double cos = (center_mouth.y - center_eye.y) / distance;
					double sin = (center_mouth.x - center_eye.x) / distance;
					point<float> new_center_eye = point<float>(center_eye.x + (float)(sin * distance / 2), (float)(center_eye.y - (1 - cos) * distance / 2));
					point<float> new_center_mouth = point<float>(center_mouth.x - (float)(sin * distance / 2), (float)(center_mouth.y + (1 - cos) * distance / 2));
					rectangle<float> final_rect = rectangle<float>(new_center_eye.x - distance,
						new_center_eye.y - distance / 2,
						distance * 2, distance * 2);
					excalibur::safty_cut_cpu(rotated_ROI, final_mat, &final_rect);
					excalibur::equalize_hist_cpu(final_mat, final_mat);

					for (size_t k = 0; k < 3; k++)
					{
						src_vector.push_back(final_mat);
					}

					excalibur::merge_channel_cpu(src_vector, color_img);
					excalibur::resize_cpu(color_img, resized_color_img, 128, 128);

					auto temp_vec = exposing::make_param_vector<std::uint8_t>();
					
					auto* ptr = resized_color_img->cpu_data();
					for (size_t k = 0; k < resized_color_img->count(); k++)
					{
						temp_vec.push_back(ptr[k]);
					}

					res.push_back(temp_vec);
				}
			}
			else
			{
				NOT_IMPLEMENTED;
			}

			return res;
		}

		static std::string version()
		{
			return "1.0.0";
		}
	private:
		void init_cache(exposing::param_span<std::uint8_t> &gray_bitmap, std::int32_t height, std::int32_t width)
		{
			if (cache_ == nullptr || cache_->height() != height  || cache_->width() != width)
			{
				cache_ = std::make_shared<memory::tensor<std::uint8_t>>(std::vector<int>{ static_cast<int>(1), static_cast<int>(1), height, width }, device_, memory::NCHW, &memory::pool_allocator_default<std::uint8_t>::get());
			}

			if (device_ > 0)
			{
#ifdef USE_CUDA
				cudaMemcpy(cache_->mutable_gpu_data(), gray_bitmap, gray_bitmap + height * width, cudaMemcpyHostToDevice);
#else
				NO_GPU;
#endif
			}
			else
				std::copy(gray_bitmap.begin(), gray_bitmap.end(), cache_->mutable_cpu_data());
		}

		int device_;
		std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
	};

	face_alignment_internal::face_alignment_internal() : impl_{ new impl }
	{
	}

	face_alignment_internal::face_alignment_internal(int device) : impl_{ new impl{ device } }
	{
	}

	face_alignment_internal::~face_alignment_internal()
	{
		if (impl_ != nullptr)
		{
			delete impl_;
			impl_ = nullptr;
		}
	}

	exposing::param_vector<exposing::param_vector<std::uint8_t>> face_alignment_internal::align(exposing::param_span<std::uint8_t>& gray_bitmap, std::int32_t height, std::int32_t width,
		exposing::param_vector<longinus::face_info>& faces) const
	{
		return impl_->align(gray_bitmap, height, width, faces);
	}

	std::string face_alignment_internal::version()
	{
		return impl::version();
	}
}