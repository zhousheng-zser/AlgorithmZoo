#pragma once

#include "Primitives/pure_c_handle_utils.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#ifdef _MSC_VER
#define LONGINUS_C_EXPORT __declspec(dllexport)
#else
#define LONGINUS_C_EXPORT
#endif

	DEFINE_PURE_C_HANDLE(longinus);

	LONGINUS_C_EXPORT longinus_handle Longinus_NewInstance(int device);
	LONGINUS_C_EXPORT void Longinus_ReleaseInstance(longinus_handle instance);
	LONGINUS_C_EXPORT char* Longinus_getVersion();
	LONGINUS_C_EXPORT unsigned char* Longinus_get(longinus_handle instance, const uint8_t* input_data, int num, int height, int width);

#ifdef __cplusplus
}
#endif
