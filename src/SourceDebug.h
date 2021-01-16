#pragma once

bool ReadC64DbgSrc(const char* filename);
strref GetSourceAt(uint16_t addr, int &spaces);
void ShutdownSourceDebug();


