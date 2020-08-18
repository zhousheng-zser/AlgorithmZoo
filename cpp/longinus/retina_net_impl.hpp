#pragma once

#include "retina_net.hpp"

#include <abi/consumer.hpp>

namespace glasssix::longinus
{
	inline constexpr exposing::utf8_string_view longinus_retina_net_qualified_name{ u8"g6.longinus.retinaNet" };

	class retina_net_native;

	class retina_net_impl : public exposing::implements<retina_net_impl, retina_net>, public exposing::make_external_qualified_name<longinus_retina_net_qualified_name>
	{
	public:
		retina_net_impl();
		~retina_net_impl();

		void init(exposing::param_string phai_path, exposing::param_string racy_path, float nms_threshold, std::int32_t device);

		exposing::param_string version() const;
		exposing::param_vector<longinus::face_info> detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t min_size, float threshold, std::int32_t order) const;
	private:
		retina_net_native* impl_;
	};
}
