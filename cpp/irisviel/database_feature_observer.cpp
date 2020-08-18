#include "database_feature_observer.hpp"

namespace glasssix
{
	namespace irisviel
	{
		int database_feature_observer::dimension() const noexcept
		{
			return dimension_;
		}

		std::vector<const float*> database_feature_observer::operator()() const
		{
			return retriever_();
		}
	}
}
