#pragma once

#include <stdint.h>
bool SaveFile(const char* filename, void* data, size_t size);
uint8_t* LoadBinary(const char* name, size_t& size);

#ifndef MSC_VER
int fopen_s(FILE **f, const char* filename, const char *options);
#endif

#ifndef _WIN32
#include <linux/limits.h>
#define PATH_MAX_LEN PATH_MAX
#define sprintf_s sprintf
#else
#define PATH_MAX_LEN _MAX_PATH
#endif
