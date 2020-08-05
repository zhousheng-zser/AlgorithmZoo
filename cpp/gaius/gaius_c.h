#pragma once

#include "Primitives/pure_c_handle_utils.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#ifdef _MSC_VER
#define GAIUS_C_EXPORT __declspec(dllexport)
#else
#define GAIUS_C_EXPORT
#endif

	DEFINE_PURE_C_HANDLE(gaius);

	GAIUS_C_EXPORT gaius_handle Gaius_NewInstance(int device);
	GAIUS_C_EXPORT void Gaius_ReleaseInstance(gaius_handle instance);
	GAIUS_C_EXPORT char* Gaius_getVersion();
	GAIUS_C_EXPORT float* Gaius_Forward(gaius_handle instance, const uint8_t* input_data, int num, int order, bool mask);

#ifdef __cplusplus
}
#endif
