#include "ocr_code_impl.hpp"
#include "ocr_code_internal.hpp"
#include "box_info_impl.hpp"
#include <map>

namespace glasssix::needledash
{
	ocr_code_impl::ocr_code_impl()
	{
	}

	ocr_code_impl::~ocr_code_impl()
	{
	}


	void ocr_code_impl::init(const exposing::param_string& model_directory, std::int32_t device)
	{
		impl_ = std::make_unique<ocr_code_internal>(exposing::to_narrow_string(model_directory), device);
	}

	exposing::param_string ocr_code_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_vector<needledash::box_info> ocr_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int type,
		int x, int y, int roi_width, int roi_height, const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"needledash internal object not initialized");

		std::map<std::string, float> param_map;

		for (auto it : param_map_abi) {
			param_map.insert(std::make_pair(it.key(), it.value()));
		}

		return impl_->detect(bitmap, channels, height, width, type, x, y, roi_width, roi_height, param_map);
	}

}
