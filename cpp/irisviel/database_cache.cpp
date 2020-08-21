#include "database_cache.hpp"

namespace glasssix
{
	namespace irisviel
	{
		database_cache::~database_cache()
		{
			// Deletes the disk file if they are empty.
			if (manager && manager->empty())
			{
				mark_for_deletion();
			}
		}

		database_cache::operator bool() const noexcept
		{
			return manager && wrapper;
		}

		void database_cache::commit() const
		{
			manager->save_changes();
			wrapper->build(true);
		}

		void database_cache::mark_for_deletion() noexcept
		{
			if (manager)
			{
				manager->mark_for_deletion();
			}

			if (wrapper)
			{
				wrapper->mark_for_deletion();
			}
		}
	}
}
