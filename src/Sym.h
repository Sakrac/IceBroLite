#pragma once

bool ReadSymbols(const char *binname);
void ReadViceCommandFile(const char *symFile);
void ReadSymbolsForBinary(const char *binname);
void ClearSymbols();
bool GetAddress(const char *name, size_t chars, uint16_t &addr);
bool SymbolsLoaded();
const char* GetSymbol(uint16_t address);
void AddSymbol(uint32_t address, const char* symbol, size_t symbolLen, const char* section, size_t sectionLen);
void FilterSectionSymbols();
const char* NearestLabel(uint16_t addr, uint16_t& offs);

struct SymbolDragDrop {
	uint32_t address;
	char symbol[128];
};
void SortSymbols(bool up, bool name);
size_t NumSymbolSearchMatches();
const char* GetSymbolSearchMatch(size_t i, uint32_t* address, const char** section);
void SearchSymbols(const char* pattern, bool case_sensitive);

size_t NumHiddenSections();
uint64_t GetHiddenSection(size_t index);
void HideSection(uint64_t section, bool hide);
size_t NumSections();
const char* GetSectionName(size_t index);
bool IsSectionVisible(uint64_t section);

void InitSymbols();
void ShutdownSymbols();
