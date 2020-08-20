#include "face_alignment_impl.hpp"
#include "face_alignment_internal.hpp"

namespace glasssix::romancia
{
	face_alignment_impl::face_alignment_impl() :impl_(nullptr)
	{
	}
	face_alignment_impl::~face_alignment_impl()
	{
	}
	void face_alignment_impl::init(std::int32_t device)
	{
		if (impl_)
		{
			delete impl_;
			impl_ = nullptr;
		}
		impl_ = new face_alignment_internal(device);
	}
	exposing::param_string face_alignment_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}
	exposing::param_vector<std::uint8_t> face_alignment_impl::get(exposing::param_span<std::uint8_t> gray_bitmap, std::int32_t height, std::int32_t width, exposing::param_vector<exposing::param_vector<std::int32_t>> bboxes, exposing::param_vector<exposing::param_vector<std::int32_t>> landmarks)
	{
		return impl_->align(gray_bitmap, height, width, bboxes, landmarks);
	}
}