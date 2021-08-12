#ifndef _IRISVIEL_SEARCH_HPP_
#define _IRISVIEL_SEARCH_HPP_

#include "irisviel_types.hpp"

#include <tuple>
#include <vector>
#include <memory>

namespace glasssix
{
	class mutex_wrapper;

	namespace irisviel
	{
		class index_builder;
		class irisviel_search_internal;

		/// <summary>
		/// Provides support for fast face databasing and searching.
		/// </summary>
		class irisviel_search
		{
		public:
			/// <summary>
			/// Creates an instance.
			/// </summary>
			/// <param name="base_data">The base data stored in separate vectors</param>
			/// <param name="dimension">The dimension of the vector, e.g. 128 or 512</param>
			irisviel_search(const std::vector<const float*>& base_data, int dimension);

			/// <summary>
			/// Creates an instance.
			/// </summary>
			/// <param name="dimension">The dimension of the vector, e.g. 128 or 512</param>
			irisviel_search(int dimension);

			/// <summary>
			/// Creates an instance.
			/// </summary>
			/// <param name="base_data">The base data stored in separate vectors</param>
			/// <param name="dimension">The dimension of the vector, e.g. 128 or 512</param>
			/// <param name="lock">The shared lock for synchronization</param>
			irisviel_search(const std::vector<const float*>& base_data, int dimension, const std::shared_ptr<mutex_wrapper>& lock);
			
			/// <summary>
			/// Creates an instance.
			/// </summary>
			/// <param name="dimension">The dimension of the vector, e.g. 128 or 512</param>
			/// <param name="lock">The shared lock for synchronization</param>
			irisviel_search(int dimension, const std::shared_ptr<mutex_wrapper>& lock); 
			irisviel_search(const irisviel_search&) = delete;
			irisviel_search& operator=(const irisviel_search&) = delete;

			virtual ~irisviel_search();

			/// <summary>
			/// Builds the graph which depends on the loaded base data.
			/// </summary>
			/// <returns>The maximum memory usage during the operation, just for debugging purposes</returns>
			int build_graph() const;

			/// <summary>
			/// Builds the graph with the newly specified base data.
			/// </summary>
			/// <returns>The maximum memory usage during the operation, just for debugging purposes</returns>
			int build_graph(const std::vector<const float*>& base_data) const;

			/// <summary>
			/// Saves the built graph to the disk.
			/// </summary>
			/// <param name="graph_path">The file path</param>
			void save_graph(const char* graph_path) const;

			/// <summary>
			/// Saves both the built graph and the base data to the disk.
			/// </summary>
			/// <param name="graph_path">The file path</param>
			/// <param name="base_data_path">The base data path</param>
			void save_graph(const char* graph_path, const char* base_data_path) const;

			/// <summary>
			/// Loads a graph from the disk.
			/// </summary>
			/// <param name="graph_path">The graph path</param>
			/// <returns>True if the operation succeeds; otherwise false</returns>
			bool load_graph(const char* graph_path) const;

			/// <summary>
			/// Gets the current base data.
			/// </summary>
			/// <returns>The current base data</returns>
			const std::vector<const float*>* base_data() const;

			/// <summary>
			/// Loads both the graph and the base data from the disk.
			/// </summary>
			/// <param name="graph_path">The graph path</param>
			/// <param name="base_data_path">The base data path</param>
			/// <returns>True if the operation succeeds; otherwise false</returns>
			bool load_graph(const char* graph_path, const char* base_data_path) const;

			/// <summary>
			/// Optimizes the built graph.
			/// </summary>
			void optimize_graph() const;

			/// <summary>
			/// Searches one or more vectors in descending order of similarities.
			/// </summary>
			/// <param name="query_data">The vectors to be queried</param>
			/// <param name="top_k">The most similar K results to return</param>
			/// <returns>The indexes and similarities of the results</returns>
			vector2d<std::tuple<std::uint32_t, float>> search_vector(const std::vector<const float*>& query_data, uint32_t top_k) const;
			
			/// <summary>
			/// Saves the results to the disk.
			/// </summary>
			/// <param name="path">The file path</param>
			/// <param name="return_ids">The indexes of the results</param>
			void save_result(const char* path, const std::vector<std::vector<uint32_t>>& return_ids) const;

			/// <summary>
			/// Gets the current version of the library.
			/// </summary>
			/// <returns>The current version of the library</returns>
			static const char* get_version();
		private:
			std::shared_ptr<index_builder> index_;
			std::shared_ptr<mutex_wrapper> mutex_wrapper_;
			std::shared_ptr<irisviel_search_internal> search_;
		};
	}
}

#endif // !_IRISVIEL_SEARCH_HPP_