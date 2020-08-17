#include "search.hpp"
#include "Primitives/tensor.hpp"

#include <cstring>
#include <fstream>
#include <iostream>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace glasssix
{
	namespace irisviel
	{
		irisviel_search_internal::irisviel_search_internal(const std::vector<const float*>& base_data, int dimension) : base_data{ &base_data }, base_num_{ static_cast<uint32_t>(base_data.size()) }, dimension_{ static_cast<uint32_t>(dimension) }
		{
		}

		irisviel_search_internal::irisviel_search_internal(int dimension) : base_data{}, base_num_{}, dimension_{ static_cast<uint32_t>(dimension) }
		{
		}

		irisviel_search_internal::~irisviel_search_internal()
		{
		}

		bool irisviel_search_internal::load_graph(const char* graphPath)
		{
			//load graph
			ngraph.clear();
			std::ifstream in_graph{ graphPath, std::ios::binary };

			if (!in_graph.is_open())
			{
				throw nsg_calculate_error("open ngraph file error");
			}

			if (!in_graph)
			{
				throw nsg_calculate_error("empty ngraph file");
			}

			constexpr size_t header_size = sizeof(bool) + sizeof(uint32_t) + sizeof(uint32_t);

			// Get the file size.
			in_graph.seekg(0, std::ios::end);
			std::streamoff file_size = in_graph.tellg();
			in_graph.seekg(0, std::ios::beg);

			// Check the file size alignment.
			if (file_size < header_size || file_size % sizeof(uint32_t) != 1 || (file_size - header_size) % sizeof(uint32_t) != 0)
			{
				throw nsg_calculate_error{ "Invalid file size alignment." };
			}

			in_graph.read(reinterpret_cast<char*>(&normalized), sizeof(normalized));
			in_graph.read(reinterpret_cast<char*>(&width), sizeof(width));
			in_graph.read(reinterpret_cast<char*>(&navigate_node), sizeof(navigate_node));

			while (in_graph)
			{
				uint32_t k = 0;
				in_graph.read(reinterpret_cast<char*>(&k), sizeof(k));

				if (!in_graph)
				{
					break;
				}

				std::vector<uint32_t> tmp;

				// Tranverse the allocation exception.
				try
				{
					tmp.resize(k);
				}
				catch (std::bad_alloc&)
				{
					throw nsg_calculate_error{ "Failed to allocate the data segment." };
				}

				in_graph.read((char*)tmp.data(), k * sizeof(uint32_t));

				// Check the size.
				if (in_graph.gcount() < k * sizeof(uint32_t))
				{
					throw nsg_calculate_error{ "Invalid data segment size." };
				}

				ngraph.emplace_back(std::move(tmp));
			}

			return true;
		}

		bool irisviel_search_internal::load_graph(const char* graph_path, const char* base_data_path)
		{
			load_graph(graph_path);

			base_data_cache_.clear();
			std::ifstream in_base_data{ base_data_path, std::ios::binary };
			if (!in_base_data.is_open())
			{
				throw nsg_calculate_error{ "open basedata file error" };
			}

			if (in_base_data.eof())
			{
				throw nsg_calculate_error{ "empty basedata file" };
			}

			while (!in_base_data.eof())
			{
				float* temp_data = (float*)malloc(dimension_ * sizeof(float));
				in_base_data.read((char*)(temp_data), dimension_ * sizeof(float));
				base_data_cache_.push_back(const_cast<const float*>(temp_data));
			}

			base_data = &base_data_cache_;
			base_num_ = base_data_cache_.size() - 1;
			if ((base_data_cache_.size() - 1) <= 0)
			{
				throw nsg_calculate_error{ "empty basedata file" };
			}

			return true;
		}

		const std::vector<const float*>* irisviel_search_internal::get_base_data()
		{
			return base_data;
		}

		void irisviel_search_internal::optimize_graph()
		{
			//use after build or load
			if (base_num_ <= 50000) {
				neighbors_max_length = 200;
			}
			else if (base_num_ <= 100000) {
				neighbors_max_length = 200;
			}
			else if (base_num_ <= 200000) {
				neighbors_max_length = 240;
			}
			else if (base_num_ <= 500000) {
				neighbors_max_length = 280;
			}
			else {
				neighbors_max_length = 300;
			}

			if (base_num_ <= neighbors_max_length)
			{
				std::cerr << "Warning: small dataset, shrinking neighborsMaxLength to " << base_num_ << "." << std::endl;
				neighbors_max_length = base_num_;
			}

			data_len_ = (dimension_ + 1) * sizeof(float);
			neighbor_len_ = (width + 1) * sizeof(uint32_t);
			node_size_ = data_len_ + neighbor_len_;
			opt_graph_tensor_.reset(new memory::tensor<char>(node_size_ * base_num_));
			opt_graph_ = opt_graph_tensor_->mutable_cpu_data();
			if (opt_graph_ == nullptr)
			{
				throw nsg_calculate_error("optGraph_ nullptr");
			}

			for (uint32_t i = 0; i < base_num_; i++) {
				char* curNodeOffset = opt_graph_ + i * node_size_;

				float cur_norm = 0;
				if (normalized)
				{
					cur_norm = 1;
				}
				else
				{
#ifdef COSINE_DISTANCE
					cur_norm = distance_cosine::norm((*base_data)[i], dimension_);
#else
					cur_norm = distance_fast_l2::norm((*base_data)[i], dimension);
#endif // COSINE_DISTANCE
				}

				memcpy(curNodeOffset, &cur_norm, sizeof(float));
				memcpy(curNodeOffset + sizeof(float), (*base_data)[i], data_len_ - sizeof(float));

				curNodeOffset += data_len_;
				uint32_t k = ngraph[i].size();
				if (k > neighbor_len_)
				{
					throw nsg_calculate_error("ngraph has a huge k");
				}

				memcpy(curNodeOffset, &k, sizeof(uint32_t));
				memcpy(curNodeOffset + sizeof(uint32_t), ngraph[i].data(), k * sizeof(uint32_t));
				std::vector<uint32_t>().swap(ngraph[i]);
			}

			compact_graph_type{}.swap(ngraph);
		}

		std::tuple<vector2d<uint32_t>, vector2d<float>> irisviel_search_internal::search_vector(const std::vector<const float*>& query_data, uint32_t top_k)
		{
			vector2d<uint32_t> ids;
			vector2d<float> similarities;

			query_data_ = &query_data;
			query_num_ = query_data.size();

			ids.resize(query_num_);
			similarities.resize(query_num_);

			if (base_num_ != 1)
			{
#ifdef _OPENMP
#pragma omp parallel for
#endif
				for (int i = 0; i < query_num_; ++i)
				{
					search_with_opt_graph((*query_data_)[i], top_k, ids[i], similarities[i]);
				}
			}
			else
			{
				for (int i = 0; i < query_num_; ++i)
				{
					ids[i].resize(1);
					similarities[i].resize(1);

					//return 0 when there is only one picture in database
					ids[i][0] = 0;

#ifdef COSINE_DISTANCE
					float norm_base = distance_cosine::norm((*base_data)[0], dimension_);
					float norm_query = distance_cosine::norm((*query_data_)[i], dimension_);
					float dist = distance_cosine::compare((*base_data)[0], norm_base, (*query_data_)[i], norm_query, dimension_);

					similarities[i][0] = 1.0f - dist;
#else
					float norm_query = distance_fast_l2::norm((*query_data_)[i], dimension);
					float dist = distance_l2::compare((*base_data)[0], (*queryData_)[i], dimension);
					return_similarities[i][0] = 1.0f - 1.0f * dist / norm_query;
#endif // COSINE_DISTANCE
				}
			}

			return std::tuple<vector2d<uint32_t>, vector2d<float>>{ ids, similarities };
		}

		void irisviel_search_internal::search_with_opt_graph(const float* single_query_data, uint32_t top_k, std::vector<uint32_t>& return_ids, std::vector<float>& return_similarities)
		{
			if (top_k > neighbors_max_length || top_k > base_num_)
			{
				std::cerr << "error, topK is bigger than " << neighbors_max_length << ", or bigger than " << base_num_;
				return;
			}

			std::vector <neighbor> return_neighbors;
			return_neighbors.resize(neighbors_max_length + 1);
			return_ids.resize(top_k);
			return_similarities.resize(top_k);
			std::vector<uint32_t> init_ids(neighbors_max_length);

			boost::dynamic_bitset<> flags{ base_num_, 0 };
			uint32_t count = 0;
			uint32_t* neighbors = (uint32_t*)(opt_graph_ + node_size_ * navigate_node + data_len_);
			uint32_t max_m_ep = *neighbors;
			neighbors++;

			for (; count < neighbors_max_length && count < max_m_ep; count++)
			{
				init_ids[count] = neighbors[count];
				flags[init_ids[count]] = true;
			}

			while (count < neighbors_max_length)
			{
				uint32_t id = rand() % base_num_;
				if (flags[id])continue;
				flags[id] = true;
				init_ids[count] = id;
				count++;
			}

#if SIMD_TYPE >= SIMDTYPE_SSE
			for (uint32_t i = 0; i < init_ids.size(); i++)
			{
				uint32_t id = init_ids[i];
				if (id >= base_num_)continue;

#ifdef _WIN32
				_mm_prefetch(opt_graph_ + node_size_ * id, _MM_HINT_T0);
#endif
			}
#endif

			float norm_query = 0.0f;
			if (normalized)
			{
				norm_query = 1.0f;
			}
			else
			{
#ifdef COSINE_DISTANCE
				norm_query = distance_cosine::norm(single_query_data, dimension_);
#else
				norm_query = distance_fast_l2::norm(single_query_data, dimension);
#endif // COSINE_DISTANCE
			}

			for (uint32_t i = 0; i < init_ids.size(); i++)
			{
				uint32_t id = init_ids[i];
				float* x = (float*)(opt_graph_ + node_size_ * id);
				float norm_x = *x; x++;

#ifdef COSINE_DISTANCE
				float dist = distance_cosine::compare(x, norm_x, single_query_data, norm_query, dimension_);
#else
				float dist = distance_fast_l2::compare(x, norm_x, single_query_data, norm_query, dimension);
#endif // COSINE_DISTANCE

				return_neighbors[i] = neighbor(id, dist, true);
				flags[id] = true;

			}

			std::sort(return_neighbors.begin(), return_neighbors.begin() + neighbors_max_length);
			int i = 0;
			while (i < (int)neighbors_max_length)
			{
				int min_pos = neighbors_max_length;
				if (return_neighbors[i].flag)
				{
					return_neighbors[i].flag = false;
					uint32_t n = return_neighbors[i].id;

#if SIMD_TYPE >= SIMDTYPE_SSE && defined(_WIN32)
					_mm_prefetch(opt_graph_ + node_size_ * n + data_len_, _MM_HINT_T0);
#endif

					uint32_t* neighbors = (uint32_t*)(opt_graph_ + node_size_ * n + data_len_);
					uint32_t MaxM = *neighbors;
					neighbors++;

#if SIMD_TYPE >= SIMDTYPE_SSE && defined(_WIN32)
					for (uint32_t m = 0; m < MaxM; ++m)
						_mm_prefetch(opt_graph_ + node_size_ * neighbors[m], _MM_HINT_T0);
#endif

					for (uint32_t m = 0; m < MaxM; ++m)
					{
						uint32_t id = neighbors[m];
						if (flags[id])continue;
						flags[id] = 1;
						float* data = (float*)(opt_graph_ + node_size_ * id);
						float norm_id = *data; data++;

#ifdef COSINE_DISTANCE
						float dist = distance_cosine::compare(data, norm_id, single_query_data, norm_query, dimension_);
#else
						float dist = distance_fast_l2::compare(data, norm_id, single_query_data, norm_query, dimension);
#endif // COSINE_DISTANCE

						if (dist >= return_neighbors[neighbors_max_length - 1].distance)continue;
						neighbor nn(id, dist, true);

						int insert_pos = insert_into_pool(&return_neighbors[0], neighbors_max_length, nn);
						if (return_neighbors.size() > neighbors_max_length + 1)
						{
							return_neighbors.pop_back();
						}

						if (insert_pos < min_pos)
						{
							min_pos = insert_pos;
						}
					}
				}

				if (min_pos <= i)
				{
					i = min_pos;
				}
				else
				{
					++i;
				}
			}

			for (size_t i = 0; i < top_k; i++)
			{
				return_ids[i] = return_neighbors[i].id;

#ifdef COSINE_DISTANCE
				return_similarities[i] = 1.0f - return_neighbors[i].distance;

#else
				return_similarities[i] = 1.0f - 1.0f * distance_l2::compare(single_query_data, (*base_data).at(return_ids[i]),
					(uint32_t)dimension) / norm_query;
#endif // COSINE_DISTANCE

			}
		}

		void irisviel_search_internal::save_result(const char* path, const std::vector<std::vector<uint32_t> >& return_ids)
		{
			std::ofstream out(path, std::ios::trunc | std::ios::binary);

			for (uint32_t i = 0; i < return_ids.size(); i++) {
				uint32_t GK = (uint32_t)return_ids[i].size();
				out.write((char*)&GK, sizeof(uint32_t));
				out.write((char*)return_ids[i].data(), GK * sizeof(uint32_t));
				if (i % 100 == 0)
				{
					out.flush();
				}
			}
			out.close();
		}

	}
}
