#include "deepMarMobileNet_net_impl.hpp"
#include "deepMarMobileNet_net_internal.hpp"

namespace glasssix::rifleman
{
	deepMarMobileNet_net_impl::deepMarMobileNet_net_impl()
	{
	}

	deepMarMobileNet_net_impl::~deepMarMobileNet_net_impl()
	{
	}

	void deepMarMobileNet_net_impl::init(exposing::param_string deepMarMobileNet_racy_path, std::int32_t device)
	{
		impl_ = std::make_unique<deepMarMobileNet_net_internal>(exposing::to_narrow_string(deepMarMobileNet_racy_path), device);
	}

	void deepMarMobileNet_net_impl::init(exposing::param_string deepMarMobileNet_phai, exposing::param_string deepMarMobileNet_racy_path, std::int32_t device)
	{
		impl_ = std::make_unique<deepMarMobileNet_net_internal>(exposing::to_narrow_string(deepMarMobileNet_phai), exposing::to_narrow_string(deepMarMobileNet_racy_path), device);
	}

	void deepMarMobileNet_net_impl::init(exposing::param_span<const exposing::param_string> deepMarMobileNet_phai, exposing::param_string deepMarMobileNet_racy_path, std::int32_t device)
	{
		std::vector<std::string> yolov5m_phai_internal(deepMarMobileNet_phai.size());
		std::transform(deepMarMobileNet_phai.begin(), deepMarMobileNet_phai.end(), yolov5m_phai_internal.begin(), &exposing::to_narrow_string);

		impl_ = std::make_unique<deepMarMobileNet_net_internal>(yolov5m_phai_internal, exposing::to_narrow_string(deepMarMobileNet_racy_path), device);
	}

	exposing::param_string deepMarMobileNet_net_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<exposing::param_vector<exposing::param_pair<float, exposing::param_string>>>  deepMarMobileNet_net_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"rifleman internal object not initialized");

		return impl_->detect(bitmap, channels, height, width, order);
	}

}
