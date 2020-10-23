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

	void retina_net_impl::init(exposing::param_string racy_path, exposing::param_string tracker_racy_path, float nms_threshold, std::int32_t device)
	{
		impl_ = std::make_unique<retina_net_internal>(racy_path, tracker_racy_path, nms_threshold, device);
	}

	void retina_net_impl::init(exposing::param_span<const exposing::param_string> phai, exposing::param_string racy_path, exposing::param_span<const exposing::param_string> tracker_phai, exposing::param_string tracker_racy_path, float nms_threshold, std::int32_t device)
	{
		std::vector<std::string> phai_internal(phai.size());
		std::vector<std::string> tracker_phai_internal(tracker_phai.size());

		std::transform(phai.begin(), phai.end(), phai_internal.begin(), &exposing::to_narrow_string);
		std::transform(tracker_phai.begin(), tracker_phai.end(), tracker_phai_internal.begin(), &exposing::to_narrow_string);
		impl_ = std::make_unique<retina_net_internal>(phai_internal, racy_path, tracker_phai_internal, tracker_racy_path, nms_threshold, device);
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
}