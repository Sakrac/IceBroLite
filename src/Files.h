#pragma once

#include <stdint.h>
bool SaveFile(const char* filename, void* data, size_t size);
uint8_t* LoadBinary(const char* name, size_t& size);

#ifndef _MSC_VER
int fopen_s(FILE **f, const char* filename, const char *options);
#endif

#if defined(__APPLE__) || defined(__linux__)
#include <limits.h>
#define PATH_MAX_LEN PATH_MAX
#define sprintf_s(buf, ...) snprintf((buf), sizeof(buf), __VA_ARGS__)
#elif defined(_WIN32)
#define PATH_MAX_LEN _MAX_PATH
#else
#error "Unknown platform"
#endif
