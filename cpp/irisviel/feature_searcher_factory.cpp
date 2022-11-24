#include "feature_searcher_factory.hpp"

#include <unordered_map>

namespace glasssix::irisviel
{
	namespace
	{
		auto& get_map()
		{
			static std::unordered_map<face_service_implemention, std::function<std::shared_ptr<feature_searcher>(int,std::string)>> registered_factories;

			return registered_factories;
		}
	}

	std::shared_ptr<feature_searcher> make_shared_feature_searcher(face_service_implemention implementation, int dimension)
	{
		auto iter = get_map().find(implementation);

		return iter != get_map().end() ? iter->second(dimension,"") : nullptr;
	}

	std::shared_ptr<feature_searcher> make_shared_feature_searcher(face_service_implemention implementation, int dimension,std::string path)
	{
		auto iter = get_map().find(implementation);
		
		return (iter != get_map().end()&& face_service_implemention::lsh_algorithm == implementation )? iter->second(dimension,path) : nullptr;
	}

	void register_feature_searcher(face_service_implemention implementation, const std::function<std::shared_ptr<feature_searcher>(int,std::string)>& shared_maker)
	{
		get_map().insert_or_assign(implementation, shared_maker);
	}
}
