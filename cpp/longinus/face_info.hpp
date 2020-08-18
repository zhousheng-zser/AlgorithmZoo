#pragma once

#include <abi/consumer.hpp>

namespace glasssix::longinus
{
	struct face_info;
}

namespace glasssix::exposing::impl
{
	template<> struct abi<longinus::face_info>
	{
		using identity_type = type_identity_interface;

		static constexpr guid id{ "FD21EE9C-CCBE-473B-9C0C-3402C2E493A1" };

		struct type : abi_unknown_object
		{
			virtual std::int32_t pts(abi_out_t<param_vector<param_pair<float, float>>> result) noexcept = 0;
			virtual std::int32_t yaw(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t pitch(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t roll(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t clarity(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t x(abi_out_t<std::int32_t> result) noexcept = 0;
			virtual std::int32_t y(abi_out_t<std::int32_t> result) noexcept = 0;
			virtual std::int32_t width(abi_out_t<std::int32_t> result) noexcept = 0;
			virtual std::int32_t height(abi_out_t<std::int32_t> result) noexcept = 0;
			virtual std::int32_t confidence(abi_out_t<float> result) noexcept = 0;

			virtual std::int32_t set_pts(abi_in_t<param_vector<param_pair<float, float>>> input) noexcept = 0;
			virtual std::int32_t set_yaw(abi_in_t<float> input) noexcept = 0;
			virtual std::int32_t set_pitch(abi_in_t<float> input) noexcept = 0;
			virtual std::int32_t set_roll(abi_in_t<float> input) noexcept = 0;
			virtual std::int32_t set_clarity(abi_in_t<float> input) noexcept = 0;
			virtual std::int32_t set_x(abi_in_t<std::int32_t> input) noexcept = 0;
			virtual std::int32_t set_y(abi_in_t<std::int32_t> input) noexcept = 0;
			virtual std::int32_t set_width(abi_in_t<std::int32_t> input) noexcept = 0;
			virtual std::int32_t set_height(abi_in_t<std::int32_t> input) noexcept = 0;
			virtual std::int32_t set_confidence(abi_in_t<float> input) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, longinus::face_info> : interface_vtable_base<Derived, longinus::face_info>
	{
		virtual std::int32_t pts(abi_out_t<param_vector<param_pair<float, float>>> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().pts()); });
		}

		virtual std::int32_t yaw(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().yaw()); });
		}

		virtual std::int32_t pitch(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().pitch()); });
		}

		virtual std::int32_t roll(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().roll()); });
		}

		virtual std::int32_t clarity(abi_out_t<float> result) noexcept override 
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().clarity()); });
		}

		virtual std::int32_t x(abi_out_t<std::int32_t> result) noexcept override 
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().x()); });
		}

		virtual std::int32_t y(abi_out_t<std::int32_t> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().y()); });
		}

		virtual std::int32_t width(abi_out_t<std::int32_t> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().width()); });
		}

		virtual std::int32_t height(abi_out_t<std::int32_t> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().height()); });
		}

		virtual std::int32_t confidence(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().confidence()); });
		}

		virtual std::int32_t set_pts(abi_in_t<param_vector<param_pair<float, float>>> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_pts(create_from_abi<param_vector<param_pair<float, float>>>(input)); });
		}
		virtual std::int32_t set_yaw(abi_in_t<float> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_yaw(create_from_abi<float>(input)); });
		}
		virtual std::int32_t set_pitch(abi_in_t<float> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_pitch(create_from_abi<float>(input)); });
		}
		virtual std::int32_t set_roll(abi_in_t<float> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_roll(create_from_abi<float>(input)); });
		}
		virtual std::int32_t set_clarity(abi_in_t<float> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_clarity(create_from_abi<float>(input)); });
		}
		virtual std::int32_t set_x(abi_in_t<std::int32_t> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_x(create_from_abi<std::int32_t>(input)); });
		}
		virtual std::int32_t set_y(abi_in_t<std::int32_t> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_y(create_from_abi<std::int32_t>(input)); });
		}
		virtual std::int32_t set_width(abi_in_t<std::int32_t> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_width(create_from_abi<std::int32_t>(input)); });
		}
		virtual std::int32_t set_height(abi_in_t<std::int32_t> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_height(create_from_abi<std::int32_t>(input)); });
		}
		virtual std::int32_t set_confidence(abi_in_t<float> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_confidence(create_from_abi<float>(input)); });
		}
	};

	template<> struct abi_adapter<longinus::face_info>
	{
		template<typename Derived>
		struct type : enable_self_abi_awareness<Derived, longinus::face_info>
		{
			param_vector<param_pair<float, float>> pts()const
			{
				param_vector<param_pair<float, float>> result{ nullptr };

				return (check_abi_result(this->self_abi().pts(put_abi(result))), result);
			}

			float yaw() const
			{
				float result = 0.f;

				return (check_abi_result(this->self_abi().yaw(put_abi(result))), result);
			}
			float pitch() const
			{
				float result = 0.f;

				return (check_abi_result(this->self_abi().pitch(put_abi(result))), result);
			}
			float roll() const
			{
				float result = 0.f;

				return (check_abi_result(this->self_abi().roll(put_abi(result))), result);
			}
			float clarity() const
			{
				float result = 0.f;

				return (check_abi_result(this->self_abi().clarity(put_abi(result))), result);
			}
			std::int32_t x() const
			{
				std::int32_t result = 0;

				return (check_abi_result(this->self_abi().x(put_abi(result))), result);
			}
			std::int32_t y() const
			{
				std::int32_t result = 0;

				return (check_abi_result(this->self_abi().y(put_abi(result))), result);
			}
			std::int32_t width() const
			{
				std::int32_t result = 0;

				return (check_abi_result(this->self_abi().width(put_abi(result))), result);
			}
			std::int32_t height() const
			{
				std::int32_t result = 0;

				return (check_abi_result(this->self_abi().height(put_abi(result))), result);
			}
			float confidence() const
			{
				float result = 0.f;

				return (check_abi_result(this->self_abi().confidence(put_abi(result))), result);
			}

			void set_pts(param_vector<param_pair<float, float>> input)
			{
				check_abi_result(this->self_abi().set_pts(get_abi(input)));
			}
			void set_yaw(float input)
			{
				check_abi_result(this->self_abi().set_yaw(get_abi(input)));
			}
			void set_pitch(float input)
			{
				check_abi_result(this->self_abi().set_pitch(get_abi(input)));
			}
			void set_roll(float input)
			{
				check_abi_result(this->self_abi().set_roll(get_abi(input)));
			}
			void set_clarity(float input)
			{
				check_abi_result(this->self_abi().set_clarity(get_abi(input)));
			}
			void set_x(std::int32_t input)
			{
				check_abi_result(this->self_abi().set_x(get_abi(input)));
			}
			void set_y(std::int32_t input)
			{
				check_abi_result(this->self_abi().set_y(get_abi(input)));
			}
			void set_width(std::int32_t input)
			{
				check_abi_result(this->self_abi().set_width(get_abi(input)));
			}
			void set_height(std::int32_t input)
			{
				check_abi_result(this->self_abi().set_height(get_abi(input)));
			}
			void set_confidence(float input)
			{
				check_abi_result(this->self_abi().set_confidence(get_abi(input)));
			}
		};
	};
}

namespace glasssix::longinus
{
	struct face_info : exposing::inherits<face_info>
	{
		using inherits::inherits;
	};
}
