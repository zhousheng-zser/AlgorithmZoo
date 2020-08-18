#pragma once

#include "Primitives/pure_c_handle_utils.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>

#ifdef _MSC_VER
#define IRISVIEL_C_EXPORT __declspec(dllexport)
#else
#define IRISVIEL_C_EXPORT
#endif

	DEFINE_PURE_C_HANDLE(irisviel_face_service);
	DEFINE_PURE_C_HANDLE(irisviel_database_record);

	typedef struct
	{
		irisviel_database_record_handle record;
		float similarity;
	} irisivel_database_search_result;

	typedef struct
	{
		char* key;
		float* feature;
		size_t key_size;
		size_t feature_size;
	} irsiviel_database_record_content;

	typedef enum
	{
		irisviel_feature_model_small,
		irisviel_feature_model_large
	} irisviel_feature_model;

	IRISVIEL_C_EXPORT irisviel_face_service_handle irisviel_create_instance(int single_database_capacity, int dimension, const char* working_directory);
	IRISVIEL_C_EXPORT irisviel_database_record_handle irisviel_create_record(irisviel_feature_model model);
	IRISVIEL_C_EXPORT irisviel_database_record_handle irisviel_create_record_with_arguments(irisviel_feature_model model, const char* key, const float* feature);
	IRISVIEL_C_EXPORT void irisviel_free(void* memory);
	IRISVIEL_C_EXPORT void irisviel_free_instance(irisviel_face_service_handle instance);
	IRISVIEL_C_EXPORT void irisviel_free_record(irisviel_database_record_handle record);
	IRISVIEL_C_EXPORT void irisviel_free_record_content(irsiviel_database_record_content* content);
	IRISVIEL_C_EXPORT void irisviel_free_search_result(irisivel_database_search_result* result, size_t size);
	IRISVIEL_C_EXPORT void irisviel_set_record_content(irisviel_database_record_handle record, const irsiviel_database_record_content* content);
	IRISVIEL_C_EXPORT void irisviel_get_record_content(irisviel_database_record_handle record, irsiviel_database_record_content* content);
	IRISVIEL_C_EXPORT void irisviel_clear(irisviel_face_service_handle instance);
	IRISVIEL_C_EXPORT void irisviel_remove_all(irisviel_face_service_handle instance);
	IRISVIEL_C_EXPORT char* irisviel_database_directory(irisviel_face_service_handle instance);
	IRISVIEL_C_EXPORT char* irisviel_cache_directory(irisviel_face_service_handle instance);
	IRISVIEL_C_EXPORT void irisviel_load_databases(irisviel_face_service_handle instance);
	IRISVIEL_C_EXPORT size_t irisviel_search(irisviel_face_service_handle instance, const float* feature, int top, irisivel_database_search_result** result);
	IRISVIEL_C_EXPORT void irisviel_add_record(irisviel_face_service_handle instance, irisviel_database_record_handle record);
	IRISVIEL_C_EXPORT void irisviel_add_records(irisviel_face_service_handle instance, irisviel_database_record_handle* records, size_t size);
	IRISVIEL_C_EXPORT void irisviel_remove_record(irisviel_face_service_handle instance, const char* key);
	IRISVIEL_C_EXPORT void irisviel_remove_records(irisviel_face_service_handle instance, const char** keys, size_t size);
	IRISVIEL_C_EXPORT void irisviel_update_record(irisviel_face_service_handle instance, irisviel_database_record_handle record);
	IRISVIEL_C_EXPORT void irisviel_update_records(irisviel_face_service_handle instance, irisviel_database_record_handle* records, size_t size);

#ifdef __cplusplus
}
#endif
