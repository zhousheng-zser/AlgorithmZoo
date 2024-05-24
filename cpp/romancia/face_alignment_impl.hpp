#pragma once

#include "face_alignment.hpp"

#include <memory>

#include <abi/consumer.hpp>

namespace glasssix::romancia
{
	inline constexpr exposing::utf8_string_view romancia_face_alignment_qualified_name{ u8"g6.romancia.faceAlignment" };

	class face_alignment_internal;

	class face_alignment_impl : public exposing::implements<face_alignment_impl, face_alignment>, public exposing::make_external_qualified_name<romancia_face_alignment_qualified_name>
	{
	public:
		face_alignment_impl();
		~face_alignment_impl();

		void init(const exposing::param_string& str_params);
		exposing::param_string version() const;
		exposing::param_string execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map);
	private:
		std::unique_ptr<face_alignment_internal> impl_;
	};
}
