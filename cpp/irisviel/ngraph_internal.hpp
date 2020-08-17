#ifndef _NGRAPH_HPP_
#define _NGRAPH_HPP_

#include "neighbor.hpp"
#include "kgraph_internal.hpp"

#include <boost/dynamic_bitset.hpp>

namespace glasssix
{
	namespace irisviel
	{
		struct index_header
		{
			uint64_t file_size;
			bool normalized;
			uint32_t graph_width;
			uint32_t navigate_node;
		};

		struct ngraph_internal
		{
			ngraph_internal() : base_data{}, base_num{}, dimension{}, norm_array{}, width{}, navigate_node{}, neighbors_max_length{}, range{}
			{
			}

			~ngraph_internal()
			{
			}

			void init_graph();
			void get_navigate_node(const float* approximate_center, std::vector<neighbor>* pnns);
			void get_neighbors(uint32_t query_id, std::vector<neighbor>& pool, std::vector<neighbor>& fullset);
			void add_to_graph(uint32_t destination_id, neighbor new_comer, locked_graph_type& cut_graph);
			void edge_prune(uint32_t query_id, std::vector<neighbor>& fullset, locked_graph_type& cut_graph);
			void build();
			void link(locked_graph_type& cut_graph);
			void tree_grow();
			void invoke_dfs(boost::dynamic_bitset<>& flag, uint32_t root, uint32_t& cnt);
			void find_root(boost::dynamic_bitset<>& flag, uint32_t& root);

			const std::vector<const float*>* base_data;
			uint32_t base_num;
			uint32_t dimension;
			const float* norm_array;
			using compact_graph_type = std::vector<std::vector<uint32_t>>;
			compact_graph_type final_graph;
			uint32_t width;
			uint32_t navigate_node;
			uint32_t neighbors_max_length;//max number of neighbors
			uint32_t range;//max number of edges followed by MRNG strategy
		};
	}
}

#endif // !_NGRAPH_HPP_