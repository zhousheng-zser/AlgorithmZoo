#include "yolo_net_impl.hpp"
#include "yolo_net_internal.hpp"
#include "hat_info_impl.hpp"

namespace glasssix::gungnir
{
	yolo_net_impl::yolo_net_impl()
	{
	}

	yolo_net_impl::~yolo_net_impl()
	{
	}

	void yolo_net_impl::init(const exposing::param_string& model_directory, std::int32_t device)
	{
		impl_ = std::make_unique<yolo_net_internal>(exposing::to_narrow_string(model_directory), device);
	}

	exposing::param_string yolo_net_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<hat_info> yolo_net_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"gungnir internal object not initialized");

		return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height);
	}
}
