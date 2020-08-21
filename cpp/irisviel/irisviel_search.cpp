#include "index_builder.hpp"
#include "search.hpp"
#include "irisviel_search.hpp"
#include "Primitives/mutex_wrapper.hpp"

#include <iostream>

namespace glasssix
{
	namespace irisviel
	{
		irisviel_search::irisviel_search(const std::vector<const float*>& base_data, int dimension)
		{
			index_.reset(new index_builder{ base_data, dimension });
			search_.reset(new irisviel_search_internal{ base_data, dimension });
			mutex_wrapper_.reset(new mutex_wrapper);
		}

		irisviel_search::irisviel_search(int dimension)
		{
			index_.reset(new index_builder{ dimension });
			search_.reset(new irisviel_search_internal{ dimension });
			mutex_wrapper_.reset(new mutex_wrapper);
		}

		irisviel_search::irisviel_search(const std::vector<const float*>& base_data, int dimension, const std::shared_ptr<mutex_wrapper>& lock) : irisviel_search{ base_data, dimension }
		{
			mutex_wrapper_ = lock;
		}

		irisviel_search::irisviel_search(int dimension, const std::shared_ptr<mutex_wrapper>& lock)
			: irisviel_search(dimension)
		{
			mutex_wrapper_ = lock;
		}

		irisviel_search::~irisviel_search()
		{
		}

		int irisviel_search::build_graph() const
		{
			auto lock = mutex_wrapper_->guard();

			int max_memory_usage = index_->build_graph();
			search_->navigate_node = index_->navigate_node;
			search_->width = index_->width;
			search_->normalized = index_->normalized;
			search_->ngraph = index_->final_graph;

			search_->ngraph.resize(index_->final_graph.size());
			for (size_t i = 0; i < index_->final_graph.size(); i++)
			{
				search_->ngraph[i].resize(index_->final_graph[i].size());
				for (size_t j = 0; j < index_->final_graph[i].size(); j++)
				{
					search_->ngraph[i][j] = index_->final_graph[i][j];
				}
			}

			return max_memory_usage;
		}


		int irisviel_search::build_graph(const std::vector<const float*>& base_data) const
		{
			auto lock = mutex_wrapper_->guard();

			int max_memory_usage = index_->build_graph(base_data);
			search_->navigate_node = index_->navigate_node;
			search_->width = index_->width;
			search_->normalized = index_->normalized;
			search_->ngraph = index_->final_graph;
			search_->base_num_ = index_->base_num;
			search_->base_data = index_->base_data;

			search_->ngraph.resize(index_->final_graph.size());
			for (size_t i = 0; i < index_->final_graph.size(); i++)
			{
				search_->ngraph[i].resize(index_->final_graph[i].size());
				for (size_t j = 0; j < index_->final_graph[i].size(); j++)
				{
					search_->ngraph[i][j] = index_->final_graph[i][j];
				}
			}

			return max_memory_usage;
		}


		void irisviel_search::save_graph(const char* graph_path) const
		{
			auto lock = mutex_wrapper_->guard();

			index_->save_graph(graph_path);
		}

		void irisviel_search::save_graph(const char* graph_path, const char* base_data_path) const
		{
			auto lock = mutex_wrapper_->guard();

			index_->save_graph(graph_path, base_data_path);
		}

		bool irisviel_search::load_graph(const char* graph_path) const
		{
			auto lock = mutex_wrapper_->guard();

			return search_->load_graph(graph_path);
		}

		// For C++/CLI, implementations shuold not be done in header files.
		// Or it will report C2001 bugs for the un-support functions.

		bool irisviel_search::load_graph(const char* graph_path, const char* base_data_path) const
		{
			auto lock = mutex_wrapper_->guard();

			return search_->load_graph(graph_path, base_data_path);
		}

		const std::vector<const float*>* irisviel_search::base_data() const
		{
			auto lock = mutex_wrapper_->guard();

			return search_->get_base_data();
		}

		void irisviel_search::optimize_graph() const
		{
			auto lock = mutex_wrapper_->guard();

			search_->optimize_graph();
		}

		std::tuple<vector2d<uint32_t>, vector2d<float>> irisviel_search::search_vector(const std::vector<const float*>& query_data, uint32_t top_k)
		{
			auto lock = mutex_wrapper_->guard();

			return search_->search_vector(query_data, top_k);
		}

		void irisviel_search::save_result(const char* path, const std::vector<std::vector<uint32_t>>& return_ids) const
		{
			auto lock = mutex_wrapper_->guard();

			search_->save_result(path, return_ids);
		}

		const char* irisviel_search::get_version()
		{
#ifdef TRIAL
			return "Glasssix Trial FaceSDK";
#else
			return "Glasssix";
#endif // TRIAL	
		}
	}
}