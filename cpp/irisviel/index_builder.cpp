#include "index_builder.hpp"

#include <cmath>
#include <fstream>
#include <iostream>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace glasssix
{
	namespace irisviel
	{
		index_builder::index_builder() : dimension_{}
		{
		}

		index_builder::index_builder(const std::vector<const float*>& baseData, int dimension)
			: dimension_{ static_cast<uint32_t>(dimension) }, base_data{ &baseData }, base_num{ static_cast<uint32_t>(baseData.size()) }
		{
			norm_array_tensor_.reset(new glasssix::memory::tensor<float>(base_num));
			norm_array_ = norm_array_tensor_->mutable_cpu_data();

			//use 10 data to judge if normalized
			float sum = 0;
			int calcNum = std::min(10, (int)base_num);
			for (int i = 0; i < calcNum; ++i)
			{
#ifdef COSINE_DISTANCE
				sum += distance_cosine::norm((*base_data).at(i), dimension_);
#else
				sum += distance_fast_l2::norm((*base_data).at(i), dimension);
#endif // COSINE_DISTANCE
			}

			if (abs(sum / calcNum - 1) <= 1e-5)
			{
				normalized = true;
#ifdef _OPENMP
#pragma omp parallel for
#endif
				for (int i = 0; i < base_num; ++i)
				{
					norm_array_[i] = 1;
				}
			}
			else
			{
#ifdef _OPENMP
#pragma omp parallel for
#endif
				for (int i = 0; i < base_num; ++i)
				{
#ifdef COSINE_DISTANCE
					norm_array_[i] = distance_cosine::norm((*base_data).at(i), dimension_);
#else
					norm_array[i] = distance_fast_l2::norm((*base_data).at(i), dimension);
#endif // COSINE_DISTANCE
				}
			}

			kgraph_.base_data = base_data;
			kgraph_.base_num = base_num;
			kgraph_.dimension = dimension_;
			kgraph_.norm_array = norm_array_;

			ngraph_.base_data = base_data;
			ngraph_.base_num = base_num;
			ngraph_.dimension = dimension_;
			ngraph_.norm_array = norm_array_;
		};

		index_builder::index_builder(int dimension)
			:dimension_(dimension)
		{
			kgraph_.dimension = dimension_;
			ngraph_.dimension = dimension_;
		}

		index_builder::~index_builder() {}

		int index_builder::build_graph()
		{
			int max_memory_usage = kgraph_.build();

			//transfer kgraph to nGraph's finalGraph_
			{
				std::vector<std::vector<neighbor>>& kgraphTemp = kgraph_.kgraph;
				ngraph_.final_graph.resize(kgraphTemp.size());
				for (uint32_t i = 0; i < kgraphTemp.size(); ++i)
				{
					auto const& neighbors = kgraphTemp[i];
					uint32_t size = neighbors.size();
					ngraph_.final_graph[i].resize(size);

					for (uint32_t j = 0; j < size; ++j)
					{
						ngraph_.final_graph[i][j] = neighbors[j].id;
					}
				}
			}

			ngraph_.build();

			width = ngraph_.width;
			navigate_node = ngraph_.navigate_node;

			final_graph.resize(ngraph_.final_graph.size());
			for (size_t i = 0; i < ngraph_.final_graph.size(); i++)
			{
				final_graph[i].resize(ngraph_.final_graph[i].size());
				for (size_t j = 0; j < ngraph_.final_graph[i].size(); j++)
				{
					final_graph[i][j] = ngraph_.final_graph[i][j];
				}
			}

			return max_memory_usage;
		}

		int index_builder::build_graph(const std::vector<const float*>& new_base_data)
		{
			base_data = &new_base_data;
			base_num = new_base_data.size();
			norm_array_tensor_.reset(new glasssix::memory::tensor<float>(base_num));
			norm_array_ = norm_array_tensor_->mutable_cpu_data();
			//use 10 data to judge if normalized
			float sum = 0;
			int calcNum = std::min(10, (int)base_num);
			for (int i = 0; i < calcNum; ++i)
			{
#ifdef COSINE_DISTANCE
				sum += distance_cosine::norm((*base_data).at(i), dimension_);
#else
				sum += distance_fast_l2::norm((*base_data).at(i), dimension);
#endif // COSINE_DISTANCE
			}

			if (std::abs(sum / calcNum - 1) <= 1e-5)
			{
				normalized = true;
#ifdef _OPENMP
#pragma omp parallel for
#endif
				for (int i = 0; i < base_num; ++i)
				{
					norm_array_[i] = 1;
				}
			}
			else
			{
#ifdef _OPENMP
#pragma omp parallel for
#endif
				for (int i = 0; i < base_num; ++i)
				{
#ifdef COSINE_DISTANCE
					norm_array_[i] = distance_cosine::norm((*base_data).at(i), dimension_);
#else
					norm_array[i] = distance_fast_l2::norm((*base_data).at(i), dimension);
#endif // COSINE_DISTANCE
				}
			}

			kgraph_.base_data = base_data;
			kgraph_.base_num = base_num;
			kgraph_.norm_array = norm_array_;

			ngraph_.base_data = base_data;
			ngraph_.base_num = base_num;
			ngraph_.norm_array = norm_array_;
			double elapsedTime = 0;

			int maxMemoryUsage = kgraph_.build();

			//transfer kgraph to nGraph's finalGraph_
			{
				std::vector<std::vector<neighbor>>& kgraph_temp = kgraph_.kgraph;
				ngraph_.final_graph.resize(kgraph_temp.size());
				for (uint32_t i = 0; i < kgraph_temp.size(); ++i)
				{
					auto const& neighbors = kgraph_temp[i];
					uint32_t size = neighbors.size();
					ngraph_.final_graph[i].resize(size);

					for (uint32_t j = 0; j < size; ++j)
					{
						ngraph_.final_graph[i][j] = neighbors[j].id;
					}
				}
			}

			ngraph_.build();

			width = ngraph_.width;
			navigate_node = ngraph_.navigate_node;

			final_graph.resize(ngraph_.final_graph.size());
			for (size_t i = 0; i < ngraph_.final_graph.size(); i++)
			{
				final_graph[i].resize(ngraph_.final_graph[i].size());
				for (size_t j = 0; j < ngraph_.final_graph[i].size(); j++)
				{
					final_graph[i][j] = ngraph_.final_graph[i][j];
				}
			}

			return maxMemoryUsage;
		}

		void index_builder::save_graph(const char* ngraph_path)
		{
			std::ofstream out(ngraph_path, std::ios::trunc | std::ios::binary);
			std::vector<std::vector<uint32_t>>& tempGraph = ngraph_.final_graph;
			assert(tempGraph.size() == base_num);

			out.write(reinterpret_cast<char const*>(&normalized), sizeof(bool));
			out.write(reinterpret_cast<char const*>(&(ngraph_.width)), sizeof(uint32_t));
			out.write(reinterpret_cast<char const*>(&(ngraph_.navigate_node)), sizeof(uint32_t));
			for (uint32_t i = 0; i < base_num; i++)
			{
				uint32_t GK = (uint32_t)tempGraph[i].size();
				out.write(reinterpret_cast<char const*>(&GK), sizeof(uint32_t));
				out.write(reinterpret_cast<char const*>(&tempGraph[i][0]), GK * sizeof(uint32_t));
				if (i % 100 == 0)
				{
					out.flush();
				}
			}
			out.close();
		}

		void index_builder::save_graph(const char* ngraph_path, const char* base_data_path)
		{
			//save ngraph
			std::ofstream outGraph(ngraph_path, std::ios::trunc | std::ios::binary);
			std::vector<std::vector<uint32_t>>& tempGraph = ngraph_.final_graph;
			assert(tempGraph.size() == base_num);

			outGraph.write(reinterpret_cast<char const*>(&normalized), sizeof(bool));
			outGraph.write(reinterpret_cast<char const*>(&(ngraph_.width)), sizeof(uint32_t));
			outGraph.write(reinterpret_cast<char const*>(&(ngraph_.navigate_node)), sizeof(uint32_t));
			for (uint32_t n = 0; n < base_num; n++)
			{
				uint32_t GK = (uint32_t)tempGraph[n].size();
				outGraph.write(reinterpret_cast<char const*>(&GK), sizeof(uint32_t));
				outGraph.write(reinterpret_cast<char const*>(&tempGraph[n][0]), GK * sizeof(uint32_t));
				if (n % 100 == 0)
				{
					outGraph.flush();
				}
			}
			outGraph.close();

			//save basedata
			std::ofstream outBaseData(base_data_path, std::ios::trunc | std::ios::binary);
			for (uint32_t n = 0; n < base_num; n++)
			{
				outBaseData.write(reinterpret_cast<char const*>(&(*base_data)[n][0]), dimension_ * sizeof(float));
				if (n % 100 == 0)
				{
					outBaseData.flush();
				}
			}
			outBaseData.close();
		}

	}
}