#ifndef __BOX_INFO_IMPL_HPP__
#define __BOX_INFO_IMPL_HPP__

#include "box_info.hpp"
#include "classify_code_internal.hpp"

namespace glasssix::pedestrian
{
    inline constexpr exposing::utf8_string_view pedestrian_box_info_qualified_name{ u8"g6.pedestrian.boxInfo" };

    class box_info_impl : public exposing::implements<box_info_impl, box_info>, public exposing::make_external_qualified_name<pedestrian_box_info_qualified_name>
    {
    public:
        box_info_impl();
        box_info_impl(const box_info_internal& internal);
        ~box_info_impl();

        int x1() const;
        int y1() const;
        int x2() const;
        int y2() const;
        float score() const;
        int category() const;
        void set_x1(float input);
        void set_y1(float input);
        void set_x2(float input);
        void set_y2(float input);
        void set_score(float input);
        void set_category(int input);

    private:
        box_info_internal internal_{};
    };
}
#endif