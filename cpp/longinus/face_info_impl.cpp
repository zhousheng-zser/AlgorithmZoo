#include "face_info_impl.hpp"

namespace glasssix::longinus
{
	face_info_impl::face_info_impl()
	{
	}
	face_info_impl::face_info_impl(const face_info_internal& internal) : internal_(internal)
	{
	}
	face_info_impl::~face_info_impl()
	{
	}

	int face_info_impl::x() const
	{
		return internal_.rect.x;
	}

	int face_info_impl::y() const
	{
		return internal_.rect.y;
	}

	int face_info_impl::width() const
	{
		return internal_.rect.w;
	}

	int face_info_impl::height() const
	{
		return internal_.rect.h;
	}

	float face_info_impl::yaw() const
	{
		return internal_.headpose[0];
	}

	float face_info_impl::pitch() const
	{
		return internal_.headpose[1];
	}

	float face_info_impl::roll() const
	{
		return internal_.headpose[2];
	}


	float face_info_impl::clarity() const
	{
		return internal_.clarity;
	}

	float face_info_impl::confidence() const
	{
		return internal_.score;
	}

	float face_info_impl::has_mask() const
	{
		return internal_.has_mask;
	}

	std::int32_t face_info_impl::is_alive() const
	{
		return internal_.is_alive ? 1 : 0;
	}

	std::int32_t face_info_impl::prob_age_index() const
	{
		return internal_.prob_age_index;
	}

	std::int32_t face_info_impl::prob_gender_index() const
	{
		return internal_.prob_gender_index;
	}

	exposing::param_vector<exposing::param_pair<float, float>> face_info_impl::pts() const
	{
		auto vec = exposing::make_param_vector<exposing::param_pair<float, float>>();
		for (int i = 0; i < sizeof(face_pts::x) / sizeof(float); i++)
		{
			auto pair = exposing::make_param_pair(internal_.pts.x[i], internal_.pts.y[i]);
			vec.push_back(pair);
		}

		return vec;
	}

	void face_info_impl::set_pts(exposing::param_vector<exposing::param_pair<float, float>> input)
	{
		if(input.size() != 5)
			throw exposing::abi_invalid_argument(exposing::format(u8"input vector size != 5. Actual: {}", input.size()));

		for (size_t i = 0; i < input.size(); i++)
		{
			internal_.pts.x[i] = input[i].key();
			internal_.pts.y[i] = input[i].value();
		}
	}
	void face_info_impl::set_yaw(float input)
	{
		internal_.headpose[0] = input;
	}
	void face_info_impl::set_pitch(float input)
	{
		internal_.headpose[1] = input;
	}
	void face_info_impl::set_roll(float input)
	{
		internal_.headpose[2] = input;
	}
	void face_info_impl::set_clarity(float input)
	{
		internal_.clarity = input;
	}
	void face_info_impl::set_x(std::int32_t input)
	{
		internal_.rect.x = input;
	}
	void face_info_impl::set_y(std::int32_t input)
	{
		internal_.rect.y = input;
	}
	void face_info_impl::set_width(std::int32_t input)
	{
		internal_.rect.w = input;
	}
	void face_info_impl::set_height(std::int32_t input)
	{
		internal_.rect.h = input;
	}
	void face_info_impl::set_confidence(float input)
	{
		internal_.score = input;
	}
	void face_info_impl::set_has_mask(float input)
	{
		internal_.has_mask = input;
	}
	void face_info_impl::set_is_alive(std::int32_t  input)
	{
		internal_.is_alive = input ? true : false;
	}
	void face_info_impl::set_prob_age_index(std::int32_t  input)
	{
		internal_.prob_age_index = input;
	}
	void face_info_impl::set_prob_gender_index(std::int32_t  input)
	{
		internal_.prob_gender_index = input;
	}
}