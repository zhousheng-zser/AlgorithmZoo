#include "anti_spoofing_internal.hpp"
#include "hardcode.hpp"

#include <algorithm>

#include <Excalibur/pipeline.hpp>
#include <Primitives/pool_allocator.hpp>
#include <Primitives/tensor_conversions.hpp>
#include <Excalibur/operation_safty_cut.hpp>
#include <Excalibur/operation_resize.hpp>

#ifdef USE_RKNNAPI
#include "RKNNWrapper/rknn_wrapper.hpp"
#endif

#ifdef USE_CUDA
#include <cuda_runtime_api.h>
#endif

namespace glasssix::damocles
{
	namespace
	{
		constexpr std::size_t result_size = 3;
		constexpr std::size_t forward_input_width = 80;
		constexpr std::size_t forward_input_height = 80;
		constexpr std::size_t forward_input_channels = 3;
		constexpr std::size_t forward_input_bytes = forward_input_channels * forward_input_width * forward_input_height;
	}

	class anti_spoofing_internal::impl
	{
	public:
		impl(std::string_view racy_path, int device, bool use_int8) : impl{ hardcode::get_model_params("FASMV2", use_int8), racy_path, device }
		{
		}

		impl(const std::vector<std::string>& phai, std::string_view racy_path, int device) : device_{ device }, fasmv2_{ phai, std::string{ racy_path }, device }
		{
		}

		std::vector<std::vector<float>> spoofing_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
		{
			if (bitmap.empty())
			{
				throw exposing::abi_invalid_argument("current frame is empty");
			}

			init_cache(bitmap, channels, height, width, order);

			auto boxes = calculate_box(faces, width, height, 2.7f);
			std::shared_ptr<memory::tensor<uint8_t>> crop_faces(new memory::tensor<uint8_t>(std::vector<int>{static_cast<int>(boxes.size()), forward_input_channels, forward_input_height, forward_input_width}, -1, memory::NCHW, nullptr));
			
			uint8_t* ptr = crop_faces->mutable_cpu_data();

			for (size_t i = 0; i < boxes.size(); i++)
			{
				std::shared_ptr<memory::tensor<uint8_t>> crop_face;
				excalibur::safty_cut_cpu(cache_, crop_face, &boxes[i]);
				excalibur::resize_cpu(crop_face, crop_face, forward_input_height, forward_input_width);
				const uint8_t* crop_data = crop_face->cpu_data();
				std::copy(crop_data, crop_data + forward_input_bytes, ptr);
				ptr += forward_input_bytes;
			}

			std::vector<std::vector<float>> result;
#ifdef USE_RKNNAPI
			auto network_result = fasmv2_.forward(crop_faces);
			if (auto iter = network_result.find("softmax"); iter != network_result.end())
#else
			auto network_result = fasmv2_.forward(crop_faces | memory::tensor_convert_to<float>);
			if (auto iter = network_result.find("softmax"); iter != network_result.end())
#endif
			{
				auto iter_softmax = iter->second->cpu_data();

				for (std::size_t i = 0; i < faces.size(); i++)
				{
					std::vector<float> result_tmp(result_size);

					std::copy(iter_softmax, iter_softmax + result_size, result_tmp.data());
					iter_softmax += result_size;
					result.emplace_back(result_tmp);
				}
			}

			return result;
		}

		static std::string version()
		{
			return "1.0.0";
		}
	private:
		void init_cache(exposing::param_span<std::uint8_t>& bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order)
		{
			if (cache_ == nullptr || cache_->channels() != channels || cache_->height() != height || cache_->width() != width || cache_->order() != order)
			{
				std::vector<int> shape;
				if (order == memory::NCHW)
					shape = { static_cast<int>(1), channels, height, width };
				else if (order == memory::NHWC)
					shape = { static_cast<int>(1), height, width, channels };
				else
					NOT_IMPLEMENTED;

				cache_ = std::make_shared<memory::tensor<std::uint8_t>>(shape, -1, (memory::orderType)order/*, &memory::pool_allocator_default<std::uint8_t>::get()*/);
			}

			if (cache_->device() > 0)
			{
#ifdef USE_CUDA
				cudaMemcpy(cache_->mutable_gpu_data(), bitmap, channels * height * width, cudaMemcpyHostToDevice);
#else
				NO_GPU;
#endif
			}
			else
				std::copy(bitmap.begin(), bitmap.end(), cache_->mutable_cpu_data());

			if (order == memory::NHWC)
				cache_->convert_order();
		}

		std::vector<excalibur::rectangle<float>> calculate_box(const exposing::param_vector<longinus::face_info>& faces, int w, int h, float s)
		{
			std::vector<excalibur::rectangle<float>> boxes;
			for (size_t i = 0; i < faces.size(); i++)
			{
				int x = faces[i].x();
				int y = faces[i].y();
				int box_width = faces[i].width();
				int box_height = faces[i].height();

				int shift_x = 0;//static_cast<int>(box_width * config.shift_x);
				int shift_y = 0;//static_cast<int>(box_height * config.shift_y);

				float scale = std::min(
					s,
					std::min((w - 1) / (float)box_width, (h - 1) / (float)box_height)
				);

				int box_center_x = box_width / 2 + x;
				int box_center_y = box_height / 2 + y;

				int new_width = static_cast<int>(box_width * scale);
				int new_height = static_cast<int>(box_height * scale);

				int left_top_x = box_center_x - new_width / 2 + shift_x;
				int left_top_y = box_center_y - new_height / 2 + shift_y;
				int right_bottom_x = box_center_x + new_width / 2 + shift_x;
				int right_bottom_y = box_center_y + new_height / 2 + shift_y;

				if (left_top_x < 0) {
					right_bottom_x -= left_top_x;
					left_top_x = 0;
				}

				if (left_top_y < 0) {
					right_bottom_y -= left_top_y;
					left_top_y = 0;
				}

				if (right_bottom_x >= w) {
					int s = right_bottom_x - w + 1;
					left_top_x -= s;
					right_bottom_x -= s;
				}

				if (right_bottom_y >= h) {
					int s = right_bottom_y - h + 1;
					left_top_y -= s;
					right_bottom_y -= s;
				}

				boxes.push_back(excalibur::rectangle<float>(left_top_x, left_top_y, new_height, new_width));
			}

			return boxes;
		}

		int device_;
#ifdef USE_RKNNAPI
		rknnwrapper::rknn_wrapper fasmv2_;
#else
		excalibur::pipeline<float> fasmv2_;
#endif
		std::shared_ptr<memory::tensor<std::uint8_t>> cache_;
	};

	anti_spoofing_internal::anti_spoofing_internal(std::string_view racy_path, int device, bool use_int8) : impl_{ std::make_unique<impl>(racy_path, device, use_int8) }
	{
	}

	anti_spoofing_internal::anti_spoofing_internal(const std::vector<std::string>& phai, std::string_view racy_path, int device) : impl_{ std::make_unique<impl>(phai, racy_path, device) }
	{
	}

	anti_spoofing_internal::~anti_spoofing_internal()
	{
	}

	std::vector<std::vector<float>> anti_spoofing_internal::spoofing_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
	{
		return impl_->spoofing_detect(faces, bitmap, channels, height, width, order);
	}

	std::string anti_spoofing_internal::version()
	{
		return impl::version();
	}
}
