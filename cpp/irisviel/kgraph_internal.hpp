#ifndef _KGRAPH_HPP_
#define _KGRAPH_HPP_

#include "neighbor.hpp"

namespace glasssix
{
	namespace irisviel
	{
		namespace
		{
			constexpr uint32_t default_k = 40;
			constexpr uint32_t default_iterations = 30;
			constexpr float default_delta = 0.0002f;
			constexpr float default_recall = 0.99f;

			///<summary>
			/// The maximum number of neighbors per each point in the KGraph result.
			///</summary>
			constexpr uint32_t default_pool_size = 90;

			///<summary>
			/// The maximum number of reversed neighbors per each point in the KGraph result.
			///</summary>
			constexpr uint32_t default_reverse_pool_size = 90;
		}

		struct kgraph_internal
		{
			///<summary>
			/// Indexing parameters.
			///</summary>
			struct index_params
			{
				uint32_t k;
				uint32_t pool_size;
				uint32_t reverse_pool_size;
				uint32_t iterations;
				float delta;
				float recall;

				/// Construct with default values.
				index_params() : iterations{ default_iterations }, pool_size{ default_pool_size }, k{ default_k }, reverse_pool_size{ default_reverse_pool_size }, delta{ default_delta }, recall{ default_recall }
				{
				}
			};

			///<summary>
			/// Information and statistics of the indexing algorithm.
			///</summary>
			struct index_info
			{
				enum class stop_condition
				{
					iteration = 0,
					delta,
					recall
				};

				stop_condition condition;
				uint32_t iterations;
				float recall;
				float delta;
			};

			uint32_t base_num;
			uint32_t dimension;
			const float* norm_array;
			index_params params;
			index_info info;
			nhoods_type nhoods;
			std::vector<std::vector<neighbor>> kgraph;
			const std::vector<const float*>* base_data;

			kgraph_internal() : base_data{}, base_num{}, dimension{}, norm_array{}
			{
			}

			~kgraph_internal()
			{
			}

			void init();
			void join();
			void update();
			int build();
			void linear_search(uint32_t query_id, uint32_t k, std::vector<neighbor>* pnns);
			void generate_control(uint32_t k, uint32_t num_controls, std::vector<neighbor_control>* pcontrols);
		};
	}
}

#endif // _KGRAPH_HPP_