#pragma once

#include "retina_net.hpp"

#include <memory>

#include <abi/consumer.hpp>

namespace glasssix::longinus
{
	inline constexpr exposing::utf8_string_view longinus_retina_net_qualified_name{ u8"g6.longinus.retinaNet" };

	class retina_net_internal;

	class retina_net_impl : public exposing::implements<retina_net_impl, retina_net>, public exposing::make_external_qualified_name<longinus_retina_net_qualified_name>
	{
	public:
		retina_net_impl();
		~retina_net_impl();

		void init(const exposing::param_string& racy_path, const exposing::param_string& tracker_racy_path, float nms_threshold, std::int32_t device);
		void init(exposing::param_span<const exposing::param_string> phai, const exposing::param_string& racy_path, exposing::param_span<const exposing::param_string> tracker_phai, const exposing::param_string& tracker_racy_path, float nms_threshold, std::int32_t device);

		exposing::param_string version() const;
		exposing::param_vector<face_info> detect(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t min_size, float threshold, std::int32_t order, bool do_attributing) const;
		face_info single_trace(face_info face, exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width, std::int32_t order) const;
		exposing::param_vector<exposing::param_vector<std::uint8_t>> center_scale_align(exposing::param_span<std::uint8_t> bitmap, std::int32_t channels, std::int32_t height, std::int32_t width,
			float scale, std::int32_t order) const;
	private:
		std::unique_ptr<retina_net_internal> impl_;
	};
}
