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
			virtual std::int32_t G6_ABI_CALL pts(abi_out_t<param_vector<param_pair<float, float>>> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL yaw(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL pitch(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL roll(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL clarity(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL x(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL y(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL width(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL height(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL ori_x(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL ori_y(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL ori_width(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL ori_height(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL confidence(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL has_mask(abi_out_t<float> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL is_alive(abi_out_t<std::int32_t> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL glass_index(abi_out_t<std::int32_t> result) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL mask_index(abi_out_t<std::int32_t> result) noexcept = 0;

			virtual std::int32_t G6_ABI_CALL set_pts(abi_in_t<param_vector<param_pair<float, float>>> input) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_yaw(abi_in_t<float> input) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_pitch(abi_in_t<float> input) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_roll(abi_in_t<float> input) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_clarity(abi_in_t<float> input) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_x(abi_in_t<float> input) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_y(abi_in_t<float> input) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_width(abi_in_t<float> input) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_height(abi_in_t<float> input) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_confidence(abi_in_t<float> input) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_has_mask(abi_in_t<float> input) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_is_alive(abi_in_t<std::int32_t> input) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_glass_index(abi_in_t<std::int32_t> input) noexcept = 0;
			virtual std::int32_t G6_ABI_CALL set_mask_index(abi_in_t<std::int32_t> input) noexcept = 0;
		};
	};

	template<typename Derived>
	struct interface_vtable<Derived, longinus::face_info> : interface_vtable_base<Derived, longinus::face_info>
	{
		virtual std::int32_t G6_ABI_CALL pts(abi_out_t<param_vector<param_pair<float, float>>> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().pts()); });
		}

		virtual std::int32_t G6_ABI_CALL yaw(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().yaw()); });
		}

		virtual std::int32_t G6_ABI_CALL pitch(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().pitch()); });
		}

		virtual std::int32_t G6_ABI_CALL roll(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().roll()); });
		}

		virtual std::int32_t G6_ABI_CALL clarity(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().clarity()); });
		}

		virtual std::int32_t G6_ABI_CALL x(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().x()); });
		}

		virtual std::int32_t G6_ABI_CALL y(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().y()); });
		}

		virtual std::int32_t G6_ABI_CALL width(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().width()); });
		}

		virtual std::int32_t G6_ABI_CALL height(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().height()); });
		}

		virtual std::int32_t G6_ABI_CALL ori_x(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().ori_x()); });
		}

		virtual std::int32_t G6_ABI_CALL ori_y(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().ori_y()); });
		}

		virtual std::int32_t G6_ABI_CALL ori_width(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().ori_width()); });
		}

		virtual std::int32_t G6_ABI_CALL ori_height(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().ori_height()); });
		}

		virtual std::int32_t G6_ABI_CALL confidence(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().confidence()); });
		}

		virtual std::int32_t G6_ABI_CALL has_mask(abi_out_t<float> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().has_mask()); });
		}

		virtual std::int32_t G6_ABI_CALL is_alive(abi_out_t<std::int32_t> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().is_alive()); });
		}

		virtual std::int32_t G6_ABI_CALL glass_index(abi_out_t<std::int32_t> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().glass_index()); });
		}
        
        virtual std::int32_t G6_ABI_CALL mask_index(abi_out_t<std::int32_t> result) noexcept override
		{
			return abi_safe_call([&] {*result = detach_abi(this->self().mask_index()); });
		}

		virtual std::int32_t G6_ABI_CALL set_pts(abi_in_t<param_vector<param_pair<float, float>>> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_pts(create_from_abi<param_vector<param_pair<float, float>>>(input)); });
		}
		virtual std::int32_t G6_ABI_CALL set_yaw(abi_in_t<float> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_yaw(create_from_abi<float>(input)); });
		}
		virtual std::int32_t G6_ABI_CALL set_pitch(abi_in_t<float> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_pitch(create_from_abi<float>(input)); });
		}
		virtual std::int32_t G6_ABI_CALL set_roll(abi_in_t<float> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_roll(create_from_abi<float>(input)); });
		}
		virtual std::int32_t G6_ABI_CALL set_clarity(abi_in_t<float> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_clarity(create_from_abi<float>(input)); });
		}
		virtual std::int32_t G6_ABI_CALL set_x(abi_in_t<float> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_x(create_from_abi<float>(input)); });
		}
		virtual std::int32_t G6_ABI_CALL set_y(abi_in_t<float> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_y(create_from_abi<float>(input)); });
		}
		virtual std::int32_t G6_ABI_CALL set_width(abi_in_t<float> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_width(create_from_abi<float>(input)); });
		}
		virtual std::int32_t G6_ABI_CALL set_height(abi_in_t<float> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_height(create_from_abi<float>(input)); });
		}
		virtual std::int32_t G6_ABI_CALL set_confidence(abi_in_t<float> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_confidence(create_from_abi<float>(input)); });
		}
		virtual std::int32_t G6_ABI_CALL set_has_mask(abi_in_t<float> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_has_mask(create_from_abi<float>(input)); });
		}
		virtual std::int32_t G6_ABI_CALL set_is_alive(abi_in_t<std::int32_t> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_is_alive(create_from_abi<std::int32_t>(input)); });
		}
		virtual std::int32_t G6_ABI_CALL set_glass_index(abi_in_t<std::int32_t> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_glass_index(create_from_abi<std::int32_t>(input)); });
		}
        virtual std::int32_t G6_ABI_CALL set_mask_index(abi_in_t<std::int32_t> input) noexcept override
		{
			return abi_safe_call([&] { this->self().set_mask_index(create_from_abi<std::int32_t>(input)); });
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
			float x() const
			{
				float result = 0;

				return (check_abi_result(this->self_abi().x(put_abi(result))), result);
			}
			float y() const
			{
				float result = 0;

				return (check_abi_result(this->self_abi().y(put_abi(result))), result);
			}
			float width() const
			{
				float result = 0;

				return (check_abi_result(this->self_abi().width(put_abi(result))), result);
			}
			float height() const
			{
				float result = 0;

				return (check_abi_result(this->self_abi().height(put_abi(result))), result);
			}
			float ori_x() const
			{
				float result = 0;

				return (check_abi_result(this->self_abi().ori_x(put_abi(result))), result);
			}
			float ori_y() const
			{
				float result = 0;

				return (check_abi_result(this->self_abi().ori_y(put_abi(result))), result);
			}
			float ori_width() const
			{
				float result = 0;

				return (check_abi_result(this->self_abi().ori_width(put_abi(result))), result);
			}
			float ori_height() const
			{
				float result = 0;

				return (check_abi_result(this->self_abi().ori_height(put_abi(result))), result);
			}
			float confidence() const
			{
				float result = 0.f;

				return (check_abi_result(this->self_abi().confidence(put_abi(result))), result);
			}

			float has_mask() const
			{
				float result = 0.f;

				return (check_abi_result(this->self_abi().has_mask(put_abi(result))), result);
			}

			std::int32_t is_alive() const
			{
				std::int32_t result = 0;

				return (check_abi_result(this->self_abi().is_alive(put_abi(result))), result);
			}

			std::int32_t glass_index() const
			{
				std::int32_t result = 0;

				return (check_abi_result(this->self_abi().glass_index(put_abi(result))), result);
			}

            std::int32_t mask_index() const
			{
				std::int32_t result = 0;

				return (check_abi_result(this->self_abi().mask_index(put_abi(result))), result);
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
			void set_x(float input)
			{
				check_abi_result(this->self_abi().set_x(get_abi(input)));
			}
			void set_y(float input)
			{
				check_abi_result(this->self_abi().set_y(get_abi(input)));
			}
			void set_width(float input)
			{
				check_abi_result(this->self_abi().set_width(get_abi(input)));
			}
			void set_height(float input)
			{
				check_abi_result(this->self_abi().set_height(get_abi(input)));
			}
			void set_confidence(float input)
			{
				check_abi_result(this->self_abi().set_confidence(get_abi(input)));
			}
			void set_has_mask(float input)
			{
				check_abi_result(this->self_abi().set_has_mask(get_abi(input)));
			}
			void set_is_alive(std::int32_t input)
			{
				check_abi_result(this->self_abi().set_is_alive(get_abi(input)));
			}
			void set_glass_index(std::int32_t input)
			{
				check_abi_result(this->self_abi().set_glass_index(get_abi(input)));
			}
            void set_mask_index(std::int32_t input)
			{
				check_abi_result(this->self_abi().set_mask_index(get_abi(input)));
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
