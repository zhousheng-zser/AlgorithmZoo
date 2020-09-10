#include "retina_net_impl.hpp"
#include "retina_net_internal.hpp"
#include "face_info_impl.hpp"

namespace glasssix::longinus
{
	retina_net_impl::retina_net_impl():impl_(nullptr)
	{
	}
	retina_net_impl::~retina_net_impl()
	{
		if (impl_)
		{
			delete impl_;
			impl_ = nullptr;
		}
	}
	void retina_net_impl::init(exposing::param_span<exposing::param_string> phai, exposing::param_string racy_path, float nms_threshold, std::int32_t device)
	{
		if (impl_)
		{
			delete impl_;
			impl_ = nullptr;
		}

		std::vector<std::string> phai_internal(phai.size());

		std::transform(phai.begin(), phai.end(), phai_internal.begin(), &exposing::to_narrow_string);
		impl_ = new retina_net_internal(phai_internal, racy_path, nms_threshold, device);
	}
	exposing::param_string retina_net_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}
	exposing::param_vector<longinus::face_info> retina_net_impl::get(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t min_size, float threshold, std::int32_t order) const
	{
		if (!impl_)
			throw exposing::abi_not_initialized(u8"longinus internal object not initialized");

		return impl_->detect(bitmap, channels, height, width, min_size, threshold, order);
	}
}