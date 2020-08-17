#include "irisviel_c.h"
#include "database_record.hpp"
#include "Primitives/memory.hpp"
#include "face_service_internal.hpp"

#include <memory>
#include <vector>
#include <cstring>
#include <cstddef>
#include <algorithm>

using namespace glasssix::pure_c;
using namespace glasssix::memory;
using namespace glasssix::irisviel;

IRISVIEL_C_EXPORT irisviel_face_service_handle irisviel_create_instance(int single_database_capacity, int dimension, const char* working_directory)
{
	return to_handle<irisviel_face_service_handle>(heap_alloc_object<face_service_internal>(single_database_capacity, dimension, working_directory));
}

IRISVIEL_C_EXPORT irisviel_database_record_handle irisviel_create_record(irisviel_feature_model model)
{
	switch (model)
	{
	case irisviel_feature_model_small:
		return to_handle<irisviel_database_record_handle>(heap_alloc_object<std::shared_ptr<database_record>>(database_record::create(128)));
	case irisviel_feature_model_large:
		return to_handle<irisviel_database_record_handle>(heap_alloc_object<std::shared_ptr<database_record>>(database_record::create(512)));
	default:
		return nullptr;
	}
}

IRISVIEL_C_EXPORT irisviel_database_record_handle irisviel_create_record_with_arguments(irisviel_feature_model model, const char* key, const float* feature)
{
	auto handle = irisviel_create_record(model);

	if (handle)
	{
		auto result = from_handle<std::shared_ptr<database_record>>(handle);

		(*result)->key(key);
		(*result)->feature(glasssix::exposing::param_span<const float>{ feature, 512 });
	}

	return handle;
}

IRISVIEL_C_EXPORT void irisviel_free(void* memory)
{
	if (memory)
	{
		heap_free(memory);
	}
}

IRISVIEL_C_EXPORT void irisviel_free_instance(irisviel_face_service_handle instance)
{
	if (instance)
	{
		heap_free_object(from_handle<face_service_internal>(instance));
	}
}

IRISVIEL_C_EXPORT void irisviel_free_record(irisviel_database_record_handle record)
{
	if (record)
	{
		heap_free_object(from_handle<std::shared_ptr<database_record>>(record));
	}
}

IRISVIEL_C_EXPORT void irisviel_free_record_content(irsiviel_database_record_content* content)
{
	if (content)
	{
		if (content->key)
		{
			heap_free(content->key);
			content->key = nullptr;
		}

		if (content->feature)
		{
			heap_free(content->feature);
			content->feature = nullptr;
		}
	}
}

IRISVIEL_C_EXPORT void irisviel_free_search_result(irisivel_database_search_result* result, std::size_t size)
{
	if (result == nullptr)
	{
		return;
	}

	for (auto ptr = result; ptr < result + size; ptr++)
	{
		if (ptr)
		{
			irisviel_free_record(ptr->record);
		}
	}

	heap_free(result);
}

IRISVIEL_C_EXPORT void irisviel_set_record_content(irisviel_database_record_handle record, const irsiviel_database_record_content* content)
{
	if (record == nullptr || content == nullptr)
	{
		return;
	}

	if (auto record_ptr = *from_handle<std::shared_ptr<database_record>>(record))
	{
		record_ptr->key(content->key);
		record_ptr->feature(glasssix::exposing::param_span<const float>{ content->feature, 512 });
	}
}

IRISVIEL_C_EXPORT void irisviel_get_record_content(irisviel_database_record_handle record, irsiviel_database_record_content* content)
{
	if (record == nullptr || content == nullptr)
	{
		return;
	}

	std::memset(content, 0, sizeof(content));

	if (auto record_ptr = *from_handle<std::shared_ptr<database_record>>(record))
	{
		content->key_size = record_ptr->key().size();
		content->feature_size = record_ptr->dimension();
		content->key = heap_alloc_elements<char>(content->key_size + 1);
		content->feature = heap_alloc_elements<float>(content->feature_size);

		std::memset(content->key, 0, content->key_size + 1);
		std::copy(record_ptr->key().begin(), record_ptr->key().end(), content->key);
		std::copy(record_ptr->feature().begin(), record_ptr->feature().end(), content->feature);
	}
}

IRISVIEL_C_EXPORT void irisviel_clear(irisviel_face_service_handle instance)
{
	if (instance)
	{
		from_handle<face_service_internal>(instance)->clear();
	}
}

IRISVIEL_C_EXPORT void irisviel_remove_all(irisviel_face_service_handle instance)
{
	if (instance)
	{
		from_handle<face_service_internal>(instance)->remove_all();
	}
}

IRISVIEL_C_EXPORT char* irisviel_database_directory(irisviel_face_service_handle instance)
{
	if (instance == nullptr)
	{
		return nullptr;
	}

	auto directory = from_handle<face_service_internal>(instance)->database_directory();
	auto result = heap_alloc_elements<char>(directory.size() + 1);

	directory.copy(result, directory.size());
	result[directory.size()] = {};

	return result;
}

IRISVIEL_C_EXPORT char* irisviel_cache_directory(irisviel_face_service_handle instance)
{
	if (instance == nullptr)
	{
		return nullptr;
	}

	auto directory = from_handle<face_service_internal>(instance)->database_directory();
	auto result = heap_alloc_elements<char>(directory.size() + 1);

	directory.copy(result, directory.size());
	result[directory.size()] = {};

	return result;
}

IRISVIEL_C_EXPORT void irisviel_load_databases(irisviel_face_service_handle instance)
{
	if (instance)
	{
		from_handle<face_service_internal>(instance)->load_databases();
	}
}

IRISVIEL_C_EXPORT std::size_t irisviel_search(irisviel_face_service_handle instance, const float* feature, int top, irisivel_database_search_result** result)
{
	if (instance == nullptr || result == nullptr)
	{
		return 0;
	}

	std::size_t index = 0;
	auto search_result = from_handle<face_service_internal>(instance)->search(feature, top);

	*result = heap_alloc_elements<irisivel_database_search_result>(search_result.size());

	for (auto& item : search_result)
	{
		(*result)[index].record = to_handle<irisviel_database_record_handle>(heap_alloc_object<std::shared_ptr<database_record>>(item.data));
		(*result)[index].similarity = item.similarity;
		index++;
	}

	return search_result.size();
}

IRISVIEL_C_EXPORT void irisviel_add_record(irisviel_face_service_handle instance, irisviel_database_record_handle record)
{
	if (instance == nullptr || record == nullptr)
	{
		return;
	}

	if (auto record_ptr = *from_handle<std::shared_ptr<database_record>>(record))
	{
		from_handle<face_service_internal>(instance)->add(*record_ptr);
	}
}

IRISVIEL_C_EXPORT void irisviel_add_records(irisviel_face_service_handle instance, irisviel_database_record_handle* records, std::size_t size)
{
	if (instance == nullptr || records == nullptr || size == 0)
	{
		return;
	}

	std::vector<std::shared_ptr<database_record>> record_ptrs;

	for (auto ptr = records; ptr < records + size; ptr++)
	{
		if (ptr)
		{
			record_ptrs.emplace_back(*from_handle<std::shared_ptr<database_record>>(*ptr));
		}
	}

	from_handle<face_service_internal>(instance)->add(record_ptrs);
}

IRISVIEL_C_EXPORT void irisviel_remove_record(irisviel_face_service_handle instance, const char* key)
{
	if (instance && key)
	{
		from_handle<face_service_internal>(instance)->remove(key);
	}
}

IRISVIEL_C_EXPORT void irisviel_remove_records(irisviel_face_service_handle instance, const char** keys, size_t size)
{
	if (instance == nullptr || keys == nullptr || size == 0)
	{
		return;
	}

	std::vector<std::string> key_strs;

	for (auto ptr = keys; ptr < keys + size; ptr++)
	{
		if (ptr && *ptr)
		{
			key_strs.emplace_back(*ptr);
		}
	}

	from_handle<face_service_internal>(instance)->remove(key_strs);
}

IRISVIEL_C_EXPORT void irisviel_update_record(irisviel_face_service_handle instance, irisviel_database_record_handle record)
{
	if (instance == nullptr || record == nullptr)
	{
		return;
	}

	if (auto record_ptr = *from_handle<std::shared_ptr<database_record>>(record))
	{
		from_handle<face_service_internal>(instance)->update(*record_ptr);
	}
}

IRISVIEL_C_EXPORT void irisviel_update_records(irisviel_face_service_handle instance, irisviel_database_record_handle* records, std::size_t size)
{
	if (instance == nullptr || records == nullptr || size == 0)
	{
		return;
	}

	std::vector<std::shared_ptr<database_record>> record_ptrs;

	for (auto ptr = records; ptr < records + size; ptr++)
	{
		if (ptr)
		{
			record_ptrs.emplace_back(*from_handle<std::shared_ptr<database_record>>(*ptr));
		}
	}

	from_handle<face_service_internal>(instance)->update(record_ptrs);
}
