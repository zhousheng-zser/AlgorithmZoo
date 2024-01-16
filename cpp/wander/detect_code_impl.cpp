#include "detect_code_impl.hpp"
#include "detect_code_internal.hpp"
#include "box_info_impl.hpp"

#include <utility>

namespace glasssix::wander
{
	
	detect_code_impl::detect_code_impl() = default;

	detect_code_impl::~detect_code_impl() = default;


	void detect_code_impl::init(const exposing::param_string& model_directory, std::int32_t device)
	{
		impl_ = std::make_unique<detect_code_internal>(exposing::to_narrow_string(model_directory), device);
	}

	exposing::param_string detect_code_impl::version() const
	{
		return exposing::to_param_string(impl_->version());
	}

	exposing::param_string detect_code_impl::remove_library(std::int32_t device) const
	{
		return exposing::to_param_string(impl_->remove_library(device));
	}

	exposing::param_string detect_code_impl::remove_person_by_index(std::int32_t device_id, std::int32_t id)const 
	{
		return exposing::to_param_string(impl_->remove_person_by_index(device_id,id));
	}

	exposing::param_vector<wander::box_info> detect_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width,int roi_x, int roi_y, int roi_width, int roi_height,
		const exposing::param_hash_map<exposing::param_string, double>& param_map_abi, 
		const exposing::param_vector<pedestrian::box_info>& pedestrain_info_abi) const
	{
		if (!impl_)
			throw exposing::abi_invalid_operation(u8"wander internal object not initialized");

		std::map<std::string, double> param_map;
		std::vector<PedestrianInfo> pedestrain_info;
		for (auto it : pedestrain_info_abi)
			pedestrain_info.emplace_back(PedestrianInfo{it});

		for (auto it : param_map_abi) {
			param_map.insert(std::make_pair(it.key(), it.value()));
		}

		return impl_->detect(std::move(bitmap), channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map, pedestrain_info);
	}

}
