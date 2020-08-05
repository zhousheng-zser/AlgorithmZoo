#include "GaiusFeature.hpp"
#include "unicorn_mobile.hpp"
#include "unicorn_mobile_mask.hpp"
#include <memory>

namespace glasssix
{
	namespace gaius
	{
		class GaiusFeature::impl
		{
		public:
			impl() : impl{ -1 }
			{
			}

			impl(int device) : mobile_unicornia_{ std::make_shared<Unicorn_mobile>(device) }, 
				               mobile_unicornia_mask_{ std::make_shared<Unicorn_mobile_mask>(device) }
			{
			}

			virtual ~impl() = default;

			std::vector<std::vector<float>> Forward(const std::uint8_t* input_data, unsigned num, int order = 0, bool mask = false) const
			{
				if (mask)
				{
					return mobile_unicornia_mask_->Forward(input_data, num, order);
				}
				else
				{
					return mobile_unicornia_->Forward(input_data, num, order);
				}				
			}

			static const char* getVersion()
			{
#ifdef TRIAL
				return "Glasssix Trial FaceSDK";
#else
				return "Glasssix";
#endif // TRIAL
			}
		private:
			std::vector<std::vector<float>> Forward(const float* input_data, unsigned num, int order = 0, bool mask = false) const
			{
				if (mask)
				{
					return mobile_unicornia_mask_->Forward(input_data, num, order);
				}
				else
				{
					return mobile_unicornia_->Forward(input_data, num, order);
				}
			}

			std::shared_ptr<Unicorn_mobile> mobile_unicornia_;
			std::shared_ptr<Unicorn_mobile_mask> mobile_unicornia_mask_;
		};

		GaiusFeature::GaiusFeature() : impl_{ new impl }
		{
		}

		GaiusFeature::GaiusFeature(int device) : impl_{ new impl{ device } }
		{
		}

		GaiusFeature::~GaiusFeature()
		{
			if (impl_ != nullptr)
			{
				delete impl_;
				impl_ = nullptr;
			}
		}

		std::vector<std::vector<float>> GaiusFeature::Forward(const std::uint8_t* input_data, unsigned num, int order, bool mask) const
		{
			return impl_->Forward(input_data, num, order, mask);
		}

		const char* GaiusFeature::getVersion()
		{
			return impl::getVersion();
		}
	}
}
