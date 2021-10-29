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

	void anti_spoofing_impl::init(const exposing::param_string& FASMV2_racy_path, const exposing::param_string& land65_racy_path, std::int32_t device, bool use_int8)
	{
		impl_ = std::make_unique<anti_spoofing_internal>(exposing::to_narrow_string(FASMV2_racy_path), exposing::to_narrow_string(land65_racy_path), device, use_int8);
	}

	void anti_spoofing_impl::init(exposing::param_span<const exposing::param_string> FASMV2_phai, const exposing::param_string& FASMV2_racy_path,
		exposing::param_span<const exposing::param_string> land65_phai, const exposing::param_string& land65_racy_path, std::int32_t device)
	{
		std::vector<std::string> FASMV2_phai_internal(FASMV2_phai.size());
		std::vector<std::string> land65_phai_internal(land65_phai.size());

		std::transform(FASMV2_phai.begin(), FASMV2_phai.end(), FASMV2_phai_internal.begin(), &exposing::to_narrow_string);
		impl_ = std::make_unique<anti_spoofing_internal>(FASMV2_phai_internal, exposing::to_narrow_string(FASMV2_racy_path), 
			land65_phai_internal, exposing::to_narrow_string(land65_racy_path), device);
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
	bool anti_spoofing_impl::presentation_attack_detect(int action_cmd, const longinus::face_info& face, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"damocles internal object not initialized");

		return impl_->presentation_attack_detect(action_cmd, face, bitmap, channels, height, width, order);
	}
}
