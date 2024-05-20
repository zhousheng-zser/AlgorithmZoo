#include "land_code_impl.hpp"
#include "land_code_internal.hpp"
#include <map>

namespace glasssix::facelandmarks
{
    land_code_impl::land_code_impl() {}

    land_code_impl::~land_code_impl() {}

    void land_code_impl::init(const exposing::param_string& model_directory, std::int32_t device)
    {
        impl_ = std::make_unique<land_code_internal>(exposing::to_narrow_string(model_directory), device);
    }

    facelandmarks::land_info land_code_impl::detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width)
    {
        if (!impl_)
            throw exposing::abi_invalid_operation(u8"facelandmarks land_code internal object not initialized"); 

        return impl_->detect(bitmap, channels, height, width);
    }

    exposing::param_string land_code_impl::version() const
    {
        return exposing::to_param_string(impl_->version());
    }
}
