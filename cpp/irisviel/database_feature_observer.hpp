#pragma once

#include <vector>
#include <utility>
#include <functional>

namespace glasssix
{
	namespace irisviel
	{
		class database_feature_observer
		{
		public:
			struct feature
			{
				const float* data;
				float feature_modulo_length;
			};

			template<typename Retriever>
			database_feature_observer(Retriever&& retriever, int dimension) : dimension_{ dimension }, retriever_{ std::forward<Retriever>(retriever) }
			{
			}

			int dimension() const noexcept;
			std::vector<feature> operator()() const;
		private:
			int dimension_;
			std::function<std::vector<database_feature_observer::feature>()> retriever_;
		};
	}
}
