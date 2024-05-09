#pragma once

#include "facedetector.hpp"
#include <memory>

#include <abi/consumer.hpp>

namespace glasssix::longinus
{
	inline constexpr exposing::utf8_string_view longinus_facedetector_qualified_name{ u8"g6.longinus.facedetector" };

	class facedetector_base;

	class facedetector_impl : public exposing::implements<facedetector_impl, facedetector>, public exposing::make_external_qualified_name<longinus_facedetector_qualified_name>
	{
	public:
		facedetector_impl();
		~facedetector_impl();

		void init(const exposing::param_string& str_params);
		exposing::param_string version() const;
		exposing::param_string execute(const exposing::param_hash_map<exposing::param_string, exposing::unknown_object>& input_params_map);
	private:
		int algo_type_;
		std::unique_ptr<facedetector_base> impl_;
	};
}
