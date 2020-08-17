#pragma once

#ifndef _NEIGHBOR_HPP_
#define _NEIGHBOR_HPP_

#include <mutex>
#include <vector>
#include <memory>

#include <boost/smart_ptr/detail/spinlock.hpp>

namespace glasssix
{
	namespace irisviel
	{
		using spin_lock_type = boost::detail::spinlock;
		using lock_guard_type = std::lock_guard<spin_lock_type>;

		struct neighbor
		{
			uint32_t id;
			float distance;
			bool flag;

			neighbor() : id{}, distance{}, flag{}
			{
			}

			neighbor(uint32_t id, float distance, bool flag = true) : id{ id }, distance{ distance }, flag{ flag }
			{
			}

			bool operator<(const neighbor& other) const noexcept
			{
				return distance < other.distance;
			}

			bool operator==(const neighbor& other) const noexcept
			{
				return id == other.id;
			}
		};

		struct synchronized_neighbor
		{
			spin_lock_type lock;
			std::vector<neighbor> pool;
		};

		using locked_graph_type = std::vector<synchronized_neighbor>;

		struct neighbor_control
		{
			uint32_t id;
			std::vector<neighbor> pool;
		};

		static int insert_into_pool(neighbor* addr, uint32_t top_k, neighbor nn)
		{
			if (top_k == 0)
			{
				addr[0] = nn;
				return 0;
			}

			int start = 0;
			int end = top_k - 1;
			int mid;

			// binary search
			while (start <= end)
			{
				mid = (start + end) / 2;
				if (nn.distance < addr[mid].distance)
				{
					end = mid - 1;
				}
				else
				{
					start = mid + 1;
				}
			}

			// check for equal ID
			int pos = start;
			while (pos-- > 0)
			{
				if (addr[pos].distance < nn.distance)
				{
					break;
				}

				if (addr[pos].id == nn.id)
				{
					return top_k + 1;
				}
			}

			for (int j = top_k; j > start; j--)
			{
				addr[j] = addr[j - 1];
			}

			addr[start] = nn;
			return start;
		}

		struct neighborhood
		{
			spin_lock_type lock;
			std::vector<neighbor> pool;
			uint32_t neighbors_length;     // # valid items in the pool,  L + 1 <= pool.size()
			uint32_t M;     // we only join items in pool[0..M)
			float radius;   // distance of interesting range
			float radius_m;
			bool found;     // helped found new NN in this round
			std::vector<uint32_t> nn_old;
			std::vector<uint32_t> nn_new;
			std::vector<uint32_t> rnn_old;
			std::vector<uint32_t> rnn_new;

			// only non-readonly method which is supposed to be called in parallel
			uint32_t try_insert_parallel(uint32_t id, float distance)
			{
				if (distance > radius)
				{
					return pool.size();
				}

				lock_guard_type guard(lock);
				uint32_t insertPos = insert_into_pool(&pool[0], neighbors_length, neighbor{ id, distance, true });
				if (insertPos <= neighbors_length)
				{ // inserted
					if (neighbors_length + 1 < pool.size())
					{ // if insertPos == neighborsLength + 1, there's a duplicate
						neighbors_length++;
					}
					else
					{
						radius = pool[neighbors_length - 1].distance;
					}
				}
				return insertPos;
			}

			// join should not be conflict with insert
			template <typename Callable>
			auto join(Callable&& callback) const
			{
				for (const uint32_t i : nn_new)
				{
					for (const uint32_t j : nn_new)
					{
						if (i < j)
						{
							std::forward<Callable>(callback)(i, j);
						}
					}
					for (uint32_t j : nn_old)
					{
						std::forward<Callable>(callback)(i, j);
					}
				}
			}
		};

		using nhoods_type = std::vector<std::shared_ptr<neighborhood>>;
	}
}

#endif // !_NEIGHBOR_HPP_