#include "kgraph_internal.hpp"
#include "Primitives/logger.hpp"
#include "distance.hpp"

#include <bitset>
#include <random>
#include <fstream>
#include <iostream>

#ifdef _OPENMP
#include <omp.h>
#endif

#include <boost/dynamic_bitset.hpp>

#if defined(PROFILER) && defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#endif 

namespace glasssix
{
	namespace irisviel
	{
		// both pool and knn should be sorted in ascending order
		static float evaluate_recall(std::vector<neighbor> const& approximate_results, std::vector<neighbor> const& accurate_results)
		{
			if (accurate_results.empty()) return 1.0;
			uint32_t found = 0;
			uint32_t n_approximate = 0;
			uint32_t n_accurate = 0;
			while (n_approximate < approximate_results.size() && n_accurate < accurate_results.size())
			{
				if (abs(accurate_results[n_accurate].distance - approximate_results[n_approximate].distance) < 1e-5)
				{
					++found;
					++n_accurate;
					++n_approximate;
				}
				else if (accurate_results[n_accurate].distance < approximate_results[n_approximate].distance)
				{
					++n_accurate;
				}
				else
				{
					std::cerr << "Distance is unstable." << std::endl;
					std::cerr << "Exact";
					for (auto const& p : accurate_results)
					{
						std::cerr << ' ' << p.id << ':' << p.distance;
					}
					std::cerr << std::endl;
					std::cerr << "Approx";
					for (auto const& p : approximate_results)
					{
						std::cerr << ' ' << p.id << ':' << p.distance;
					}
					std::cerr << std::endl;
					throw nsg_calculate_error("distance is unstable");
				}
			}
			return float(found) / accurate_results.size();
		}

		static float evaluate_delta(const std::vector<neighbor>& pool, uint32_t k)
		{
			uint32_t count = 0;
			uint32_t num = k;
			if (pool.size() < num) num = pool.size();
			for (uint32_t i = 0; i < num; ++i)
			{
				if (pool[i].flag) ++count;
			}
			return float(count) / k;
		}

		void kgraph_internal::linear_search(uint32_t query_id, uint32_t k, std::vector<neighbor>* pnns)
		{
			std::vector<neighbor> neighbors(k + 1);
			neighbor nn;
			nn.id = 0;
			nn.flag = true; // we don't really use this
			float normQuery = norm_array[query_id];
			uint32_t tempK = 0;
			while (nn.id < base_num)
			{
				if (nn.id != query_id)
				{
					float normNN = norm_array[nn.id];
#ifdef COSINE_DISTANCE
					nn.distance = distance_cosine::compare((*base_data).at(nn.id), normNN, (*base_data).at(query_id), normQuery, dimension);
#else
					nn.distance = distance_fast_l2::compare((*base_data).at(nn.id), norm_nn, (*base_data).at(query_id), normQuery, dimension);
#endif // COSINE_DISTANCE

					insert_into_pool(&neighbors[0], tempK, nn);
					if (tempK < k) ++tempK;
				}
				++nn.id;
			}
			neighbors.resize(k);
			pnns->swap(neighbors);
		}

		void kgraph_internal::generate_control(uint32_t k, uint32_t num_controls, std::vector<neighbor_control>* pcontrols)
		{
			std::vector<neighbor_control> controls(num_controls);
			{
				std::vector<uint32_t> index(base_num);
				int i = 0;
				for (uint32_t& v : index)
				{
					v = i++;
				}
				std::mt19937 random_device{ std::random_device{}() };
				shuffle(index.begin(), index.end(), random_device);

#ifdef _OPENMP
#pragma omp parallel for
#endif
				for (int i = 0; i < num_controls; ++i)
				{
					controls[i].id = index[i];
					linear_search(index[i], k, &controls[i].pool);
				}
			}
			pcontrols->swap(controls);
		}

		// generate size distinct random numbers (< numItem) to fill in addr
		template <typename RNG>
		static void gen_random(RNG& rng, uint32_t* addr, uint32_t size, uint32_t num_item)
		{
			if (num_item == size)
			{
				for (uint32_t i = 0; i < size; ++i)
				{
					addr[i] = i;
				}
				return;
			}
			for (uint32_t i = 0; i < size; ++i)
			{
				addr[i] = rng() % (num_item - size);
			}
			std::sort(addr, addr + size);
			for (uint32_t i = 1; i < size; ++i)
			{
				if (addr[i] <= addr[i - 1])
				{
					addr[i] = addr[i - 1] + 1;
				}
			}
			uint32_t off = rng() % num_item;
			for (uint32_t i = 0; i < size; ++i)
			{
				addr[i] = (addr[i] + off) % num_item;
			}
		}

		// The neighborhood structure maintains a pool of near neighbors of an object.
		// The neighbors are stored in the pool.  "n" (<="params.poolSize") is the number of valid entries
		// in the pool, with the beginning "k" (<="n") entries sorted.

		void kgraph_internal::init()
		{

#ifdef COSINE_DISTANCE

			if (dimension <= 128)
			{
				if (base_num <= 50000)
				{
					params.k = 50;
					params.pool_size = 100;
					params.reverse_pool_size = 100;
				}
				else if (base_num <= 100000)
				{
					params.k = 60;
					params.pool_size = 110;
					params.reverse_pool_size = 110;
				}
				else if (base_num <= 200000)
				{
					params.k = 80;
					params.pool_size = 130;
					params.reverse_pool_size = 130;
				}
				else if (base_num <= 500000)
				{
					params.k = 80;
					params.pool_size = 130;
					params.reverse_pool_size = 130;
				}
				else
				{
					params.k = 80;
					params.pool_size = 130;
					params.reverse_pool_size = 130;
				}
			}
			else
			{
				if (base_num <= 50000)
				{
					params.k = 70;
					params.pool_size = 120;
					params.reverse_pool_size = 120;
				}
				else if (base_num <= 100000)
				{
					params.k = 80;
					params.pool_size = 130;
					params.reverse_pool_size = 130;
				}
				else if (base_num <= 200000)
				{
					params.k = 70;
					params.pool_size = 120;
					params.reverse_pool_size = 120;
				}
				else if (base_num <= 500000)
				{
					params.k = 100;
					params.pool_size = 150;
					params.reverse_pool_size = 150;
				}
				else
				{
					params.k = 100;
					params.pool_size = 150;
					params.reverse_pool_size = 150;
				}
			}

#else

			if (dimension <= 128)
			{
				if (base_num <= 50000)
				{
					params.k = 50;
					params.pool_size = 100;
					params.reverse_pool_size = 100;
				}
				else if (base_num <= 100000)
				{
					params.k = 60;
					params.pool_size = 110;
					params.reverse_pool_size = 110;
				}
				else if (base_num <= 200000)
				{
					params.k = 80;
					params.pool_size = 130;
					params.reverse_pool_size = 130;
				}
				else if (base_num <= 500000)
				{
					params.k = 80;
					params.pool_size = 130;
					params.reverse_pool_size = 130;
				}
				else
				{
					params.k = 80;
					params.pool_size = 130;
					params.reverse_pool_size = 130;
				}
			}
			else
			{
				if (base_num <= 50000)
				{
					params.k = 70;
					params.pool_size = 120;
					params.reverse_pool_size = 120;
				}
				else if (base_num <= 100000)
				{
					params.k = 80;
					params.pool_size = 130;
					params.reverse_pool_size = 130;
				}
				else if (base_num <= 200000)
				{
					params.k = 70;
					params.pool_size = 120;
					params.reverse_pool_size = 120;
				}
				else if (base_num <= 500000)
				{
					params.k = 100;
					params.pool_size = 150;
					params.reverse_pool_size = 150;
				}
				else
				{
					params.k = 100;
					params.pool_size = 150;
					params.reverse_pool_size = 150;
				}
			}

#endif // COSINE_DISTANCE

			if (base_num <= params.k)
			{
				//LOG(WARNING) << "Warning: small dataset, shrinking params.K to " << base_num - 1 << ".";
				params.k = base_num - 1;
			}
			if (base_num <= params.pool_size)
			{
				//LOG(WARNING) << "Warning: small dataset, shrinking poolSize to " << base_num - 1 << ".";
				params.pool_size = base_num - 1;
			}
			if (base_num <= params.reverse_pool_size)
			{
				//LOG(WARNING) << "Warning: small dataset, shrinking reversePoolSize to " << base_num - 1 << ".";
				params.reverse_pool_size = base_num - 1;
			}

			uint32_t N = base_num;
			nhoods.resize(base_num);
			uint32_t seed = 1998;
			std::mt19937 rng{ seed };

#ifdef _OPENMP
#pragma omp parallel for
#endif
			for (int n = 0; n < N; n++)
			{
				auto& nhood = nhoods[n];
				nhood.reset(new neighborhood());
				(nhood->nn_new).resize(params.k * 2);
				(nhood->pool).resize(params.pool_size + 1);
				nhood->radius = std::numeric_limits<float>::max();
			}

#ifdef _OPENMP
#pragma omp parallel
#endif
			{
#ifdef _OPENMP
				std::mt19937 rng{ seed ^ omp_get_thread_num() };
#else
				std::mt19937 rng(seed);
#endif

				std::vector<uint32_t> random(params.k + 1);
#ifdef _OPENMP
#pragma omp for
#endif
				for (int n = 0; n < N; ++n)
				{
					auto& nhood = nhoods[n];
					std::vector<neighbor>& pool = nhood->pool;
					gen_random(rng, &nhood->nn_new[0], nhood->nn_new.size(), N);
					gen_random(rng, &random[0], random.size(), N);
					nhood->neighbors_length = params.k;
					nhood->M = params.k;
					uint32_t i = 0;
					for (uint32_t j = 0; j < nhood->neighbors_length; ++j)
					{
						if (random[i] == n) ++i;
						auto& nn = nhood->pool[j];
						nn.id = random[i++];
						float norm_n = norm_array[n];
						float norm_nn = norm_array[nn.id];
#ifdef COSINE_DISTANCE
						nn.distance = distance_cosine::compare((*base_data).at(nn.id), norm_nn, (*base_data).at(n), norm_n, dimension);
#else
						nn.distance = distance_fast_l2::compare((*base_data).at(nn.id), norm_nn, (*base_data).at(n), norm_n, dimension);
#endif // COSINE_DISTANCE

						nn.flag = true;
					}
					sort(pool.begin(), pool.begin() + nhood->neighbors_length);
				}
			}
		}

		void kgraph_internal::join()
		{
#ifdef _OPENMP
#pragma omp parallel for default(shared) schedule(dynamic, 100)
#endif
			for (int n = 0; n < base_num; ++n)
			{
				size_t signal = 0;
				nhoods[n]->found = false;
				nhoods[n]->join([&](uint32_t i, uint32_t j)
				{
					float norm_i = norm_array[i];
					float norm_j = norm_array[j];
#ifdef COSINE_DISTANCE
					float dist = distance_cosine::compare((*base_data).at(i), norm_i, (*base_data).at(j), norm_j, dimension);
#else
					float dist = distance_fast_l2::compare((*base_data).at(i), norm_i, (*base_data).at(j), norm_j, dimension);
#endif // COSINE_DISTANCE	
					uint32_t insertPos;
					insertPos = nhoods[i]->try_insert_parallel(j, dist);
					if (insertPos < params.k) ++signal;
					insertPos = nhoods[j]->try_insert_parallel(i, dist);
					if (insertPos < params.k) ++signal;
				});
				nhoods[n]->found = signal > 0;
			}
		}

		void kgraph_internal::update()
		{
			std::random_device rd;
			std::mt19937 rng(rd());
			uint32_t N = base_num;

#ifdef _OPENMP
#pragma omp parallel for
#endif
			for (int n = 0; n < N; ++n)
			{
				auto& nhood = nhoods[n];
				nhood->nn_new.clear();
				nhood->nn_old.clear();
				nhood->rnn_new.clear();
				nhood->rnn_old.clear();
				nhood->radius = nhoods[n]->pool.back().distance;

				if (nhood->found)
				{
					uint32_t maxPos = std::min(nhood->M + params.k, nhood->neighbors_length);
					uint32_t count = 0;
					uint32_t pos = 0;
					while ((pos < maxPos) && (count < params.k))
					{
						if (nhood->pool[pos].flag) ++count;
						++pos;
					}
					//nhood->M is position represented by nhood.old+params.K
					nhood->M = pos;
				}
				BOOST_VERIFY(nhood->M > 0);
				nhood->radius_m = nhood->pool[nhood->M - 1].distance;
			}
#ifdef _OPENMP
#pragma omp parallel for
#endif
			for (int n = 0; n < N; ++n)
			{
				auto& nhood = nhoods[n];
				auto& nnNew = nhood->nn_new;
				auto& nnOld = nhood->nn_old;
				for (uint32_t i = 0; i < nhood->M; ++i)
				{
					auto& nn = nhood->pool[i];
					auto& nhoodO = nhoods[nn.id];  // nhood on the other side of the edge
					if (nn.flag)
					{
						nnNew.push_back(nn.id);
						//nn belongs to nhoods[n]->pool, but nn is not a knn point of n(we can judge from a large dist),
						//so nn is a reverse-KNN point of n, that is to say n is a knn point of nn, so we add n to nhoodO.rnn
						if (nn.distance > nhoodO->radius_m)
						{
							lock_guard_type guard(nhoodO->lock);
							nhoodO->rnn_new.push_back(n);
						}
						nn.flag = false;
					}
					else
					{
						nnOld.push_back(nn.id);
						if (nn.distance > nhoodO->radius_m)
						{
							lock_guard_type guard(nhoodO->lock);
							nhoodO->rnn_old.push_back(n);
						}
					}
				}
			}

#ifdef _OPENMP
#pragma omp parallel for
#endif
			for (int n = 0; n < N; ++n)
			{
				auto& nhood = nhoods[n];
				auto& nn_new = nhood->nn_new;
				auto& nn_old = nhood->nn_old;
				auto& rnn_new = nhood->rnn_new;
				auto& rnn_old = nhood->rnn_old;

				if (rnn_new.size() > params.reverse_pool_size)
				{
					std::shuffle(rnn_new.begin(), rnn_new.end(), rng);
					rnn_new.resize(params.reverse_pool_size);
				}
				nn_new.insert(nn_new.end(), rnn_new.begin(), rnn_new.end());
				if (rnn_old.size() > params.reverse_pool_size)
				{
					std::shuffle(rnn_old.begin(), rnn_old.end(), rng);
					rnn_old.resize(params.reverse_pool_size);
				}
				nn_old.insert(nn_old.end(), rnn_old.begin(), rnn_old.end());
			}
		}

		int kgraph_internal::build()
		{
			init();
			int max_memory_usage = 0;
			uint32_t n = base_num;
			std::vector<neighbor_control> controls;

			int control_num = 100 < n ? 100 : n;
			generate_control(params.k, control_num, &controls);
			info.condition = index_info::stop_condition::iteration;
			info.recall = 0;
			info.iterations = 0;
			info.delta = 1.0;

#ifdef PROFILER
#ifdef _MSC_VER
			HANDLE handle = GetCurrentProcess();
			PROCESS_MEMORY_COUNTERS_EX pmc = { 0 };
			if (!GetProcessMemoryInfo(handle, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
			{
				DWORD errCode = GetLastError();
				return max_memory_usage;
			}
#endif // _MSC_VER
#endif // PROFILER

			for (uint32_t it = 0; it < params.iterations; ++it)
			{
				++info.iterations;

				join();
				{
					float recall2 = 0, delta2 = 0;
					for (const auto& nhood : nhoods)
					{
						delta2 += evaluate_delta(nhood->pool, params.k);
					}
					for (const auto& c : controls)
					{
						recall2 += evaluate_recall(nhoods[c.id]->pool, c.pool);
					}
					info.delta = delta2 / nhoods.size();
					info.recall = recall2 / controls.size();


					if (info.delta <= params.delta)
					{
						info.condition = index_info::stop_condition::delta;
						//LOG(INFO) << "recall: " << info.recall << " delta: " << info.delta;
						break;
					}
					if (info.recall >= params.recall)
					{
						info.condition = index_info::stop_condition::recall;
						//LOG(INFO) << "recall: " << info.recall << " delta: " << info.delta;
						break;
					}
					update();
				}
			}

			kgraph.resize(n);

			for (uint32_t i = 0; i < n; ++i)
			{
				auto& knn = kgraph[i];
				auto const& pool = nhoods[i]->pool;
				uint32_t size = params.pool_size;
				knn.resize(size);
				for (uint32_t k = 0; k < size; ++k)
				{
					knn[k].id = pool[k].id;
					knn[k].distance = pool[k].distance;
				}
			}
#if defined(PROFILER) && defined(_MSC_VER)
			max_memory_usage = pmc.WorkingSetSize / 1048576;
#endif 
			nhoods.clear();
			return max_memory_usage;
		}
	}
}