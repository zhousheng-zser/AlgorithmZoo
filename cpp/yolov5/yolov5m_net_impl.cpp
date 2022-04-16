#include "yolov5m_net_impl.hpp"
#include "yolov5m_net_internal.hpp"

namespace glasssix::yolov5
{
	yolov5m_net_impl::yolov5m_net_impl()
	{
	}

	yolov5m_net_impl::~yolov5m_net_impl()
	{
	}

	void yolov5m_net_impl::init(exposing::param_string yolov5m_racy_path, std::int32_t device)
	{
		impl_ = std::make_unique<yolov5m_net_internal>(exposing::to_narrow_string(yolov5m_racy_path), device);
	}

	void yolov5m_net_impl::init(exposing::param_string yolov5m_phai, exposing::param_string yolov5m_racy_path, std::int32_t device)
	{
		impl_ = std::make_unique<yolov5m_net_internal>(exposing::to_narrow_string(yolov5m_phai), exposing::to_narrow_string(yolov5m_racy_path), device);
	}

	void yolov5m_net_impl::init(exposing::param_span<const exposing::param_string> yolov5m_phai, exposing::param_string yolov5m_racy_path, std::int32_t device)
	{
		std::vector<std::string> yolov5m_phai_internal(yolov5m_phai.size());
		std::transform(yolov5m_phai.begin(), yolov5m_phai.end(), yolov5m_phai_internal.begin(), &exposing::to_narrow_string);

		impl_ = std::make_unique<yolov5m_net_internal>(yolov5m_phai_internal, exposing::to_narrow_string(yolov5m_racy_path), device);
	}

	exposing::param_string yolov5m_net_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	yolov5::result_info yolov5m_net_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"yolov5 internal object not initialized");

		return impl_->detect(bitmap, channels, height, width, order);
	}

}
