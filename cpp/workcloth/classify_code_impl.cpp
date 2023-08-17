#include "classify_code_impl.hpp"
#include "classify_code_internal.hpp"
#include "box_info_impl.hpp"
#include <map>
#include <utility>

namespace glasssix::workcloth
{
	classify_code_impl::classify_code_impl() = default;

	classify_code_impl::~classify_code_impl() = default;


	void classify_code_impl::init(const exposing::param_string& model_directory, std::int32_t device)
	{
		impl_ = std::make_unique<classify_code_internal>(exposing::to_narrow_string(model_directory), device);
	}

	exposing::param_string classify_code_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<workcloth::box_info> classify_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height, int color_index,
		const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"workcloth internal object not initialized");

		std::map<std::string, float> param_map;

		for (auto it : param_map_abi) {
			param_map.insert(std::make_pair(it.key(), it.value()));
		}

		return impl_->detect(std::move(bitmap), channels, height, width, roi_x, roi_y, roi_width, roi_height, color_index, param_map);
	}

}
