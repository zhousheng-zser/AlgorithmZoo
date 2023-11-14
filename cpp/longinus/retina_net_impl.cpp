#include "retina_net_impl.hpp"
#include "retina_net_internal.hpp"
#include "face_info_impl.hpp"

namespace glasssix::longinus
{
	retina_net_impl::retina_net_impl()
	{
	}

	retina_net_impl::~retina_net_impl()
	{
	}

	void retina_net_impl::init(const exposing::param_string& models_directory, int model_type, int instance, float nms_threshold, std::int32_t device)
	{
		impl_ = std::make_unique<retina_net_internal>(exposing::to_narrow_string(models_directory), model_type, instance, nms_threshold, device );
	}

	exposing::param_string retina_net_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<longinus::face_info> retina_net_impl::detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t min_size, float threshold, std::int32_t order, bool do_attributing) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"longinus internal object not initialized");

		return impl_->detect(bitmap, channels, height, width, min_size, threshold, order, do_attributing);
	}
    
	face_info retina_net_impl::single_trace(face_info face, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"longinus internal object not initialized");

		return impl_->single_trace(face, bitmap, channels, height, width,  order);
	}

	exposing::param_vector<exposing::param_vector<std::uint8_t>> retina_net_impl::center_scale_align(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, float scale, std::int32_t order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"romancia internal object not initialized");

		return impl_->center_scale_align(bitmap, channels, height, width, scale, order);
	}
}