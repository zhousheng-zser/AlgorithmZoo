#include "detect_code_impl.hpp"
#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"
#include <map>
#include <utility>

namespace glasssix::pump_light
{
	detect_code_impl::detect_code_impl() = default;

	detect_code_impl::~detect_code_impl() = default;


	void detect_code_impl::init(const exposing::param_string& model_directory, std::int32_t device, std::int32_t model_type)
	{
		impl_ = std::make_unique<detect_code_internal>(exposing::to_narrow_string(model_directory), device, model_type);
	}

	exposing::param_string detect_code_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	pump_light::box_info detect_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width,
		const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"pump_light internal object not initialized");
			
		std::map<std::string, float> param_map;

		for (auto it : param_map_abi) {
			param_map.insert(std::make_pair(it.key(), it.value()));
		}

		return impl_->detect(std::move(bitmap), channels, height, width, param_map);
	}

}
