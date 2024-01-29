#include "detect_code_impl.hpp"
#include "detect_code_internal.hpp"
#include <map>

namespace glasssix::vesthelmet
{
    detect_code_impl::detect_code_impl() {}

    detect_code_impl::~detect_code_impl() {}

    void detect_code_impl::init(const exposing::param_string& model_directory, std::int32_t device)
    {
        impl_ = std::make_unique<detect_code_internal>(exposing::to_narrow_string(model_directory), device);
    }

    exposing::param_vector<vesthelmet::box_info> detect_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, const exposing::param_hash_map<exposing::param_string,float>& param_map_abi)
    {
        std::map<std::string,float> param_map_std;
        for (auto it : param_map_abi) {
            param_map_std.insert(std::make_pair(it.key(), it.value()));
        }

        if (!impl_)
            throw exposing::abi_invalid_operation(u8"vesthelmet detect_code internal object not initialized"); 

        return impl_->detect(bitmap, channels, height, width, param_map_std);
    }

    exposing::param_string detect_code_impl::version() const
    {
        return exposing::to_param_string(impl_->version());
    }
}
