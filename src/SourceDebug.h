#pragma once

bool ReadC64DbgSrcExtra(const char* filename);

bool ReadC64DbgSrc(const char* filename);
bool ReadListingFile(const char* filename);
strref GetSourceAt(uint16_t addr, int &spaces);
strref GetListingFile();
void ListingToSrcDebug(int column);
void InitSourceDebug();
void ShutdownSourceDebug();



