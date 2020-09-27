// Copyright (c) 2015 midnightBITS
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#ifdef _WIN32
#define DLL_EXPORT(T) __declspec(dllexport) T __stdcall
#define DLL_IMPORT(T) __declspec(dllimport) T __stdcall
#else
#define DLL_EXPORT(T) T
#define DLL_IMPORT(T) T
#endif

#ifdef STRINGS_EXPORT
#define STRINGS_API DLL_EXPORT
#else
#define STRINGS_API DLL_IMPORT
#endif

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StringsTag {
	int _Placeholder;
} * HSTRINGS;

STRINGS_API(HSTRINGS) OpenStrings(const char* path);
STRINGS_API(const char*) ReadString(HSTRINGS, uint32_t);
STRINGS_API(const char*) ReadStringPl(HSTRINGS, uint32_t, intmax_t);
STRINGS_API(void) CloseStrings(HSTRINGS);

#ifdef __cplusplus
}
#endif