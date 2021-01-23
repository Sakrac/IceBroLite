#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include "struse/struse.h"
#include "6510.h"
#include <string.h>
#include <malloc.h>
#include "HashTable.h"
#include "Files.h"
#include "SourceDebug.h"
#include "Sym.h"
#include "Breakpoints.h"
#include "platform.h"

struct SymEntry {
	int16_t count;
};

struct SymList {
	enum {
		MIN_LIST = 7,
		GROW_LIST = 8
	};
	size_t capacity;
	const char* names[ 1 ];
};

union SymRef {
	const char* unique;
	SymList* multi;
};

struct SymbolInfo {
	uint32_t address;
	uint32_t section;
	char* label;
};

static SymEntry* sLabelCount = nullptr;
static SymRef* sLabelEntries = nullptr;
static std::vector<uint16_t> sortedSymAddrs;
static HashTable<uint64_t, uint32_t> sReverseLookup;
static HashTable<uint64_t, uint32_t> sDuplicateCheck;	// look up from section + symbol + value
static std::vector<char*> sectionNames;
static std::vector<SymbolInfo> labelList;
static std::vector<SymbolInfo> sortedLabelList;			// this is a copy of labelList without ownership of values
static std::vector<uint64_t> hiddenSections;			// hashed value of section name
static std::vector<uint32_t> matchedLabelList;			// search result
static bool lastSortedName = false;
static bool lastSortedUp = true;
static IBMutex symbolMutex;


bool SymbolsLoaded() { return sortedSymAddrs.size() > 0; }

void InitSymbols()
{
	IBMutexInit(&symbolMutex, "Symbol");
}

void ShutdownSymbols()
{
	IBMutexDestroy(&symbolMutex);
}


void ResetSymbols()
{
	IBMutexLock(&symbolMutex);
	sReverseLookup.Clear();
	sortedSymAddrs.clear();
	sortedLabelList.clear();
	if( sLabelCount ) {
		for( size_t adr = 0; adr < 0x10000; ++adr ) {
			if( sLabelCount[ adr ].count > 1 ) {
				if (sLabelEntries && sLabelEntries[adr].multi) {
					free(sLabelEntries[adr].multi);
				}
			}
		}
		free( sLabelCount );
		sLabelCount = nullptr;
		free( sLabelEntries );
		sLabelEntries = nullptr;
	}
	IBMutexRelease(&symbolMutex);
}

size_t GetLabelSlot(uint16_t addr)
{
	size_t lb = 0, ub = sortedSymAddrs.size();

	while ((ub-lb)>1) {
		size_t cb = (ub + lb) >> 1;
		uint16_t addr_cmp = sortedSymAddrs[cb];
		if (addr == addr_cmp) {
			return cb;
		} else if (addr > addr_cmp) {
			lb = cb;
		} else {
			ub = cb;
		}
	}
	return lb;
}

const char* NearestLabel(uint16_t addr, uint16_t& offs)
{
	IBMutexLock(&symbolMutex);
	size_t i = GetLabelSlot(addr);
	const char* ret = nullptr;
	offs = addr;
	if (i < sortedSymAddrs.size() && addr >= sortedSymAddrs[i]) {
		uint16_t prevAddr = sortedSymAddrs[i];
		offs = addr - prevAddr;
		if (sLabelEntries[prevAddr].unique) {
			ret = sLabelEntries[prevAddr].unique;
		} else if (SymList* syms = sLabelEntries[prevAddr].multi) {
			ret = sLabelCount[prevAddr].count ? syms->names[0] : nullptr;
		}
	}
	IBMutexRelease(&symbolMutex);
	return ret;
}

static int _compareSymAddrUp(const void* A, const void* B)
{
	return (int)((const SymbolInfo*)A)->address - (int)((const SymbolInfo*)B)->address;
}

static int _compareSymAddrDown(const void* A, const void* B)
{
	return (int)((const SymbolInfo*)B)->address - (int)((const SymbolInfo*)A)->address;
}

static int _compareSymNameUp(const void* A, const void* B)
{
	const char* sA = ((const SymbolInfo*)A)->label;
	const char* sB = ((const SymbolInfo*)B)->label;
	while (*sA && *sB) {
		char cA = *sA++; if (cA >= 'a' && cA <= 'z') { cA -= 'a' - 'A'; }
		char cB = *sB++; if (cB >= 'a' && cB <= 'z') { cB -= 'a' - 'A'; }
		if (cA != cB) { return cA - cB; }
	}
	if (!*sA && !*sB) { return 0; }
	if (*sB) { return 1; }
	return -1;
}

static int _compareSymNameDown(const void* A, const void* B)
{
	const char* sA = ((const SymbolInfo*)A)->label;
	const char* sB = ((const SymbolInfo*)B)->label;
	while (*sA && *sB) {
		char cA = *sA++; if (cA >= 'a' && cA <= 'z') { cA -= 'a' - 'A'; }
		char cB = *sB++; if (cB >= 'a' && cB <= 'z') { cB -= 'a' - 'A'; }
		if (cA != cB) { return cB - cA; }
	}
	if (!*sA && !*sB) { return 0; }
	if (*sA) { return 1; }
	return -1;
}

void SortSymbols(bool up, bool name)
{
	IBMutexLock(&symbolMutex);
	lastSortedName = name;
	lastSortedUp = up;
	size_t numSymbols = sortedLabelList.size();
	if (numSymbols) {
		SymbolInfo* symbols = &sortedLabelList[0];
		if (name) {
			if (up) {
				qsort(symbols, numSymbols, sizeof(SymbolInfo), _compareSymNameUp);
			} else {
				qsort(symbols, numSymbols, sizeof(SymbolInfo), _compareSymNameDown);
			}
		} else {
			if (up) {
				qsort(symbols, numSymbols, sizeof(SymbolInfo), _compareSymAddrUp);
			} else {
				qsort(symbols, numSymbols, sizeof(SymbolInfo), _compareSymAddrDown);
			}
		}
	}
	IBMutexRelease(&symbolMutex);
}

size_t NumSymbolSearchMatches() { return matchedLabelList.size() ? matchedLabelList.size() : sortedLabelList.size(); }
const char* GetSymbolSearchMatch(size_t i, uint32_t* address, const char** section)
{
	IBMutexLock(&symbolMutex);
	if (!matchedLabelList.size()) {
		if (i < sortedLabelList.size()) {
			SymbolInfo sym = sortedLabelList[i];
			if ((size_t)sym.section < sectionNames.size()) {
				*section = sectionNames[sym.section];
			} else { *section = ""; }
			*address = sym.address;
			IBMutexRelease(&symbolMutex);
			return sym.label;
		}
	} else if (i < matchedLabelList.size()) {
		uint32_t o = matchedLabelList[i];
		if ((size_t)o < sortedLabelList.size()) {
			SymbolInfo sym = sortedLabelList[o];
			if ((size_t)sym.section < sectionNames.size()) {
				*section = sectionNames[sym.section];
			} else { *section = ""; }
			*address = sym.address;
			IBMutexRelease(&symbolMutex);
			return sym.label;
		}
	}
	IBMutexRelease(&symbolMutex);
	return nullptr;
}

void SearchSymbols(const char* pattern, bool case_sensitive)
{
	matchedLabelList.clear();
	if (!*pattern) { return; }	// clear search string -> show all

	strown<512> wildcard;
	if (*pattern == '*') {
		wildcard.copy(pattern + 1);	// substring
	} else {
		wildcard.append('@').append(pattern);
	}

	for (size_t i = 0, n = sortedLabelList.size(); i < n; ++i) {
		const char* str = sortedLabelList[i].label;
		if (str && str[0]) {
			if (strref(str).find_wildcard(wildcard.get_strref(), 0, case_sensitive)) {
				matchedLabelList.push_back((uint32_t)i);
			}
		}
	}
}

size_t NumHiddenSections() { return hiddenSections.size(); }
uint64_t GetHiddenSection(size_t index) { return hiddenSections[index]; }
void HideSection(uint64_t section, bool hide)
{
	bool found = false;
	for (std::vector<uint64_t>::iterator h = hiddenSections.begin(); h != hiddenSections.end(); ++h) {
		if (*h == section) {
			if (!hide) {
				hiddenSections.erase(h);
				FilterSectionSymbols();
			}
			return; // removed if shown or already hidden
		}
	}
	hiddenSections.push_back(section);
	FilterSectionSymbols();
}
size_t NumSections() { return sectionNames.size(); }
const char* GetSectionName(size_t index) { return sectionNames[index]; }


void BeginAddingSymbols()
{
	IBMutexLock(&symbolMutex);
	sDuplicateCheck.Clear();
	for (size_t i = 0, n = sectionNames.size(); i < n; ++i) {
		if (char* sectName = sectionNames[i]) {
			sectionNames[i] = nullptr;
			free(sectName);
		}
	}
	sectionNames.clear();
	for (size_t i = 0, n = labelList.size(); i < n; ++i) {
		if (char* symName = labelList[i].label) {
			labelList[i].label = nullptr;
			free(symName);
		}
	}
	labelList.clear();
	hiddenSections.clear();
	IBMutexRelease(&symbolMutex);
}

// discard all symbol lookups and fill out with a filtered set of sections
// call after loading symbols
void FilterSectionSymbols()
{
	size_t numSects = sectionNames.size();
	uint8_t* hidden = (uint8_t*)calloc(1, numSects);
	if (hidden == nullptr) { return; }
	for (std::vector<uint64_t>::iterator i = hiddenSections.begin(); i != hiddenSections.end(); ++i) {
		uint64_t hiddenName = *i;
		for (size_t j = 0; j < numSects; ++j ) {
			if (strref(sectionNames[j]).fnv1a_64() == hiddenName) {
				hidden[j] = 1;
				break;
			}
		}
	}
	ResetSymbols();

	sortedLabelList.clear();

	IBMutexLock(&symbolMutex);
	// make sure label array exists
	if (!sLabelCount) {
		sLabelCount = (SymEntry*)malloc(sizeof(SymEntry) * 0x10000);
		sLabelEntries = (SymRef*)malloc(sizeof(SymRef) * 0x10000);
	}
	if (sLabelEntries == nullptr || sLabelCount == nullptr) { return; }

	memset(sLabelCount, 0, sizeof(SymEntry) * 0x10000);
	memset(sLabelEntries, 0, sizeof(SymRef) * 0x10000);

	for (std::vector<SymbolInfo>::iterator sym = labelList.begin(); sym != labelList.end(); ++sym) {
		if (hidden[sym->section]) { continue; }	// if this section is hidden don't add it!
		if (sym->label == nullptr) { continue; }

		strref lbl(sym->label);

		sortedLabelList.push_back(*sym);

		// 32 bit vymbol support
		uint64_t hash = lbl.fnv1a_64();
		if (!sReverseLookup.Exists(hash)) {
			sReverseLookup.Insert(hash, sym->address);
		}

		// 16 bit symbol support
		if (sym->address < 0x10000) {
			uint16_t address = (uint16_t)sym->address;
			if (sLabelCount[address].count) {
				if (sLabelCount[address].count == 1) {
					if (lbl.same_str_case(sLabelEntries[address].unique)) { return; }
				} else {
					if (const char** ppStr = sLabelEntries[address].multi->names) {
						for (size_t i = 0, n = sLabelCount[address].count; i < n; ++i) {
							if (lbl.same_str(*ppStr)) { continue; }
							++ppStr;
						}
					}
				}
			}

			if (!sLabelCount[address].count) {
				sLabelCount[address].count = 1;
				sLabelEntries[address].unique = sym->label;
			} else if (sLabelCount[address].count == 1) {
				const char* prev = sLabelEntries[address].unique;
				sLabelEntries[address].multi = (SymList*)malloc(sizeof(SymList) + sizeof(const char*) * (SymList::MIN_LIST - 1));
				sLabelEntries[address].multi->capacity = SymList::MIN_LIST;
				sLabelCount[address].count = 2;
				sLabelEntries[address].multi->names[0] = prev;
				sLabelEntries[address].multi->names[1] = sym->label;
			} else {
				SymList* entries = sLabelEntries[address].multi;
				if (entries->capacity == (size_t)sLabelCount[address].count) {
					sLabelEntries[address].multi = (SymList*)malloc(
						sizeof(SymList) + sizeof(const char*) * (entries->capacity + SymList::GROW_LIST - 1));
					sLabelEntries[address].multi->capacity = entries->capacity + SymList::GROW_LIST;
					memcpy(sLabelEntries[address].multi->names, entries->names, sizeof(const char*) * sLabelCount[address].count);
					free(entries);
					entries = sLabelEntries[address].multi;
				}
				entries->names[sLabelCount[address].count++] = sym->label;
			}

			// insert into range slot array
			size_t slot = GetLabelSlot(address);
			if (slot < sortedSymAddrs.size()) {
				if (address < sortedSymAddrs[slot]) {
					sortedSymAddrs.insert(sortedSymAddrs.begin() + slot, address);
				} else if (address > sortedSymAddrs[slot]) {
					sortedSymAddrs.insert(sortedSymAddrs.begin() + slot + 1, address);
				}
			} else {
				sortedSymAddrs.push_back(address);
			}
		}
	}
	free(hidden);
	IBMutexRelease(&symbolMutex);

	SortSymbols(lastSortedUp, lastSortedName);
}

void AddSymbol(uint32_t address, const char *symbol, size_t symbolLen, const char *section, size_t sectionLen)
{
	strref sym(symbol, (strl_t)symbolLen);
	strref sect(section, (strl_t)sectionLen);

	//sDuplicateCheck
	uint64_t hash = sect.fnv1a_64(((uint64_t)address<<16) + 14695981039346656037ULL);
	hash = sym.fnv1a_64(hash);

	if (sDuplicateCheck.Exists(hash)) { return; }
	IBMutexLock(&symbolMutex);
	sDuplicateCheck.Insert(hash, address);

	size_t sectIdx = 0, numSects = sectionNames.size();
	for (; sectIdx < numSects; ++sectIdx) {
		if (sect.same_str(sectionNames[sectIdx]) || (!sect.valid() && !sectionNames[sectIdx][0])) { break; }
	}
	if (sectIdx == numSects) {
		char* sectionCopy = (char*)calloc(1, sect.get_len() + 1);
		if (sect.get_len()) { memcpy(sectionCopy, sect.get(), sect.get_len()); }
		sectionNames.push_back(sectionCopy);
	}
	char* copy = (char*)calloc(1, sym.get_len() + 1);
	memcpy(copy, sym.get(), sym.get_len());
	SymbolInfo symInfo = { address, (uint32_t)sectIdx, copy };
	labelList.push_back(symInfo);
	IBMutexRelease(&symbolMutex);
}

void ClearSymbols()
{
	ResetSymbols();
	BeginAddingSymbols();
}

const char* GetSymbol(uint16_t address)
{
	const char* sym = nullptr;
	if( sLabelCount ) {
		IBMutexLock(&symbolMutex);
		if( !sLabelCount[ address ].count ) { sym = nullptr; }
		else if( sLabelCount[ address ].count == 1 ) { sym = sLabelEntries[ address ].unique; }
		else { sym = sLabelEntries[address].multi->names[0]; }
		IBMutexRelease(&symbolMutex);
	}
	return sym;
}

bool GetAddress( const char *name, size_t chars, uint16_t &addr )
{
	uint64_t key = strref(name, (strl_t)chars).fnv1a_64();
	IBMutexLock(&symbolMutex);
	if( uint32_t* value = sReverseLookup.Value( key ) ) {
		addr = (uint16_t)*value;
		IBMutexRelease(&symbolMutex);
		return true;
	}
	IBMutexRelease(&symbolMutex);
	return false;
}

void ReadViceCommandFile(const char *symFile)
{
	ResetSymbols();
	FILE* f;
	if (fopen_s(&f, symFile, "rb") == 0 && f) {
		fseek(f, 0, SEEK_END);
		uint32_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (void *voidbuf = malloc(size)) {
			fread(voidbuf, size, 1, f);

			for (int pass = 0; pass<2; pass++) {
				strref file((const char*)voidbuf, size);

				while (strref line = file.line()) {
					if (strref command = line.get_word()) {
						uint32_t addr;
						line += command.get_len();
						line.trim_whitespace();
						if (command.same_str("break") || command.same_str("bk")) {
							if (pass) {
								if (line.get_first() == '$') { ++line; }
								ViceAddBreakpoint((uint16_t)(line + 1).ahextoui());
							}
						} else if (command.same_str("al") || command.same_str("add_label")) {
							if (line.has_prefix("c:")) { line += 2; }
							line.skip_whitespace();
							if (line.get_first() == '$') { ++line; }
							addr = (uint16_t)line.ahextoui_skip();
							line.skip_whitespace();
							if (addr < 0x10000) {
								AddSymbol(addr, line.get(), line.get_len(), nullptr, 0);
							}
						}
					}
				}
			}
			free(voidbuf);
		}
		fclose(f);
	}
}

bool ReadSymbols(const char *filename)
{
	ResetSymbols();
	FILE *f;
	if (fopen_s(&f, filename, "rb") == 0 && f != nullptr) {
		fseek(f, 0, SEEK_END);
		uint32_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (void *voidbuf = malloc(size)) {
			BeginAddingSymbols();
			fread(voidbuf, size, 1, f);
			strref file((const char*)voidbuf, size);
			while (file) {
				if (strref line = file.line()) {
					line.skip_whitespace();
					if (line.grab_prefix(".label")) {
						line.skip_whitespace();
						strref label = line.split_label();
						line.skip_whitespace();
						if (line.grab_char('=')) {
							line.skip_whitespace();
							if (line.grab_char('$')) {
								size_t addr = line.ahextoui();
								if (label.same_str("debugbreak")) {
									Breakpoint bp;
									if (!BreakpointAt((uint16_t)addr, bp)) {
										ViceAddBreakpoint((uint16_t)addr);
									}
								} else {
									AddSymbol((uint16_t)addr, label.get(), label.get_len(), nullptr, 0);
								}
							}
						}
					}
				}
			}
			free(voidbuf);
		}
		fclose(f);
		FilterSectionSymbols();
		return true;
	}
	return false;
}

void ReadSymbolsForBinary(const char *binname)
{
	strref origname = strref(binname).before_last('.');
	if (!origname) { origname = strref(binname); }

	strown<PATH_MAX_LEN> symFile(origname);

	symFile.append(".dbg");
	if (ReadC64DbgSrc(symFile.c_str())) { return; }

	symFile.append(".sym");
	if (ReadSymbols(symFile.c_str())) { return; }

	symFile.copy(origname);
	symFile.append(".vs");
	ReadViceCommandFile(symFile.c_str());
}

