#include "anti_spoofing_impl.hpp"
#include "anti_spoofing_internal.hpp"

namespace glasssix::damocles
{
	anti_spoofing_impl::anti_spoofing_impl()
	{
	}

	anti_spoofing_impl::~anti_spoofing_impl()
	{
	}

	void anti_spoofing_impl::init(const exposing::param_string& racy_path, std::int32_t device, bool use_int8)
	{
		impl_ = std::make_unique<anti_spoofing_internal>(exposing::to_narrow_string(racy_path), device, use_int8);
	}

	void anti_spoofing_impl::init(exposing::param_span<const exposing::param_string> phai, const exposing::param_string& racy_path, std::int32_t device)
	{
		std::vector<std::string> phai_internal(phai.size());

		std::transform(phai.begin(), phai.end(), phai_internal.begin(), &exposing::to_narrow_string);
		impl_ = std::make_unique<anti_spoofing_internal>(phai_internal, exposing::to_narrow_string(racy_path), device);
	}

	exposing::param_string anti_spoofing_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<exposing::param_vector<float>> anti_spoofing_impl::spoofing_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"damocles internal object not initialized");

		auto native_result = impl_->spoofing_detect(faces, bitmap, channels, height, width, order);
		auto result = exposing::make_param_vector<float, 2>();
		for (const auto& item : native_result)
		{
			result.push_back(exposing::make_param_vector<float>(item));
		}

		return result;
	}
}
