#include "ngraph_internal.hpp"
#include "Primitives/tensor.hpp"
#include "distance.hpp"

#include <stack>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace glasssix
{
	namespace irisviel
	{
		void ngraph_internal::get_navigate_node(const float* approximate_center, std::vector<neighbor>* pnns)
		{
			int testNum = std::min(10, (int)base_num);
			std::vector<neighbor> neighbors(testNum + 1);
			uint32_t tempK = 0;
			for (uint32_t i = 0; i < base_num; ++i)
			{
				float dist = distance_l2::compare((*base_data).at(i), approximate_center, (uint32_t)dimension);
				neighbor nn = neighbor{ i, dist, true };
				insert_into_pool(&neighbors[0], tempK, nn);
				if (tempK < testNum) ++tempK;
			}
			neighbors.resize(testNum);
			pnns->swap(neighbors);
		}

		void ngraph_internal::get_neighbors(
			uint32_t query_id,
			std::vector <neighbor>& pool, std::vector <neighbor>& fullset)
		{
			pool.resize(neighbors_max_length + 1);
			std::vector<uint32_t> init_ids(neighbors_max_length);
			boost::dynamic_bitset<> flags{ base_num, 0 };
			uint32_t count = 0;

			for (uint32_t i = 0; i < init_ids.size() && i < final_graph[navigate_node].size(); i++)
			{
				init_ids[i] = final_graph[navigate_node][i];
				flags[init_ids[i]] = true;
				count++;
			}
			while (count < init_ids.size())
			{
				uint32_t id = rand() % base_num;
				if (flags[id])continue;
				init_ids[count] = id;
				count++;
				flags[id] = true;
			}

			float norm_query = norm_array[query_id];
			for (uint32_t i = 0; i < init_ids.size(); i++)
			{
				uint32_t id = init_ids[i];
				float norm_id = norm_array[id];

#ifdef COSINE_DISTANCE
				float dist = distance_cosine::compare((*base_data).at(id), norm_id, (*base_data).at(query_id), norm_query, dimension);
#else
				float dist = distance_fast_l2::compare((*base_data).at(id), norm_id, (*base_data).at(query_id), norm_query, dimension);
#endif // COSINE_DISTANCE

				pool[i] = neighbor{ id, dist, true };
			}

			std::sort(pool.begin(), pool.begin() + neighbors_max_length);
			int i = 0;
			while (i < (int)neighbors_max_length)
			{
				int minPos = neighbors_max_length;

				if (pool[i].flag)
				{
					pool[i].flag = false;
					uint32_t n = pool[i].id;

					for (uint32_t m = 0; m < final_graph[n].size(); ++m)
					{
						uint32_t id = final_graph[n][m];
						if (flags[id])continue;
						flags[id] = 1;

						float norm_id = norm_array[id];

#ifdef COSINE_DISTANCE
						float dist = distance_cosine::compare((*base_data).at(id), norm_id, (*base_data).at(query_id), norm_query, dimension);
#else
						float dist = distance_fast_l2::compare((*base_data).at(id), norm_id, (*base_data).at(query_id), norm_query, dimension);
#endif // COSINE_DISTANCE

						neighbor nn(id, dist, true);
						fullset.push_back(nn);

						if (dist >= pool[neighbors_max_length - 1].distance)continue;
						int insert_pos = insert_into_pool(&pool[0], neighbors_max_length, nn);
						if (pool.size() > neighbors_max_length + 1)
						{
							pool.pop_back();
						}

						if (insert_pos < minPos)
						{
							minPos = insert_pos;
						}
					}
				}
				if (minPos <= i)
				{
					i = minPos;
				}
				else
				{
					++i;
				}
			}
		}

		void ngraph_internal::init_graph()
		{

#ifdef COSINE_DISTANCE

			if (dimension <= 128)
			{
				if (base_num <= 50000)
				{
					neighbors_max_length = 50;
					range = 30;
				}
				else if (base_num <= 100000)
				{
					neighbors_max_length = 50;
					range = 30;
				}
				else if (base_num <= 200000)
				{
					neighbors_max_length = 50;
					range = 40;
				}
				else if (base_num <= 500000)
				{
					neighbors_max_length = 60;
					range = 60;
				}
				else
				{
					neighbors_max_length = 60;
					range = 60;
				}
			}
			else
			{
				if (base_num <= 50000)
				{
					neighbors_max_length = 20;
					range = 20;
				}
				else if (base_num <= 100000)
				{
					neighbors_max_length = 40;
					range = 40;
				}
				else if (base_num <= 200000)
				{
					neighbors_max_length = 70;
					range = 70;
				}
				else if (base_num <= 500000)
				{
					neighbors_max_length = 40;
					range = 40;
				}
				else
				{
					neighbors_max_length = 40;
					range = 40;
				}
			}

#else

			if (dimension <= 128)
			{
				if (base_num <= 50000)
				{
					neighbors_max_length = 50;
					range = 30;
				}
				else if (base_num <= 100000)
				{
					neighbors_max_length = 50;
					range = 40;
				}
				else if (base_num <= 200000)
				{
					neighbors_max_length = 80;
					range = 80;
				}
				else if (base_num <= 500000)
				{
					neighbors_max_length = 60;
					range = 60;
				}
				else
				{
					neighbors_max_length = 60;
					range = 60;
				}
			}
			else
			{
				if (base_num <= 50000)
				{
					neighbors_max_length = 20;
					range = 20;
				}
				else if (base_num <= 100000)
				{
					neighbors_max_length = 40;
					range = 40;
				}
				else if (base_num <= 200000)
				{
					neighbors_max_length = 70;
					range = 70;
				}
				else if (base_num <= 500000)
				{
					neighbors_max_length = 40;
					range = 40;
				}
				else
				{
					neighbors_max_length = 40;
					range = 40;
				}
			}

#endif // COSINE_DISTANCE

			if (base_num <= neighbors_max_length)
			{
				//LOG(WARNING) << "Warning: small dataset, shrinking neighborsMaxLength to " << base_num << ".";
				neighbors_max_length = base_num;
			}

			if (base_num <= range)
			{
				//LOG(WARNING) << "Warning: small dataset, shrinking range to " << base_num << ".";
				range = base_num;
			}

			std::shared_ptr<memory::tensor<float>> center_tensor = std::make_shared<memory::tensor<float>>(dimension);
			float* center = center_tensor->mutable_cpu_data();
			for (uint32_t j = 0; j < dimension; j++)
			{
				center[j] = 0;
			}
			for (uint32_t i = 0; i < base_num; i++)
			{
				for (uint32_t j = 0; j < dimension; j++)
				{
					center[j] += (*base_data).at(i)[j];
				}
			}
			for (uint32_t j = 0; j < dimension; j++)
			{
				center[j] /= base_num;
			}
			std::vector <neighbor> pool;
			get_navigate_node(center, &pool);
			navigate_node = pool[0].id;
		}

		void ngraph_internal::add_to_graph(uint32_t destination_id, neighbor new_comer, locked_graph_type& cut_graph)
		{
			lock_guard_type guard(cut_graph[destination_id].lock);
			for (uint32_t i = 0; i < cut_graph[destination_id].pool.size(); i++)
			{
				if (new_comer.id == cut_graph[destination_id].pool[i].id)
					return;
			}
			cut_graph[destination_id].pool.push_back(new_comer);
			if (cut_graph[destination_id].pool.size() > range)
			{
				std::vector <neighbor> result;
				std::vector <neighbor>& pool = cut_graph[destination_id].pool;
				uint32_t start = 0;
				std::sort(pool.begin(), pool.end());
				result.push_back(pool[start]);

				while (result.size() < range && (++start) < pool.size())
				{
					auto& candidate = pool[start];
					bool signal = false;
					for (uint32_t i = 0; i < result.size(); i++)
					{
						if (candidate.id == result[i].id)
						{
							signal = true;
							break;
						}

						//three points(candidate,queryID and result[i].id) construct a triangle,
						//MRNG strategy will be satisfied(for all result[i]) when edge cr(edge between candidata and result[i].id) is the longest
						float norm_result_id = norm_array[result[i].id];
						float norm_candidate_id = norm_array[candidate.id];

#ifdef COSINE_DISTANCE
						float distance = distance_cosine::compare((*base_data).at(result[i].id), norm_result_id,
							(*base_data).at(candidate.id), norm_candidate_id, dimension);
#else
						float distance = distance_fast_l2::compare((*base_data).at(result[i].id), norm_result_id,
							(*base_data).at(candidate.id), norm_candidate_id, dimension);
#endif // COSINE_DISTANCE

						if (distance < candidate.distance)
						{
							signal = true;
							break;
						}

					}
					if (!signal)
						result.push_back(candidate);
				}
				pool.swap(result);
			}
		}

		void ngraph_internal::edge_prune(uint32_t query_id, std::vector <neighbor>& fullset, locked_graph_type& cut_graph)
		{
			width = range;
			uint32_t start = 0;

			boost::dynamic_bitset<> flags{ base_num, 0 };
			for (uint32_t i = 0; i < fullset.size(); i++)
			{
				flags[fullset[i].id] = 1;
			}

			float normQuery = norm_array[query_id];
			for (uint32_t nn = 0; nn < final_graph[query_id].size(); nn++)
			{
				uint32_t id = final_graph[query_id][nn];
				if (flags[id])continue;

				float norm_id = norm_array[id];

#ifdef COSINE_DISTANCE
				float dist = distance_cosine::compare((*base_data).at(query_id), normQuery,
					(*base_data).at(id), norm_id, dimension);
#else

				float dist = distance_fast_l2::compare((*base_data).at(query_id), norm_query,
					(*base_data).at(id), norm_id, dimension);
#endif // COSINE_DISTANCE

				fullset.push_back(neighbor(id, dist, true));
			}

			std::sort(fullset.begin(), fullset.end());
			std::vector <neighbor> result;
			if (fullset[start].id == query_id)start++;
			result.push_back(fullset[start]);

			while (result.size() < range && (++start) < fullset.size())
			{
				auto& candidate = fullset[start];
				bool signal = false;
				for (uint32_t i = 0; i < result.size(); i++)
				{
					if (candidate.id == result[i].id)
					{
						signal = true;
						break;
					}
					//three points(candidate,queryID and result[i].id) construct a triangle,
					//MRNG strategy will be satisfied(for all result[i]) when edge cr(edge between candidata and result[i].id) is the longest
					float normResultID = norm_array[result[i].id];
					float normCandidateID = norm_array[candidate.id];

#ifdef COSINE_DISTANCE
					float distance = distance_cosine::compare((*base_data).at(result[i].id), normResultID,
						(*base_data).at(candidate.id), normCandidateID, dimension);
#else
					float distance = distance_fast_l2::compare((*base_data).at(result[i].id), norm_result_id,
						(*base_data).at(candidate.id), norm_candidate_id, dimension);
#endif // COSINE_DISTANCE

					if (distance < candidate.distance)
					{
						signal = true;
						break;
					}
				}
				if (!signal)result.push_back(candidate);
			}

			for (uint32_t i = 0; i < result.size(); i++)
			{
				add_to_graph(query_id, result[i], cut_graph);
				add_to_graph(result[i].id, neighbor(query_id, result[i].distance, true), cut_graph);
			}
		}

		void ngraph_internal::link(locked_graph_type& cutGraph_)
		{
#ifdef _OPENMP
#pragma omp parallel for
#endif
			for (int n = 0; n < base_num; ++n)
			{
				std::vector <neighbor> pool, fullset;
				get_neighbors(n, pool, fullset);
				edge_prune(n, fullset, cutGraph_);
			}
		}

		void ngraph_internal::build()
		{
			init_graph();//find out navigateNode
			locked_graph_type cutGraph_(base_num);
			link(cutGraph_);//for each point, find nearest neighbors to navigateNode, then execute prune strategy
			final_graph.resize(base_num);

			uint32_t max = 0, min = 1e6, avg = 0, cnt = 0;

			for (uint32_t i = 0; i < base_num; i++)
			{
				auto& pool = cutGraph_[i].pool;
				final_graph[i].resize(pool.size());
				for (uint32_t j = 0; j < pool.size(); j++)
				{
					final_graph[i][j] = pool[j].id;
				}

				max = max < pool.size() ? pool.size() : max;
				min = min > pool.size() ? pool.size() : min;
				avg += pool.size();
				if (pool.size() < 2) cnt++;
			}

			tree_grow();

			avg /= base_num;
			max = 0;
			for (uint32_t i = 0; i < base_num; i++)
			{
				max = max < final_graph[i].size() ? final_graph[i].size() : max;
			}
			if (max > width)
				width = max;
		}

		void ngraph_internal::invoke_dfs(boost::dynamic_bitset<>& flag, uint32_t root, uint32_t& cnt)
		{
			uint32_t tmp = root;
			std::stack<uint32_t> s;
			s.push(root);
			if (!flag[root])
			{
				cnt++;
			}

			flag[root] = true;

			while (!s.empty())
			{
				uint32_t next = base_num + 1;
				for (uint32_t i = 0; i < final_graph[tmp].size(); i++)
				{
					if (flag[final_graph[tmp][i]] == false)
					{
						next = final_graph[tmp][i];
						break;
					}
				}
				if (next == (base_num + 1))
				{
					s.pop();
					if (s.empty())
						break;
					tmp = s.top();
					continue;
				}
				tmp = next;
				flag[tmp] = true; s.push(tmp); cnt++;
			}
		}

		void ngraph_internal::find_root(boost::dynamic_bitset<>& flag, uint32_t& root)
		{
			uint32_t id;
			for (uint32_t i = 0; i < base_num; i++)
			{
				if (flag[i] == false)
				{
					id = i;
					break;
				}
			}
			std::vector<neighbor> pool;
			std::vector<neighbor> fullset;

			get_neighbors(id, pool, fullset);
			std::sort(pool.begin(), pool.end());

			uint32_t found = 0;
			for (uint32_t i = 0; i < pool.size(); i++)
			{
				if (flag[pool[i].id])
				{
					root = pool[i].id;
					found = 1;
					break;
				}
			}
			if (found == 0)
			{
				while (true)
				{
					uint32_t rid = rand() % base_num;
					if (flag[rid])
					{
						root = rid;
						break;
					}
				}
			}
			final_graph[root].push_back(id);
		}

		void ngraph_internal::tree_grow()
		{
			uint32_t root = navigate_node;
			boost::dynamic_bitset<> flags{ base_num, 0 };
			uint32_t unlinked_cnt = 0;

			while (unlinked_cnt < base_num)
			{
				invoke_dfs(flags, root, unlinked_cnt);
				if (unlinked_cnt >= base_num)
				{
					break;
				}

				find_root(flags, root);
			}
		}
	}
}