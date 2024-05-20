#pragma once

#include "land_code.hpp"
#include <memory>
#include <abi/consumer.hpp>

namespace glasssix::facelandmarks
{
    inline constexpr exposing::utf8_string_view facelandmarks_land_code_qualified_name{ u8"g6.facelandmarks.land_code" };

    class land_code_internal;

    class land_code_impl : public exposing::implements<land_code_impl, land_code>, public exposing::make_external_qualified_name<facelandmarks_land_code_qualified_name>
    {
    public:
        land_code_impl();
        ~land_code_impl();
        void init(const exposing::param_string& model_directory, std::int32_t device);
        facelandmarks::land_info detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width);
        exposing::param_string version() const;

    private:
        std::unique_ptr<land_code_internal> impl_;
    };
}
