#ifndef __CLASSIFY_CODE_IMPL_HPP__
#define __CLASSIFY_CODE_IMPL_HPP__

#include "classify_code.hpp"

#include <memory>
#include <abi/consumer.hpp>

namespace glasssix::refvest
{
    inline constexpr exposing::utf8_string_view refvest_classify_code_qualified_name{ u8"g6.refvest.classify_code" };

    class classify_code_internal;

    class classify_code_impl : public exposing::implements<classify_code_impl, classify_code>, public exposing::make_external_qualified_name<refvest_classify_code_qualified_name>
    {
    public:
        classify_code_impl();
        ~classify_code_impl();

        void init(const exposing::param_string& model_directory, std::int32_t device);

        exposing::param_string version() const;

        exposing::param_vector<refvest::box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int roi_x, int roi_y, int roi_width, int roi_height) const;

    private:

        std::unique_ptr<classify_code_internal> impl_;
    };
}

#endif