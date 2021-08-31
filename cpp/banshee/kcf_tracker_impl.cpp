#include "kcf_tracker_impl.hpp"
#include "kcf_tracker_internal.hpp"
#include "track_info_impl.hpp"

namespace glasssix::banshee
{
	kcf_tracker_impl::kcf_tracker_impl()
	{
	}

	kcf_tracker_impl::~kcf_tracker_impl()
	{
	}

	void kcf_tracker_impl::init(exposing::param_span<std::uint8_t> bitmap, std::int32_t width, std::int32_t height, std::int32_t x, std::int32_t y, std::int32_t roi_width, std::int32_t roi_height)
	{
		impl_ = std::make_unique<kcf_tracker_internal>(bitmap, width, height, x, y, roi_width, roi_height);
	}

	exposing::param_string kcf_tracker_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	track_info kcf_tracker_impl::update(exposing::param_span<std::uint8_t> bitmap, std::int32_t width, std::int32_t height) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"banshee internal object not initialized");

		return impl_->update(bitmap, width, height);
	}
}
