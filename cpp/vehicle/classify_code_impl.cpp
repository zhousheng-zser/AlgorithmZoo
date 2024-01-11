#include "classify_code_impl.hpp"
#include "classify_code_internal.hpp"
#include <map>

namespace glasssix::vehicle
{
    classify_code_impl::classify_code_impl() {}

    classify_code_impl::~classify_code_impl() {}

    void classify_code_impl::init(const exposing::param_string& model_directory, std::int32_t device)
    {
        impl_ = std::make_unique<classify_code_internal>(exposing::to_narrow_string(model_directory), device);
    }

    exposing::param_vector<vehicle::box_info> classify_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t roi_x, std::int32_t roi_y, std::int32_t roi_width, std::int32_t roi_height, const exposing::param_hash_map<exposing::param_string,float>& param_map_abi)
    {
        std::map<std::string,float> param_map_std;
        for (auto it : param_map_abi) {
            param_map_std.insert(std::make_pair(it.key(), it.value()));
        }

        if (!impl_)
            throw exposing::abi_invalid_operation(u8"vehicle classify_code internal object not initialized"); 

        return impl_->detect(bitmap, channels, height, width, roi_x, roi_y, roi_width, roi_height, param_map_std);
    }

    exposing::param_string classify_code_impl::version() const
    {
        return exposing::to_param_string(impl_->version());
    }
}
