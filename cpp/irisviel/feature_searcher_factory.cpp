#include "feature_searcher_factory.hpp"

#include <unordered_map>

namespace glasssix::irisviel
{
	namespace
	{
		std::unordered_map<face_service_implemention, std::function<std::shared_ptr<feature_searcher>(int)>> registered_factories;
	}

	std::shared_ptr<feature_searcher> make_shared_feature_searcher(face_service_implemention implementation, int dimension)
	{
		auto iter = registered_factories.find(implementation);

		return iter != registered_factories.end() ? iter->second(dimension) : nullptr;
	}

	void register_feature_searcher(face_service_implemention implementation, const std::function<std::shared_ptr<feature_searcher>(int)>& shared_maker)
	{
		registered_factories.insert_or_assign(implementation, shared_maker);
	}
}
