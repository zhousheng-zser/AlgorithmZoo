#include "classify_code_impl.hpp"
#include "classify_code_internal.hpp"
#include "box_info_impl.hpp"
#include <map>
#include <utility>

namespace glasssix::workcloth
{
	classify_code_impl::classify_code_impl() = default;

	classify_code_impl::~classify_code_impl() = default;

	inline void change_color_hsv_cfg(std::unordered_map<int, std::vector<cv::Scalar>> &color_hsv_cfg , int id, const exposing::param_vector<int>& temp) {
		for (int i = 0; i < temp.size(); i += 3)
			color_hsv_cfg[id].push_back(cv::Scalar{temp[i],temp[i+1] ,temp[i+2]});
	}

	void classify_code_impl::init(const exposing::param_string& model_directory, std::int32_t device)
	{
		impl_ = std::make_unique<classify_code_internal>(exposing::to_narrow_string(model_directory), device);
	}

	exposing::param_string classify_code_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}
	
	exposing::param_vector<workcloth::box_info> classify_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height,
		const exposing::param_hash_map<exposing::param_string, float>& param_map_abi, 
		const exposing::param_hash_map<exposing::param_string, exposing::param_vector<int>>& color_hsv_cfg_abi) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"workcloth internal object not initialized");

		std::map<std::string, float> param_map;

		for (auto it : param_map_abi) {
			param_map.insert(std::make_pair(it.key(), it.value()));
		}
		std::unordered_map<int, std::vector<cv::Scalar>> color_hsv_cfg;
		change_color_hsv_cfg(color_hsv_cfg, 0, color_hsv_cfg_abi.get_value("black"));
		change_color_hsv_cfg(color_hsv_cfg, 1, color_hsv_cfg_abi.get_value("grey"));
		change_color_hsv_cfg(color_hsv_cfg, 2, color_hsv_cfg_abi.get_value("white"));
		change_color_hsv_cfg(color_hsv_cfg, 3, color_hsv_cfg_abi.get_value("red"));
		change_color_hsv_cfg(color_hsv_cfg, 4, color_hsv_cfg_abi.get_value("orange"));
		change_color_hsv_cfg(color_hsv_cfg, 5, color_hsv_cfg_abi.get_value("yellow"));
		change_color_hsv_cfg(color_hsv_cfg, 6, color_hsv_cfg_abi.get_value("green"));
		change_color_hsv_cfg(color_hsv_cfg, 7, color_hsv_cfg_abi.get_value("cyan"));
		change_color_hsv_cfg(color_hsv_cfg, 8, color_hsv_cfg_abi.get_value("blue"));
		change_color_hsv_cfg(color_hsv_cfg, 9, color_hsv_cfg_abi.get_value("purple"));

		return impl_->detect(std::move(bitmap), channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map, color_hsv_cfg);
	}

}
