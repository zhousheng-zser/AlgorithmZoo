#pragma once

#include "feature_searcher.hpp"
#include "face_service_implemention.hpp"

#include <memory>
#include <functional>

namespace glasssix::irisviel
{
	std::shared_ptr<feature_searcher> make_shared_feature_searcher(face_service_implemention implementation, int dimension);
	void register_feature_searcher(face_service_implemention implementation, const std::function<std::shared_ptr<feature_searcher>(int)>& shared_maker);
}
