#include "material_code_impl.hpp"
#include "material_code_internal.hpp"
#include "box_info_impl.hpp"

namespace glasssix::ring
{
	material_code_impl::material_code_impl()
	{
	}

	material_code_impl::~material_code_impl()
	{
	}


	void material_code_impl::init(const exposing::param_string& model_directory, const std::int32_t factory_type, std::int32_t device)
	{
		impl_ = std::make_unique<material_code_internal>(exposing::to_narrow_string(model_directory), factory_type, device);
	}

	exposing::param_string material_code_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<box_info> material_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int border_orient, int order,
																int x, int y, int roi_width, int roi_height) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"ring internal object not initialized");

		return impl_->detect(bitmap, channels, height, width, border_orient, order, x, y, roi_width, roi_height);
	}

}
