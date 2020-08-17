#pragma once

#ifndef _SEARCH_HPP_
#define _SEARCH_HPP_

#include "distance.hpp"
#include "ngraph_internal.hpp"
#include "kgraph_internal.hpp"
#include "irisviel_types.hpp"
#include "Primitives/tensor.hpp"

#include <tuple>
#include <cstdint>

namespace glasssix
{
	namespace irisviel
	{
		class irisviel_search_internal
		{
		public:
			irisviel_search_internal(const std::vector<const float*>& base_data, int dimension);
			irisviel_search_internal(int dimension);
			virtual ~irisviel_search_internal();

			bool load_graph(const char* graph_path);
			bool load_graph(const char* graph_path, const char* base_data_path);
			const std::vector<const float*>* get_base_data();
			void optimize_graph();
			std::tuple<vector2d<uint32_t>, vector2d<float>> search_vector(const std::vector<const float*>& query_data, uint32_t top_k);
			void save_result(const char* path, const std::vector<std::vector<uint32_t> >& return_ids);

			uint32_t navigate_node = 0;
			uint32_t width = 0;
			bool normalized = false;
			std::vector<std::vector<uint32_t>> ngraph;
			const std::vector<const float*>* base_data;
			uint32_t base_num_;
		private:
			uint32_t dimension_;
			uint32_t query_num_;
			std::vector<const float*> base_data_cache_;
			const std::vector<const float*>* query_data_;
			std::shared_ptr<memory::tensor<char>> opt_graph_tensor_;

			char* opt_graph_ = nullptr;

			size_t node_size_;
			size_t data_len_;
			size_t neighbor_len_;

			uint32_t neighbors_max_length = 0;
			using compact_graph_type = std::vector<std::vector<uint32_t>>;
			void search_with_opt_graph(const float* single_query_data, uint32_t top_k, std::vector<uint32_t>& return_ids, std::vector<float>& return_similarities);
		};
	}
}

#endif // !_SEARCH_HPP_