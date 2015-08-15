#pragma once

#define DLL_EXPORT(T) __declspec(dllexport) T __stdcall
#define DLL_IMPORT(T) __declspec(dllimport) T __stdcall
#ifdef STRINGS_EXPORT
#define STRINGS_API DLL_EXPORT
#else
#define STRINGS_API DLL_IMPORT
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StringsTag* HSTRINGS;

STRINGS_API(HSTRINGS) OpenStrings(const char* path);
STRINGS_API(const char*) ReadString(HSTRINGS, uint32_t);
STRINGS_API(const char*) ReadStringPl(HSTRINGS, intmax_t, uint32_t);
STRINGS_API(void) CloseStrings(HSTRINGS);

#ifdef __cplusplus
}
#endif