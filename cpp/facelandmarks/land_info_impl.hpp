#pragma once

#include "land_info.hpp"
#include "land_info_internal.hpp"

namespace glasssix::facelandmarks
{
    inline constexpr exposing::utf8_string_view facelandmarks_land_info_qualified_name{ u8"g6.facelandmarks.land_info" };


    class land_info_impl : public exposing::implements<land_info_impl, land_info>, public exposing::make_external_qualified_name<facelandmarks_land_info_qualified_name>
    {
    public:
        land_info_impl();
        land_info_impl(const land_info_internal &internal);
        ~land_info_impl();
        exposing::param_vector<exposing::param_pair<float,float>> pts();
        float score();

    private:
        land_info_internal internal_;
    };
}
