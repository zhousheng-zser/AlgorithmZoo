#include "retina_net_impl.hpp"
#include "retina_net_native.hpp"
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
	void retina_net_impl::init(exposing::param_string phai_path, exposing::param_string racy_path, float nms_threshold, std::int32_t device)
	{
		if (impl_)
		{
			delete impl_;
			impl_ = nullptr;
			impl_ = new retina_net_native(to_narrow_string(phai_path), to_narrow_string(racy_path), nms_threshold, device);
		}
	}
	exposing::param_string retina_net_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}
	exposing::param_vector<longinus::face_info> retina_net_impl::detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t min_size, float threshold, std::int32_t order) const
	{
		if (!impl_)
			throw exposing::abi_not_initialized(u8"native detect object not initialized");

		auto result_internal = impl_->detect(bitmap.data(), channels, height, width, min_size, threshold, order);

		auto result = exposing::make_param_vector<face_info>();
		for (auto x : result_internal)
		{
			auto face = exposing::make_as_first<face_info_impl>(x);
			result.push_back(face);
		}

		return result;
	}
}