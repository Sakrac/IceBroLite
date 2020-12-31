#pragma once

#include <stdint.h>
bool SaveFile(const char* filename, void* data, size_t size);
uint8_t* LoadBinary(const char* name, size_t& size);
