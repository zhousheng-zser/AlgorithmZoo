#include "face_alignment_impl.hpp"
#include "face_alignment_native.hpp"

namespace glasssix::romancia
{
	face_alignment_impl::face_alignment_impl() :face_alignment_impl{-1}
	{
	}
	face_alignment_impl::face_alignment_impl(int device):impl_(new face_alignment_native(-1))
	{
	}
	face_alignment_impl::~face_alignment_impl()
	{
	}
	exposing::param_string face_alignment_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}
	exposing::param_vector<std::uint8_t> face_alignment_impl::get(exposing::param_span<std::uint8_t> gray_bitmap, std::int32_t height, std::int32_t width, exposing::param_vector<exposing::param_vector<std::int32_t>> bboxes, exposing::param_vector<exposing::param_vector<std::int32_t>> landmarks)
	{
		return impl_->get(gray_bitmap, height, width, bboxes, landmarks);
	}
}