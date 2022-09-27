#include "material_code_impl.hpp"
#include "material_code_internal.hpp"
#include "box_info_impl.hpp"
#include <map>

namespace glasssix::heimdall
{
	material_code_impl::material_code_impl()
	{
	}

	material_code_impl::~material_code_impl()
	{
	}

	void material_code_impl::init(const exposing::param_string& model_directory, const std::int32_t factory_type, std::int32_t device, const exposing::param_hash_map<exposing::param_string, float>& param_map_abi)
	{
		std::map<std::string, float> param_map;

		for (auto it : param_map_abi) {
			param_map.insert(std::make_pair(it.key(), it.value()));
		}

		impl_ = std::make_unique<material_code_internal>(exposing::to_narrow_string(model_directory), factory_type, device, param_map);
	}

	exposing::param_string material_code_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<box_info> material_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int top_five, int order, int x, int y, int roi_width, int roi_height) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"heimdall internal object not initialized");

		return impl_->detect(bitmap, channels, height, width, top_five, order, x, y, roi_width, roi_height);
	}
}
