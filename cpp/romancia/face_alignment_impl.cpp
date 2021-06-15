#include "face_alignment_impl.hpp"
#include "face_alignment_internal.hpp"

namespace glasssix::romancia
{
	face_alignment_impl::face_alignment_impl()
	{
	}

	face_alignment_impl::~face_alignment_impl()
	{
	}
	void face_alignment_impl::init(/*const exposing::param_string& mask_detector_model_path, */const exposing::param_string& antispoofing_model_path, std::int32_t device)
	{
		impl_ = std::make_unique<face_alignment_internal>(/*mask_detector_model_path, */antispoofing_model_path, device);
	}

	exposing::param_string face_alignment_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}
	exposing::param_vector<exposing::param_vector<std::uint8_t>> face_alignment_impl::align(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, const exposing::param_vector<longinus::face_info>& faces, std::int32_t order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"romancia internal object not initialized");

		return impl_->align(bitmap, channels, height, width, faces, order);
	}
	exposing::param_vector<exposing::param_vector<std::uint8_t>> face_alignment_impl::align256(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, const exposing::param_vector<longinus::face_info>& faces, std::int32_t order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"romancia internal object not initialized");

		return impl_->align256(bitmap, channels, height, width, faces, order);
	}
	exposing::param_vector<double> face_alignment_impl::blur_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"romancia internal object not initialized");

		return impl_->blur_detect(faces, bitmap, channels, height, width, order);
	}
	exposing::param_vector<bool> face_alignment_impl::antispoofing(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"romancia internal object not initialized");
		return impl_->antispoofing(faces, bitmap, channels, height, width, order);
	}
	//exposing::param_vector<bool> face_alignment_impl::mask_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
	//{
	//	if (!impl_)
	//		throw exposing::abi_invalid_operation(u8"romancia internal object not initialized");

	//	return impl_->mask_detect(faces, bitmap, channels, height, width, order);
	//}
	exposing::param_vector<double> face_alignment_impl::mask_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"romancia internal object not initialized");

		return impl_->mask_detect(faces, bitmap, channels, height, width, order);
	}
}
