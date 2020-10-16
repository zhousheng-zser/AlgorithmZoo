#include "face_alignment_impl.hpp"
#include "face_alignment_internal.hpp"

namespace glasssix::romancia
{
	face_alignment_impl::face_alignment_impl() :impl_(nullptr)
	{
	}
	face_alignment_impl::~face_alignment_impl()
	{
		if (impl_)
		{
			delete impl_;
			impl_ = nullptr;
		}
	}
	void face_alignment_impl::init(/*exposing::param_string mask_detector_model_path, */exposing::param_string antispoofing_model_path, std::int32_t device)
	{
		if (impl_)
		{
			delete impl_;
			impl_ = nullptr;
		}
		impl_ = new face_alignment_internal(/*mask_detector_model_path, */antispoofing_model_path, device);
	}
	exposing::param_string face_alignment_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}
	exposing::param_vector<exposing::param_vector<std::uint8_t>> face_alignment_impl::get(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, exposing::param_vector<longinus::face_info> faces, std::int32_t order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"romancia internal object not initialized");

		return impl_->align(bitmap, channels, height, width, faces, order);
	}
	double face_alignment_impl::blur_detect(longinus::face_info face, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"longinus internal object not initialized");

		return impl_->blur_detect(face, bitmap, channels, height, width, order);
	}
	double face_alignment_impl::antispoofing(longinus::face_info face, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"romancia internal object not initialized");
		return impl_->antispoofing(face, bitmap, channels, height, width, order);
	}
	double face_alignment_impl::mask_detect(longinus::face_info face, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"longinus internal object not initialized");

		return impl_->mask_detect(face, bitmap, channels, height, width, order);
	}
}