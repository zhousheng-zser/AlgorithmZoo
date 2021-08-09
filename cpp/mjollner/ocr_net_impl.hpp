#ifndef __OCR_NET_IMPL_HPP__
#define __OCR_NET_IMPL_HPP__

#include "ocr_net.hpp"

#include <memory>
#include <abi/consumer.hpp>

namespace glasssix::mjollner
{
    inline constexpr exposing::utf8_string_view mjollner_ocr_net_qualified_name{u8"g6.mjollner.ocr_net"};

    class ocr_net_internal;

    class ocr_net_impl : public exposing::implements<ocr_net_impl, ocr_net>, public exposing::make_external_qualified_name<mjollner_ocr_net_qualified_name>
    {
    public:
        ocr_net_impl();
        ~ocr_net_impl();
        void init(const exposing::param_string &det_racy_path, const exposing::param_string &rec_racy_path, const exposing::param_string &alphabet_path, std::int32_t device);
        void init(exposing::param_span<const exposing::param_string> det_phai, const exposing::param_string &det_racy_path, exposing::param_span<const exposing::param_string> rec_phai, const exposing::param_string &rec_racy_path, const exposing::param_string &alphabet_path, std::int32_t device);
        exposing::param_string version() const;
        exposing::param_vector<box_info> detect(exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order, int x, int y, int roi_width, int roi_height) const;

    private:
        std::unique_ptr<ocr_net_internal> impl_;
    };
}

#endif