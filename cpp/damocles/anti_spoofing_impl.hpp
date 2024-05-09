#pragma once

#include "anti_spoofing.hpp"

#include <memory>

#include <abi/consumer.hpp>

namespace glasssix::damocles
{
	inline constexpr exposing::utf8_string_view damocles_anti_spoofing_qualified_name{ u8"g6.damocles.anti_spoofing" };

	class anti_spoofing_internal;

	class anti_spoofing_impl : public exposing::implements<anti_spoofing_impl, anti_spoofing>, public exposing::make_external_qualified_name<damocles_anti_spoofing_qualified_name>
	{
	public:
		anti_spoofing_impl();
		~anti_spoofing_impl();

		void init(const exposing::param_string& str_params);
		exposing::param_string version() const;
		exposing::param_string execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map);

		exposing::param_vector<exposing::param_vector<float>> spoofing_detect(const exposing::param_vector<longinus::face_info>& faces, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const;
		bool presentation_attack_detect(int action_cmd, const longinus::face_info& face, exposing::param_span<std::uint8_t> bitmap, int channels, int height, int width, int order) const;
	private:
		std::unique_ptr<anti_spoofing_internal> impl_;
	};
}
