#ifndef __HAT_INFO_IMPL_HPP__
#define __HAT_INFO_IMPL_HPP__

#include "hat_info.hpp"
#include "yolo_net_internal.hpp"

#include <abi/consumer.hpp>

namespace glasssix::gungnir
{
    inline constexpr exposing::utf8_string_view gungnir_hat_info_qualified_name{u8"g6.gungnir.hatInfo"};

    class hat_info_impl : public exposing::implements<hat_info_impl, hat_info>, public exposing::make_external_qualified_name<gungnir_hat_info_qualified_name>
    {
    public:
        hat_info_impl();
        hat_info_impl(const hat_info_internal &internal);
        ~hat_info_impl();

        float x() const;
		float y() const;
		float width() const;
		float height() const;
		float prob() const;
		float label() const;

    private:
        hat_info_internal internal_;
    };
}

#endif