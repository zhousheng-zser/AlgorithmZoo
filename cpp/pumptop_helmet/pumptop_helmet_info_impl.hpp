#pragma once

#include "pumptop_helmet_info.hpp"
#include "pumptop_helmet_detector_impl.hpp"

namespace glasssix::pumptop_helmet
{

	inline constexpr exposing::utf8_string_view pumptop_helmet_info_qualified_name{ u8"g6.pumptop_helmet.pumptop_helmet_info" };

	class pumptop_helmet_info_impl : public exposing::implements<pumptop_helmet_info_impl, pumptop_helmet_info>, public exposing::make_external_qualified_name<pumptop_helmet_info_qualified_name>
	{
	public:
		pumptop_helmet_info_impl();
		pumptop_helmet_info_impl(const pumptop_helmet::pumptop_helmet_info_internal& internal);
		~pumptop_helmet_info_impl();

		int x1() const;
		int y1() const;
		int x2() const;
		int y2() const;
		int category() const;

	private:
		pumptop_helmet_info_internal internal_;
	};
}
