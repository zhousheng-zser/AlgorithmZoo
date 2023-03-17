#ifndef __TXT_CODE_IMPL_HPP__
#define __TXT_CODE_IMPL_HPP__

#include "txt_code.hpp"

#include <memory>
#include <abi/consumer.hpp>

namespace glasssix::genocr
{
    inline constexpr exposing::utf8_string_view genocr_txt_code_qualified_name{ u8"g6.genocr.txt_code" };

    class txt_code_internal;

    class txt_code_impl : public exposing::implements<txt_code_impl, txt_code>, public exposing::make_external_qualified_name<genocr_txt_code_qualified_name>
    {
    public:
        txt_code_impl();
        ~txt_code_impl();
        void init(const exposing::param_string& model_directory, const exposing::param_string& chardic_directory, const std::int32_t factory_type, std::int32_t device, const exposing::param_hash_map<exposing::param_string, float>& param_map_abi);
        exposing::param_string version() const;
        exposing::param_vector<box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int top_five, int order, int x, int y, int roi_width, int roi_height) const;

    private:
        std::unique_ptr<txt_code_internal> impl_;
    };
}

#endif