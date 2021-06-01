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

	void yolo_net_impl::init(const exposing::param_string& racy_path, std::int32_t device)
	{
		impl_ = std::make_unique<yolo_net_internal>(exposing::to_narrow_string(racy_path), device);
	}

	void yolo_net_impl::init(exposing::param_span<const exposing::param_string> phai, const exposing::param_string& racy_path, std::int32_t device)
	{
		std::vector<std::string> phai_internal(phai.size());

		std::transform(phai.begin(), phai.end(), phai_internal.begin(), &exposing::to_narrow_string);
		impl_ = std::make_unique<yolo_net_internal>(phai_internal, exposing::to_narrow_string(racy_path), device);
	}

	exposing::param_string yolo_net_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<hat_info> yolo_net_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"gungnir internal object not initialized");

		return impl_->detect(bitmap, channels, height, width, order);
	}
}
