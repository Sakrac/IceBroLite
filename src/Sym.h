#pragma once

bool ReadSymbols(const char *binname);
void ReadViceCommandFile(const char *symFile);
void ReadSymbolsForBinary(const char *binname);
void ShutdownSymbols();
bool GetAddress(const char *name, size_t chars, uint16_t &addr);
bool SymbolsLoaded();
const char* GetSymbol(uint16_t address);
void AddSymbol(uint16_t address, const char *name, size_t chars);
const char* NearestLabel(uint16_t addr, uint16_t& offs);

