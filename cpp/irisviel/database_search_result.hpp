#pragma once

#include "database_record.hpp"

namespace glasssix
{
	namespace irisviel
	{
		struct database_search_result
		{
			std::shared_ptr<database_record> data;
			float similarity;
		};
	}
}
