#include "gate_status_impl.hpp"
#include "gate_status_internal.hpp"

namespace glasssix::pump_gate_status
{
    gate_status_impl::gate_status_impl()
    {
    }

    gate_status_impl::~gate_status_impl()
    {
    }

    void gate_status_impl::init(std::int32_t device)
	{
		impl_ = std::make_unique<gate_status_internal>();
	}

    void gate_status_impl::init(std::int32_t model_type, const exposing::param_string &racy_path, std::int32_t device, bool use_int8)
    {
        impl_ = std::make_unique<gate_status_internal>(model_type, exposing::to_narrow_string(racy_path), device, use_int8);
    }

    void gate_status_impl::init(exposing::param_span<const exposing::param_string> phai, const exposing::param_string &racy_path, std::int32_t device)
    {
        std::vector<std::string> phai_internal(phai.size());
        std::transform(phai.begin(), phai.end(), phai_internal.begin(), &exposing::to_narrow_string);
        impl_ = std::make_unique<gate_status_internal>(phai_internal, exposing::to_narrow_string(racy_path), device);
    }

    exposing::param_string gate_status_impl::version() const
    {
        return exposing::to_param_string(impl_->version());
    }

    exposing::param_vector<exposing::param_vector<float>> gate_status_impl::get(exposing::param_span<std::uint8_t> bitmaps, std::uint64_t count, std::int32_t order) const
    {
        auto native_result = impl_->get(bitmaps, count, order);
        auto result = exposing::make_param_vector<float, 2>();

        for (const auto &item : native_result)
        {
            result.push_back(exposing::make_param_vector<float>(item));
        }

        return result;
    }

    std::int32_t gate_status_impl::detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int yellow_hsv_lower, int yellow_hsv_upper, int gray_hsv_lower, int gray_hsv_upper, 
            const exposing::param_vector<int>& rois,    
    const exposing::param_hash_map<exposing::param_string, float>& param_map_abi) const
    { 
        std::map<std::string, float> param_map;
		for (auto it : param_map_abi) 
			param_map.insert(std::make_pair(it.key(), it.value()));

        std::vector<int> std_rois(rois.size());
        for (size_t i = 0; i < rois.size(); i++)
            std_rois[i] = rois[i];
        
        return impl_->detect(bitmap, channels, height, width, yellow_hsv_lower, yellow_hsv_upper, gray_hsv_lower,  gray_hsv_upper, std_rois,  param_map  );
    }
}
