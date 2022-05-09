#include "yolov5Deepsort_net_impl.hpp"
#include "yolov5Deepsort_net_internal.hpp"

namespace glasssix::pan
{
	yolov5Deepsort_net_impl::yolov5Deepsort_net_impl()
	{
	}

	yolov5Deepsort_net_impl::~yolov5Deepsort_net_impl()
	{
	}

	void yolov5Deepsort_net_impl::init(exposing::param_string yolov5m_racy_path, exposing::param_string deepsort_racy_path, std::int32_t device)
	{
		impl_ = std::make_unique<yolov5Deepsort_net_internal>(exposing::to_narrow_string(yolov5m_racy_path), exposing::to_narrow_string(deepsort_racy_path), device);
	}

	void yolov5Deepsort_net_impl::init(exposing::param_string yolov5m_phai, exposing::param_string yolov5m_racy_path, exposing::param_string deepsort_phai, exposing::param_string deepsort_racy_path, std::int32_t device)
	{
		// use this
		impl_ = std::make_unique<yolov5Deepsort_net_internal>(exposing::to_narrow_string(yolov5m_phai), exposing::to_narrow_string(yolov5m_racy_path), exposing::to_narrow_string(deepsort_phai), exposing::to_narrow_string(deepsort_racy_path), device);
	}

	void yolov5Deepsort_net_impl::init(exposing::param_span<const exposing::param_string> yolov5m_phai, exposing::param_string yolov5m_racy_path, exposing::param_span<const exposing::param_string> deepsort_phai, exposing::param_string deepsort_racy_path,  std::int32_t device)
	{
		std::vector<std::string> yolov5m_phai_internal(yolov5m_phai.size());
		std::transform(yolov5m_phai.begin(), yolov5m_phai.end(), yolov5m_phai_internal.begin(), &exposing::to_narrow_string);
		std::vector<std::string> deepsort_phai_internal(deepsort_phai.size());
		std::transform(deepsort_phai.begin(), deepsort_phai.end(), deepsort_phai_internal.begin(), &exposing::to_narrow_string);

		impl_ = std::make_unique<yolov5Deepsort_net_internal>(yolov5m_phai_internal, exposing::to_narrow_string(yolov5m_racy_path), deepsort_phai_internal, exposing::to_narrow_string(deepsort_racy_path), device);
	}

	exposing::param_string yolov5Deepsort_net_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	pan::result_info yolov5Deepsort_net_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order)
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"pan internal object not initialized");

		return impl_->detect(bitmap, channels, height, width, order);
	}

}
