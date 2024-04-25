#include "detect_code_impl.hpp"
#include "detect_code_internal.hpp"
#include <map>

namespace glasssix::fighting
{
    detect_code_impl::detect_code_impl() {}

    detect_code_impl::~detect_code_impl() {}

    void detect_code_impl::init(const exposing::param_string& model_directory, std::int32_t device, std::int32_t batch)
    {
        impl_ = std::make_unique<detect_code_internal>(exposing::to_narrow_string(model_directory), device, batch);
    }

    exposing::param_vector<fighting::box_info> detect_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t height, std::int32_t width, std::int32_t roi_x, std::int32_t roi_y, std::int32_t roi_width, std::int32_t roi_height, const exposing::param_hash_map<exposing::param_string,float>& param_map_abi)
    {
        std::map<std::string,float> param_map_std;
        for (auto it : param_map_abi) {
            param_map_std.insert(std::make_pair(it.key(), it.value()));
        }

        if (!impl_)
            throw exposing::abi_invalid_operation(u8"fighting detect_code internal object not initialized"); 

        return impl_->detect(bitmap, height, width, roi_x, roi_y, roi_width, roi_height, param_map_std);
    }

    exposing::param_string detect_code_impl::version() const
    {
        return exposing::to_param_string(impl_->version());
    }
}
