#pragma once

#ifndef _INDEX_HPP_
#define _INDEX_HPP_

#include "distance.hpp"
#include "ngraph_internal.hpp"
#include "kgraph_internal.hpp"
#include "Primitives/tensor.hpp"

namespace glasssix 
{
	namespace irisviel
	{
		class index_builder
		{
		public:
			index_builder();
			index_builder(const std::vector<const float*>& base_data, int dimension);
			index_builder(int dimension);
			virtual ~index_builder();

			int build_graph();
			int build_graph(const std::vector<const float*>& base_data);
			void save_graph(const char* ngraph_path);
			void save_graph(const char* ngraph_path, const char* base_data_path);

			bool normalized;
			uint32_t width;
			uint32_t base_num;
			uint32_t navigate_node;
			const std::vector<const float*>* base_data;
			std::vector<std::vector<uint32_t>> final_graph;
		private:
			kgraph_internal kgraph_;
			ngraph_internal ngraph_;
			float *norm_array_;
			uint32_t dimension_;
			std::shared_ptr<glasssix::memory::tensor<float>> norm_array_tensor_;
		};
	}
}
#endif // !_INDEX_HPP_
